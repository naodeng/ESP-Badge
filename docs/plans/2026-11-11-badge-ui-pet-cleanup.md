# ESP Badge app refresh Implementation Plan

> Historical plan, superseded on 2026-11-11: Pocket Tank and Tilt-a-tron were
> removed after the user tested the device. The current firmware keeps the
> Waveshare examples, restores the 24 MB recording partition, adds VibeBuddy
> as Codex page four, and keeps the Grok expression clock on the lock screen.
> See `agent-badge/README.md` for the current implementation and validation.

> **For agentic workers:** Use inline implementation for the existing project. Steps use checkbox syntax for tracking.

**Goal:** Remove the selected apps, add Pocket Tank, Tilt-a-tron, and Waveshare demos, and polish and verify the app set on the 1.75C badge.

**Architecture:** Keep the ESP-IDF/LVGL app lifecycle. Put hardware access behind `port.h`, adapt each upstream core to this firmware, and generate all tile art from one asset pipeline.

**Tech Stack:** ESP-IDF, LVGL, C, Python asset/build tools, existing PC simulator.

**Design record:** `docs/design/history/2026-11-11-agent-badge-cleanup.md`

## Global Constraints

- Keep screen geometry at 466×466 and preserve Codex Activity and Meet recorder behavior.
- The desktop has exactly two five-app pages: Tools and Play, as specified.
- Meet recording and Waveshare spectrum never own the I2S microphone simultaneously.
- Pocket Tank works without a model; the build never writes the model partition.
- Preserve upstream license notices; Pocket Tank is PolyForm Noncommercial 1.0.
- Do not flash the device or model as part of build verification.

## Review Focus

- Launcher arrays, page counts, labels, and all icon pointers match exactly ten app descriptors.
- A game started and exited repeatedly releases its canvas/timers; polar-table failure leaves non-polar games usable.
- Spectrum start fails cleanly while Meet recording is active, and leaving spectrum releases the microphone.
- Missing or invalid model uses the fallback safely; no code interprets arbitrary model bytes as valid.
- Every generated icon builds without screenshot fixtures and each visible tile has distinct artwork.

---

### Task 1: Set the final desktop and remove selected apps

**Files:** `main/app.h`, `main/launcher.c`, `main/apps/app_settings.c`, `main/CMakeLists.txt`, `main/apps/`, `main/assets/`, `main/sand/`, `main/main.c`.

- [ ] Set Tools to Clock, Calc, Codex, Meet, Waveshare; Play to Water, Games, Orbit, Pocket Tank, Tilt-a-tron.
- [ ] Fix five-tile positions, labels, and page indicator bounds for the circular display; retain swipe suppression and Settings/battery behavior.
- [ ] Remove Air Mouse/Trackpad, Sand, and Keys from launcher and delete their app-only screens, helpers, assets, and import route while retaining shared Meet BLE and recorder services.
- [ ] Update source generation to include nested third-party and app sources needed by the new apps.

### Task 2: Integrate the three selected app projects

**Files:** create `main/apps/app_pocket_tank.c`, `main/apps/pocket_tank_adapter.c`, `main/apps/app_tilt_a_tron.c`, `main/apps/app_waveshare.c`; update `main/port.h`, `main/rec_store.c`, `sim/port_sim.c`, `sim/build.py`, `partitions.csv`, upstream sources and notices.

- [ ] Build Pocket Tank on the upstream round-board simulation, with one feed control, real-time simulation only while visible, model readiness, and a stable fallback.
- [ ] Keep model data in an 8 MiB partition after a 16 MiB recording partition; validate tag/length/SHA before enabling inference and provide a separate manual install path.
- [ ] Host four Tilt-a-tron games behind a shared palette canvas, round-safe pause menu, touch/tilt adapter, per-game saves, and PSRAM-backed polar tables where available.
- [ ] Adapt the Waveshare IMU demo and microphone spectrum demo into two modes. Spectrum uses the microphone ownership API and handles busy/unavailable state.
- [ ] Add simulator stubs for hardware-owned IMU and spectrum data without linking ESP-only model tasks.

### Task 3: Unify all app icons and review every app flow

**Files:** `tools/mkassets.py`, `main/assets/assets.h`, generated icon sources, and each retained/new app source.

- [ ] Generate unique clock, calculator, Codex, Meet, Water, Games, Orbit, Pocket Tank, Tilt-a-tron, Waveshare, and Settings artwork with shared dark/gold/magenta visual language.
- [ ] Ensure the generator does not rely on unavailable screen captures and the launcher has a valid artwork fallback.
- [ ] Review Clock (timer/alarm/stopwatch), Calc, Codex, Meet, Games, Water, Orbit, Pocket Tank, Tilt-a-tron, Waveshare, and Settings for touch feedback, readable state, safe cleanup, and sensible unavailable/busy states.
- [ ] Keep bespoke game and Codex visualizations intact; make focused fixes only where the review finds an actual usability or stability issue.

### Task 4: Update docs and verify the build

**Files:** root and firmware README, integration/changelog/third-party notes, bridge README, CMake, simulator build files, model install helper.

- [ ] Document the new two-page app list, model installation and flash/storage implications, spectrum/microphone ownership, and upstream licenses.
- [ ] Remove stale claims for the four deleted apps and Sand import; keep the Codex Activity bridge instructions accurate.
- [ ] Run `python3 sim/build.py` and fix compile/link errors in shared code.
- [ ] Run the repository's existing read-only documentation/protocol checks where they do not write device data; inspect the simulator screens for app selection and primary interactions.
- [ ] Check `git diff --check`, search removed app names, and review the final diff for unrelated changes and resource/lifecycle defects.

## Execution notes

- The existing uncommitted tree already contains Pet/Codex Activity work and an incomplete launcher/spectrum foundation; preserve those edits.
- The ESP-IDF build wrapper and esptool were unavailable during initial inspection. Recheck during verification; report simulator-only coverage if board flashing remains unavailable.
