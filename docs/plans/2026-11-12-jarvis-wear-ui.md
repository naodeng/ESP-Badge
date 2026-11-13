# Jarvis Wear-Inspired UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace Jarvis's two-page app grid with a legible, status-first circular carousel, natural-water artwork, redesigned unlock, and dedicated Quick Controls while preserving each app's behavior.

**Architecture:** Keep LVGL screens and the existing launcher/app lifecycle. Replace only the home shell and lock-screen input; use a small draw-event water background rather than a full-screen bitmap. Add an app-card snapshot interface for live Clock, Codex, and Meet summaries, and expose the existing Settings state through Quick Controls rather than duplicating it.

**Tech Stack:** ESP-IDF 5.5.4, LVGL 9.5, C, existing PC LVGL simulator and ESP32-S3 device.

**Design record:** `docs/design/history/2026-11-12-wear-interaction-proposal.md`

## Global Constraints

- Target Waveshare ESP32-S3-Touch-AMOLED-1.75C, 466 × 466 circular AMOLED, ESP32-S3R8, 32 MB Flash, 8 MB PSRAM.
- Preserve current screen, touch, power, application lifecycle, recording, and Codex bridge interfaces unless this plan names an additive interface.
- Keep Quick Controls available only over lock/home; closing restores the exact covered screen.
- Refresh the Codex home snapshot every 30 seconds only while its card is selected and the display is awake; mark data stale after 60 seconds; keep Codex's in-app 10-second refresh unchanged.
- Keep Settings' explicit time-sync action in full Settings; Quick Controls display sync state only.
- Preserve the 13 lock-screen expressions and active-app resume behavior.
- Use an OLED-black base with low-contrast natural-water ripples; do not restore the purple-red route motif or yellow outer ring.
- Add no Android runtime or dependency; use ESP-IDF and LVGL.

## Review Focus

- A horizontal swipe begun over an app card must advance exactly one card without opening it; Task 3 captures and checks all nine home states.
- Rebuilding home or lock repeatedly must not leak draw buffers, duplicate timers, or retain dead LVGL objects; Tasks 1, 3, and 5 check repeated transitions in the simulator and firmware logs.
- Opening/closing Quick Controls or sleeping while Codex is selected must stop home polling and restore the exact prior screen; Tasks 2 and 4 exercise the visible/hidden lifecycle and stale labels.
- Offline Codex responses and unknown battery readings must preserve cached information and show explicit offline/unknown states; Tasks 2 and 4 check both snapshots.
- Meet recording, Games/Water touch input, and returning to a live app after unlock must keep their existing input and lifecycle behavior; Tasks 4 and 5 smoke-check those flows on the simulator and device.

---

### Task 1: Cool-water icon family and lightweight background

**Files:**
- Modify: `agent-badge/tools/mkassets.py`
- Modify: `agent-badge/tools/gen_agent_icons.py`
- Regenerate: `agent-badge/main/assets/app_icon_*.c`
- Regenerate: `agent-badge/main/assets/icon_gear.c`
- Create: `agent-badge/main/waterfield.c`
- Create: `agent-badge/main/waterfield.h`
- Delete: `agent-badge/main/nightsky.c`
- Delete: `agent-badge/main/nightsky.h`
- Modify: `agent-badge/main/CMakeLists.txt`
- Modify: `agent-badge/main/launcher.c`
- Modify: `agent-badge/main/lock.c`
- Modify: `agent-badge/tools/regress.sh`

**Interfaces:**
- Produces: `lv_obj_t *waterfield_create(lv_obj_t *parent)`; creates one non-clickable LVGL draw-event child, with no retained 466 × 466 pixel buffer.
- Consumes: LVGL 9 draw-arc/line APIs and the parent screen's 466 × 466 coordinates.

- [x] Add a regression check that requires the new `waterfield_create()` module and rejects the retired night-sky dependency; run `./tools/regress.sh` and confirm the new check fails before implementation.
- [x] Replace icon generator magenta/yellow accents with a consistent slate, blue, and cyan palette; keep each icon's silhouette distinct and retain the existing 120 × 120 RGB565A8 format.
- [x] Regenerate the eight app icon assets and monochrome Settings gear from the generator scripts.
- [x] Implement `waterfield_create()` with two off-center, low-opacity ripple groups and a black OLED base; keep the center low contrast and the object non-clickable/non-scrollable.
- [x] Replace the `nightsky` include/use and CMake source with `waterfield` in launcher and lock.
- [x] Run `python3 tools/mkassets.py` and `python3 sim/build.py`; expected: assets regenerate and simulator links without warnings promoted to errors.
- [x] Run the simulator screenshot mode; expected: ripples remain inside the round mask, leave center text legible, and use no purple/red route or yellow perimeter.

### Task 2: App-card snapshots and Codex home refresh

**Files:**
- Modify: `agent-badge/main/app.h`
- Modify: `agent-badge/main/apps/app_clock.c`
- Modify: `agent-badge/main/apps/app_codex.c`
- Modify: `agent-badge/main/apps/app_meet.c`
- Modify: `agent-badge/main/apps/app_calc.c`
- Modify: `agent-badge/main/apps/app_games.c`
- Modify: `agent-badge/main/apps/app_settings.c`
- Modify: `agent-badge/main/apps/app_water.c`
- Modify: `agent-badge/main/apps/app_waveshare.c`

**Interfaces:**
- Produces: `badge_home_status_t { char value[24]; char detail[48]; uint32_t accent; bool active; bool stale; }` and optional `void (*home_status)(badge_home_status_t *out)` on `badge_app_t`; static launch hint remains `home_hint`.
- Produces: `void app_clock_home_status(...)`, `void app_meet_home_status(...)`, and `void app_codex_home_status(...)` snapshots; `void app_codex_home_active(bool active)` controls the selected-card lifecycle.
- Consumes: `port_battery_*`, `port_rec_*`, existing Clock time source, and Codex's existing bridge cache/fetch task.

- [x] Add simulator assertions for non-empty Clock/Meet snapshots and Codex cached/offline status; run and confirm they fail because the callbacks are missing.
- [x] Add the card snapshot and hint fields to `badge_app_t` without changing existing `enter`, `leave`, `screen`, or radio behavior.
- [x] Implement Clock snapshot as current local time/date and Meet snapshot as ready/recording plus saved/free state.
- [x] Implement Codex snapshot under the existing port lock, showing cached activity, task count, online/offline state, last update time, and stale-after-60-seconds state.
- [x] Extend the existing Codex worker to poll at 30-second intervals only while the selected card is visible and awake; leave its active-app interval at 10 seconds and stop polling on lock, another card, or sleep.
- [x] Populate short, readable hints for Calc, Waveshare, Games, Water, Orbit, and Settings.
- [x] Compile with `python3 sim/build.py`; expected: the simulator enforces the project's strict C diagnostics for the new interface.
- [x] Assert initial, recent offline-cache, and stale Codex snapshots in screenshot-mode simulator; expected: cached activity remains visible offline and turns stale at 60 seconds.

### Task 3: Status-first single-app carousel

**Files:**
- Modify: `agent-badge/main/launcher.c`
- Modify: `agent-badge/main/app.h`
- Modify: `agent-badge/sim/main_sim.c`

**Interfaces:**
- Produces: `void launcher_home_move(int direction)` for one-step navigation, shared by touch handling and simulator screenshot capture.
- Consumes: the nine-app order Clock, Codex, Meet, Calc, Waveshare, Games, Water, Orbit, Settings and Task 2's card snapshot interface.

- [x] Replace the two page arrays with the single nine-app registry and persistent selected index.
- [x] Build one safe-area card at a time with redesigned icon, app name, status/value or concise hint, tap action, and readable `NN / 09` position.
- [x] Use touch press/release coordinates so a horizontal swipe changes one card and suppresses the tap; update Codex home-active state when entering/leaving its card.
- [x] Add simulator touch sweeps over a card, background, and battery control; confirm one-step navigation and tap suppression fail before the new handlers are implemented.
- [x] Move battery status to the top edge as a compact independent control; remove the center Settings/battery capsule and old accent colors.
- [x] Keep Settings as the final app card; tapping it still calls the existing `launcher_open(&app_settings)` path.
- [x] Add simulator screenshot capture for all nine card positions and the first/last wraparound; expected: exactly one app is primary, the count matches its index, and no content crosses the round safe area.
- [x] Render Clock, Meet, and Codex idle/offline/cached snapshots in simulator screenshots; expected: values fit the card and the offline path retains cached information.
- [x] Run the simulator and `./tools/regress.sh`; expected: all existing stability checks pass, with only checks explicitly tied to the retired two-page grid updated to the new one-step carousel behavior.

### Task 4: Lock-screen unlock and Quick Controls

**Files:**
- Modify: `agent-badge/main/launcher.c`
- Modify: `agent-badge/main/lock.c`
- Modify: `agent-badge/main/grokface.c`
- Modify: `agent-badge/main/grokface.h`
- Modify: `agent-badge/main/app.h`
- Modify: `agent-badge/main/apps/app_settings.c`
- Modify: `agent-badge/sim/main_sim.c`

**Interfaces:**
- Produces: `void launcher_quick_controls_open(void)` and `void launcher_quick_controls_close(void)`; overlay only opens when no app is running and deletes cleanly to reveal the covered lock/home screen.
- Produces: shared Settings helpers for brightness and sound so Quick Controls persists and reflects the same values as full Settings.
- Consumes: `launcher_get/set_autorotate`, `port_battery_*`, `port_time_sync_state`, and the current grokface time/battery data.

- [ ] Add a top status/button in lock and home with battery percentage and charging indication; route tap to Quick Controls.
- [ ] Add whole-screen lock press/pressing/release tracking, a visible upward-progress indicator, and an unlock threshold; keep a small tap on the battery button as Quick Controls, not unlock.
- [ ] Add simulator checks for Quick Controls open/close preserving the covered screen and for incomplete/complete lock swipes; run them red before adding the overlay and unlock handlers.
- [ ] Keep the existing expression rotation and time/date updates; ensure incomplete swipes reset progress and PWR behavior is unchanged.
- [ ] Implement the full-screen Quick Controls overlay with battery/charging/estimate, brightness slider, sound toggle, auto-rotate toggle, and time-sync state; no sync-now action.
- [ ] Close the overlay by close button or return gesture; it must reveal the same lock/home state without screen recreation or application transition.
- [ ] Add simulator screenshot capture for lock, Quick Controls over lock, and Quick Controls over home; expected: no overlapping or clipped controls.
- [ ] Run `python3 sim/build.py` and `./tools/regress.sh`; expected: strict compile succeeds and no existing app/lifecycle checks regress.

### Task 5: Handover, firmware build, install, and device smoke check

**Files:**
- Modify: `README.md`
- Modify: `agent-badge/README.md`
- Modify: `agent-badge/INTEGRATION.md`
- Update: `docs/design/history/2026-11-12-wear-interaction-proposal.md`

**Interfaces:**
- Consumes: completed Tasks 1–4; no firmware API changes.
- Produces: documentation matching the shipped one-app carousel and gestures, simulator screenshots, and device verification results.

- [ ] Replace the outdated Tools/Play and bottom-handle instructions with carousel, battery/Quick Controls, and anywhere-up unlock behavior.
- [ ] Review screenshots for all cards, lock, Quick Controls, Codex, Meet recording, and return flow; expected: no truncation, clipped tap target, stale timer, or broken round safe area.
- [ ] Run `python3 sim/build.py`, `./tools/regress.sh`, and `./tools/dev.sh build`; expected: simulator, source regressions, and ESP-IDF firmware all build successfully.
- [ ] Flash with `./tools/dev.sh flash <detected-serial-port>` and capture boot logs with `./tools/dev.sh monitor <detected-serial-port>`.
- [ ] On the ESP32-S3-Touch-AMOLED-1.75C, manually verify PWR wake, partial/complete unlock, carousel swipes/taps/wraparound, battery Quick Controls, Settings, Codex online/offline snapshot, Meet ready/recording, and return to the prior live app.
- [ ] Record any device-only limitations or unavailable hardware verification in the final handover; do not call installation complete without a successful flash and smoke check.

## Plan Self-Review

- **Spec coverage:** Tasks 1–5 cover the selected carousel, dynamic status cards, icon redraw, natural-water background, safe-area legibility, battery/Settings separation, unlock progress, Quick Controls, app preservation, README updates, and device checks.
- **Step scan:** Every step produces an asset, API, screen behavior, doc update, or named check; no implementation step is left as an open-ended “polish” task.
- **Type consistency:** The `badge_home_status_t` snapshot is shared by app callbacks and the launcher; `launcher_home_move()` is used by both touch navigation and simulator captures; Quick Controls uses Settings helpers rather than a second state store.
- **Review Focus:** All five risks are attached to simulator screenshot, lifecycle, offline-cache, or device smoke checks above.
- **Proportion:** The plan groups work by reviewable screen/component boundaries and remains shorter than the design specification.
