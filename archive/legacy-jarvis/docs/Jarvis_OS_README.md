# Jarvis_OS

Jarvis_OS adapts selected soRound_os modules for the Jarvis product scope and adds a Codex companion app, an expression clock and a phone-connected Travel speedometer. The default launcher contains 18 apps; ten low-priority apps have been withdrawn from the default build while their source remains available for later packaging decisions.

## Hardware and firmware

- Waveshare ESP32-S3-Touch-AMOLED-1.75C
- ESP32-S3R8, 32 MB flash, 8 MB PSRAM
- 466 × 466 round AMOLED
- Main firmware: [`GeekTool-IDF/`](GeekTool-IDF/)

## Jarvis additions

- **Codex**: read-only Usage, Radar, Activity, and VibeBuddy pages. The device discovers the Mac bridge on the local network; usage is never entered with a slider.
- **Travel**: phone-provided speed, moving heading and accuracy over BLE. It is currently a speedometer view, not a route-navigation app.
- **Removed from the default build**: Mouse, Twin, Level, Maze, Fluid, Answers, Zodiac, Merit, Pixels and Stocks. This also disables the BLE mouse/media services and Now Playing bridge integration; local audio, Focus soundscapes, recording and playback remain.
- **Expression clock**: 13 vector expressions with time, date, and battery status. A new expression is selected when the display wakes from full sleep.
- **UI adaptation**: new pages use soRound_os typography, black backgrounds, white/gray content, red status accents, and round-screen safe areas.

Start the Mac bridge from the repository root with `./Jarvis_OS/tools/run_codex_bridge.sh`. Build and flash commands, app inventory, partition details, and test commands are in the [root README](../README.md).

The original upstream project guide and its historical notes are preserved in [`UPSTREAM_README.md`](UPSTREAM_README.md); current Jarvis commands are maintained in the root README.
