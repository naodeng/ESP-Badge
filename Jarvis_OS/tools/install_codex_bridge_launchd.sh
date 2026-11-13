#!/usr/bin/env bash
# Install a per-user LaunchAgent so the Codex bridge starts at login.
# Run once from a terminal:  Jarvis_OS/tools/install_codex_bridge_launchd.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PLIST="$HOME/Library/LaunchAgents/com.jarvisos.codexbridge.plist"
LEGACY_PLIST="$HOME/Library/LaunchAgents/com.agentbadge.codexbridge.plist"
LOG="$HOME/Library/Logs/JarvisOS-codex.log"
mkdir -p "$(dirname "$PLIST")" "$(dirname "$LOG")"

cat > "$PLIST" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Label</key><string>com.agentbadge.codexbridge</string>
  <key>ProgramArguments</key>
  <array><string>/bin/bash</string><string>$ROOT/tools/run_codex_bridge.sh</string></array>
  <key>RunAtLoad</key><true/>
  <key>KeepAlive</key><true/>
  <key>StandardOutPath</key><string>$LOG</string>
  <key>StandardErrorPath</key><string>$LOG</string>
  <key>EnvironmentVariables</key>
  <dict><key>PATH</key><string>/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin</string></dict>
</dict>
</plist>
EOF

launchctl unload "$LEGACY_PLIST" 2>/dev/null || true
launchctl unload "$PLIST" 2>/dev/null || true
launchctl load "$PLIST"
echo "Installed $PLIST"
echo "Bridge log: $LOG"
echo "Stop the manual bridge first if port 8790 is already in use."
