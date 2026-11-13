# Jarvis_OS

Jarvis_OS adapts the complete soRound_os firmware to the Jarvis product name and adds two native integrations: a Codex companion app and a Grok Bot expression clock. The original soRound_os apps, watch faces, settings, audio features, and recorder are retained.

## Hardware and firmware

- Waveshare ESP32-S3-Touch-AMOLED-1.75C
- ESP32-S3R8, 32 MB flash, 8 MB PSRAM
- 466 × 466 round AMOLED
- Main firmware: [`GeekTool-IDF/`](GeekTool-IDF/)

## Jarvis additions

- **Codex**: read-only Usage, Radar, Activity, and VibeBuddy pages. The device discovers the Mac bridge on the local network; usage is never entered with a slider.
- **Expression clock**: 13 vector expressions with time, date, and battery status. A new expression is selected when the display wakes from full sleep.
- **UI adaptation**: new pages use soRound_os typography, black backgrounds, white/gray content, red status accents, and round-screen safe areas.

Start the Mac bridge from the repository root with `./Jarvis_OS/tools/run_codex_bridge.sh`. Build and flash commands, app inventory, partition details, and test commands are in the [root README](../README.md).

The original upstream project guide and its historical notes are preserved in [`UPSTREAM_README.md`](UPSTREAM_README.md); current Jarvis commands are maintained in the root README.
