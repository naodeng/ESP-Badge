# Changelog

What changed, and — more usefully — why it broke in the first place.

The [README](README.md) calls the checks "a list of scars". This file is the
same idea for behaviour: each entry tries to leave you with something you can
use on your own board, not just a version number.

## 2026-11-10 — Agent Badge (personal combined build)

### Added

- **Codex app**: Usage rings, CodexRadar IQ page, Status page. Data comes from
  the macOS bridge (`tools/codex_bridge_http.py`), which reads `~/.codex` and
  serves usage, radar and a PNG-to-RLE scene endpoint.
- **Sand app**: adapted `sand_sim.c` on an LVGL 466x466 canvas, 12 materials,
  tilt gravity, touch brush, two-row palette, radial menu with two flash scene
  slots (RLE in the tail of the `rec` partition), bridge PNG import.
- **Icons**: generated 120x120 RGB565A8 Pet/Codex/Sand tiles
  (`tools/gen_agent_icons.py`).
- **UDP discovery**: the badge broadcasts on 8790 and the bridge answers with the
  LAN address it sees, so the Mac IP no longer needs to be compiled in.
- **WiFi migration**: the old Codey NVS namespace `codey/nets` is imported into
  the Badge WiFi slots on first boot.

### Fixed

- Codex fetch task stack 6 KB -> 10 KB. The old stack left 716 bytes free,
  overflowed and corrupted LVGL objects, rebooting when Codex opened.
- Sand scene save/load moved from the full NVS partition to `rec` tail slots.
- Sand menu is created on `lv_layer_top()`; the first version was created as a
  sibling of the canvas and could stay invisible.
- Sand painting now uses LVGL canvas touch coordinates and converts screen
  pixels to 116x116 grid cells.

### Deferred

- USB PNG import: the Badge's TinyUSB exposes a read-only recordings FAT and
  has no teardown; a writeable FAT mode needs a separate reboot-based design.
- Pet: needs Claude Desktop Hardware Buddy / BLE approval.

## 2026-09-29

### Fixed — a quick tap after stopping a recording showed "!"

**What you saw.** Stop a recording, tap through OK, tap REC again straight
away — and instead of a new recording you get the red "!" screen. Wait a
moment and try again, and it works.

**Why.** The screen moves to OK the instant you press stop, but the mic task
is still finishing: it writes the last page, updates the directory and hands
the I2S channel back. `port_rec_start()` refused to start while the old task
was still running, and a fast tap landed inside that window.

**Fix.** If the running recording has been told to stop, `port_rec_start()`
now waits for it (up to one second) before starting the next one. The
finishing path never takes the LVGL lock, so waiting from the UI cannot
deadlock.

**Check.** `tools/sim-rec-again-check.py` records three times in a row, at a
normal pace and with taps 60 ms apart. The simulator stops instantly, so it
cannot reproduce this particular race — the check guards the flow around it.

## 2026-09-21

### Fixed — the stopwatch started when you swiped away from it

**What you saw.** You swipe up to leave the stopwatch, and the moment your
finger lifts the clock is running. Swipe back in to stop it, and now it has a
stray lap. Annoying in the way that makes you stop trusting the screen.

**Why.** The stopwatch takes the whole screen as its button — there is no small
"start" target to hit, which is the right call on a 466 px round display you
are looking at from a metre away. It watched `PRESSED` to start a long-press
timer, and `RELEASED` to toggle the run state:

```c
if (code == LV_EVENT_RELEASED) {
    if (!s_long_done) set_run(!s_run);
}
```

That is correct for a tap and wrong for everything else. A swipe is also a
press followed by a release — the app never asked **where** the finger went in
between, so a gesture meant for the launcher landed on the stopwatch on its way
out.

**The fix.** Remember where the press started; on release, ignore it if the
finger travelled more than 60 px. Long-press (lap / reset) is untouched.

```c
int dx = p.x - s_press_pt.x, dy = p.y - s_press_pt.y;
if (dx * dx + dy * dy >= 60 * 60) return;   /* that was a swipe */
```

**If you are building something similar.** Any full-screen tap target on a
device whose main navigation is a swipe has this bug until you add the distance
check. It will not show up while you are testing the feature itself — you only
find it once the screen has a neighbour to swipe to. Ours hid for months.

The same guard already lives in the launcher and in the badge's key app; this
just brings the stopwatch in line with them.
