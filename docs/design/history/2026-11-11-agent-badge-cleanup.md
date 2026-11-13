# ESP Badge app refresh — Design

> Superseded update (2026-11-11): Pocket Tank and Tilt-a-tron were removed
> after device testing, and the 8 MB model reservation was reverted. VibeBuddy
> is now a fourth page inside Codex; it is not the lock screen. The lock screen
> separately cycles Grok Bot-inspired expressions and shows the clock. The
> current inventory is documented in `agent-badge/README.md`.

## Intent

Make the 1.75C badge desktop useful and readable after removing Air Mouse, Trackpad, Sand, and Keys, then add Pocket Tank, Tilt-a-tron, and adapted Waveshare demos. Give every app tile a distinct, consistent icon and review each retained screen for clear controls, visible state, and safe resource cleanup.

## Goals

- Keep Clock, Calc, Codex, Meet, Water, Games, Orbit, and Settings.
- Add Pocket Tank, Tilt-a-tron, and Waveshare examples as first-class launcher apps.
- Show ten apps in two balanced five-item pages on the 466×466 circular display.
- Remove the four selected apps and their app-only code, assets, launcher entries, and documentation.
- Keep Codex Activity and the existing Meet recorder behavior intact.
- Create matching, app-specific icons for every launcher app.
- Check each app's main flow, empty/error state, touch targets, timer lifecycle, and shared radio/mic/IMU/memory use.
- Update the README and third-party notices to match the firmware.

## Non-goals

- Rebuild existing app functionality that is unrelated to usability or stability.
- Flash firmware or model data automatically during build.
- Copy a separate upstream firmware image or its launcher wholesale.
- Add a full-featured plugin/game installation system; selected games are compiled in.

## Approaches

1. **Integrate selected app cores into the existing launcher — selected.** Reuse Pocket Tank's platform-neutral tank simulation and Tilt-a-tron's four MIT mini-games behind small LVGL adapters. Recreate the two Waveshare examples as native modes using this firmware's IMU and exclusive microphone APIs. This keeps the current app lifecycle and hardware ownership model.
2. **Port full upstream firmware projects.** This duplicates display, input, and storage owners and risks incompatible board configuration.
3. **Link to external companion apps.** This leaves the requested functionality off the badge and does not meet the confirmed request.

## Accepted desktop and interaction design

- Tools: Clock, Calc, Codex, Meet, Waveshare.
- Play: Water, Games, Orbit, Pocket Tank, Tilt-a-tron.
- Use the existing dark circular background, two-page swipe navigation, Settings hit target, and read-only battery capsule. Keep all tile labels and page indicators inside the round-screen safe area.
- Keep each existing app's established visual identity while making controls and error states consistent and readable; use gold and magenta as restrained shared accents.
- Pocket Tank uses the upstream fish simulation, a visible feed action, and its local model when the separately installed model partition is valid. If the model is absent, use the tank's own fallback behavior and state that clearly.
- Tilt-a-tron contains Breakout, Echo, PinDrop, and Tilt-a-tris. Games use shared touch/IMU input, sound feedback, per-game score storage, a pause menu, and graceful fallback if its PSRAM polar tables cannot be allocated.
- Waveshare contains an IMU-reactive visual mode and a 24-band microphone spectrum mode. Spectrum claims the existing microphone exclusively and reports when Meet recording owns it.

## Constraints and risks

- Preserve ESP-IDF, LVGL, screen dimensions, settings, Codex Activity transport, Meet recording, and the existing app lifecycle.
- Pocket Tank's 7,557,640-byte model must live in a separate 8 MiB flash partition. The current recording partition becomes 16 MiB to make room; firmware build must not write model data or erase existing recordings.
- Model installation is a separate, explicit operation. The UI must remain usable without it.
- Spectrum and Meet share one I2S microphone and must never open it concurrently.
- Tilt-a-tron's 466×466 polar tables require about 870 KiB PSRAM; Echo and PinDrop remain selectable when allocation fails.
- App leave callbacks must stop timers and release resources they own. Long-lived recording remains owned by Meet's recorder service.
- Preserve upstream attribution and licenses, including Pocket Tank's PolyForm Noncommercial terms.

## Implementation ownership

- `launcher.c` and `app.h`: two-page membership, tile layout, app registrations, and shared touch behavior.
- `apps/app_pocket_tank.c` and its adapter: tank lifecycle, rendering, feed action, model readiness.
- `apps/app_tilt_a_tron.c`: game API adapter, menu, input, palette canvas, and resource lifecycle.
- `apps/app_waveshare.c`: IMU and microphone spectrum modes.
- `tools/mkassets.py` and `main/assets/`: generated shared app tile illustrations.
- `CMakeLists.txt`, partition table, simulator, and README/third-party docs: build, resource, and operating instructions.
