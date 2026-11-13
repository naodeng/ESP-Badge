#!/usr/bin/env python3
"""Serve a compact Codex usage/activity snapshot for Jarvis_OS over HTTP.

The collection code is Codex Island's macOS bridge (personal use, see
INTEGRATION.md). It reads ~/.codex auth/sessions and keeps credentials local.
"""
from __future__ import annotations

import json
import os
import re
import socket
import threading
import time
import traceback
from dataclasses import asdict
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from codex_island_bridge.codex_activity import CodexActivityProvider
from codex_island_bridge.config import Settings
from codex_island_bridge.service import UsageService
import requests

PORT = int(os.environ.get("CODEX_BRIDGE_PORT", "8790"))
CACHE_SECONDS = float(os.environ.get("CODEX_BRIDGE_CACHE", "60"))

_settings = Settings.from_environment()
_usage_service = UsageService(_settings)
_activity_provider = CodexActivityProvider(_settings.codex_sessions_dir)
_cache_at = 0.0
_cache_body: bytes | None = None

# ── Radar (降智雷达) ─────────────────────────────────────────────
# codexradar.com now renders the IQ table from embedded JSON rather than the
# semantic section the bridge's HtmlRadarProvider used. The score150 rows are
# still in the page: {"model":"...","effort":"...","score150":...}. Parse
# those directly and cache for 30 minutes.
_radar_lock = threading.Lock()
_radar_rows: list[dict] = []
_radar_updated = 0
_RADAR_HEADERS = {"User-Agent": "JarvisOS/0.1 (+local macOS bridge; 30 min)"}
_RADAR_RE = re.compile(r'\{[^{}]*?"model":"([^"]+)"[^{}]*?"effort":"([^"]+)"[^{}]*?"score150":([0-9.]+)')


def _radar_refresh() -> None:
    global _radar_rows, _radar_updated
    try:
        response = requests.get("https://codexradar.com/", headers=_RADAR_HEADERS, timeout=25)
        response.raise_for_status()
        best: dict[tuple[str, str], float] = {}
        for model, effort, score in _RADAR_RE.findall(response.text):
            key = (model, effort)
            value = float(score)
            if value > best.get(key, 0):
                best[key] = value
        rows = [
            {"m": m, "e": e, "iq": round(v * 10)}
            for (m, e), v in sorted(best.items(), key=lambda item: item[1], reverse=True)
        ]
        with _radar_lock:
            _radar_rows = rows[:16]
            _radar_updated = int(time.time())
        print(f"[bridge] radar rows={len(_radar_rows)}", flush=True)
    except Exception as exc:
        print(f"[bridge] radar refresh failed: {exc}", flush=True)


def _local_ip_for(peer: str) -> str:
    """Source address the kernel would use to reach this peer (not a VPN)."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect((peer, 8790))
        return s.getsockname()[0]
    finally:
        s.close()


def _udp_loop() -> None:
    """Answer the badge's LAN discovery broadcast with this Mac's IP."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", PORT))
    while True:
        try:
            data, addr = sock.recvfrom(64)
        except OSError:
            continue
        if data.strip() == b"AGENTBADGE/1":
            ip = _local_ip_for(addr[0]).encode()
            sock.sendto(ip, addr)
            print(f"[bridge] discovery reply {ip.decode()} -> {addr[0]}", flush=True)


def _radar_loop() -> None:
    while True:
        _radar_refresh()
        with _radar_lock:
            ready = bool(_radar_rows)
        time.sleep(1800 if ready else 60)


def _snapshot() -> dict:
    usage = _usage_service.collect()
    activity = _activity_provider.collect()
    now = int(time.time())
    reset_s = max(0, usage.five_hour_reset_at - now) if usage.five_hour_reset_at else 0
    return {
        "v": 1,
        "ts": now,
        "usage": {
            "p5": usage.five_hour_percent,
            "p7": usage.seven_day_percent,
            "reset_s": reset_s,
            "tok": usage.today_tokens,
            "cost_c": usage.today_cost_cents,
            "daily": list(usage.daily_tokens),
            "plan": usage.plan,
            "updated_at": usage.updated_at,
        },
        "activity": {
            "state": activity.state,
            "active_tasks": activity.active_tasks,
            "updated_at": activity.updated_at,
        },
        "radar": _radar_snapshot(),
    }


def _radar_snapshot() -> dict:
    with _radar_lock:
        rows = list(_radar_rows)
        updated = _radar_updated
    return {
        "updated": updated,
        "stale": (not updated) or (time.time() - updated > 7200),
        "rows": rows,
    }


class Handler(BaseHTTPRequestHandler):
    server_version = "CodexBridge/1.0"

    def _send(self, code: int, payload: bytes, ctype: str = "application/json") -> None:
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self) -> None:  # noqa: N802
        global _cache_at, _cache_body
        if self.path == "/health":
            self._send(200, b'{"ok":true}')
            return
        path = self.path.split("?")[0]
        if path != "/codex.json":
            self._send(404, b'{"error":"not found"}')
            return

        now = time.time()
        if _cache_body is None or now - _cache_at > CACHE_SECONDS:
            try:
                body = json.dumps(_snapshot(), separators=(",", ":"), ensure_ascii=False).encode()
                _cache_body = body
                _cache_at = now
            except Exception as exc:  # keep the last good snapshot on transient errors
                traceback.print_exc()
                if _cache_body is None:
                    self._send(503, json.dumps({"error": str(exc)}).encode())
                    return
                stale = json.loads(_cache_body)
                stale["stale"] = True
                self._send(200, json.dumps(stale, separators=(",", ":")).encode())
                return
        self._send(200, _cache_body)

    def log_message(self, fmt: str, *args) -> None:  # quiet per-request logging
        print("[bridge] " + fmt % args, flush=True)


def main() -> None:
    print(f"[bridge] Codex bridge on http://0.0.0.0:{PORT}/codex.json", flush=True)
    print(f"[bridge] data_dir={_settings.data_dir}", flush=True)
    threading.Thread(target=_radar_loop, daemon=True).start()
    threading.Thread(target=_udp_loop, daemon=True).start()
    ThreadingHTTPServer(("0.0.0.0", PORT), Handler).serve_forever()


if __name__ == "__main__":
    main()
