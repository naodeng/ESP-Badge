# Jarvis_OS × soRound OS Integration Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the old `agent-badge` firmware base with the full soRound OS project under `Jarvis_OS/`, then add the existing Codex feature and random expression clock in soRound OS's native UI.

**Architecture:** Use the authorized upstream snapshot `soBigRice/soRound_os` commit `1d388b5` as the project base. Keep its apps, 62 existing watchfaces, audio recorder, storage layout and UI lifecycle intact. Add Codex as an `app_t` plus a data/client module; append the expression clock as watchface index 62. Move the old firmware directory aside only after the new tree builds, then update workspace entry points and documentation.

**Tech Stack:** ESP-IDF, LVGL 9, ESP32-S3-Touch-AMOLED-1.75C, CMake/CTest, FreeRTOS, existing Python Codex bridge.

**Design records:** `docs/design/history/2026-11-12-soround-ui-adaptation.md`; detailed UI contracts: `docs/design/history/2026-11-12-codex-native-ui.md` and `docs/design/history/2026-11-12-expression-watchface.md`.

## Global Constraints

- Preserve all 26 upstream apps, including sound capsules recording, playback and deletion.
- Preserve every existing watchface ID 0–61; add Codex as launcher app 27 and Jarvis face as watchface 62.
- Keep the device target at ESP32-S3-Touch-AMOLED-1.75C, 32 MB flash, 8 MB PSRAM and the upstream dual 8 MiB OTA plus storage/capsules partitions.
- The user authorized clearing old recordings for the partition migration; preserve the existing NVS credentials where the partition layout permits.
- Use soRound OS black, warm white, gray, red accent, round-screen safe areas and native app/watchface/power lifecycles.
- Codex usage is read-only and comes from the current bridge schema; preserve UDP `AGENTBADGE/1` auto-discovery and never use a hard-coded Mac IP.
- Poll Codex only while its app is visible; retain the last valid snapshot through offline and parse-error states.
- User confirmed upstream source reuse and modification authorization on 2026-11-12. Do not publish or redistribute unless that authorization covers redistribution.
- Build and test the candidate under `/private/tmp` or the new `Jarvis_OS/` tree; do not overwrite `agent-badge` until the new tree is reviewable.

## Review Focus

- Missing/null usage windows must render as unavailable and remain non-interactive — Task 2 parser tests and Task 3 UI input test.
- Malformed/failed bridge responses must preserve the last valid snapshot and show offline/stale state — Task 2 parser/client tests.
- All 16 Radar rows must fit inside the 466×466 round safe area without truncation — Task 3 captured UI test.
- Reopening the expression face or waking from full sleep must change the expression without changing the selected face; AOD transitions must not randomize it — Task 4 lifecycle test.
- Repartitioning must retain voice capsule support while clearing the user's old recording region — Task 1 partition assertion and Task 6 device verification.

---

### Task 1: Stage and baseline the complete upstream project

**Files:**
- Create: `Jarvis_OS/**` from the authorized upstream repository archive, without nested `.git` metadata.
- Create: `Jarvis_OS/UPSTREAM.md` with source URL, commit, scope and the user-confirmed reuse/modification authorization record.
- Modify: `Jarvis_OS/GeekTool-IDF/CMakeLists.txt` to use project name `Jarvis_OS`.
- Modify: nested upstream `.gitignore` and workflow paths for the enclosing `Jarvis_OS/` directory.

**Interfaces:**
- Produces the full upstream app registry (26 apps), watchface catalog (62 entries) and `partitions.csv` as the migration baseline.
- Does not import any old `agent-badge` app except the Codex bridge/function and expression clock listed in later tasks.

- [ ] Export the upstream commit into `Jarvis_OS/`, retaining firmware, source docs, web companion and project tooling while excluding `.git`.
- [ ] Confirm registry still contains `app_capsules`, every upstream app and all six companion icons; confirm `partitions.csv` still defines `ota_0`, `ota_1`, `storage` and `capsules` at upstream offsets.
- [ ] Run ESP-IDF `reconfigure` to resolve `idf_component.yml` and generate the ignored `managed_components/` tree; record the exact toolchain version (the workspace currently has ESP-IDF 5.5.4).
- [ ] Run upstream host test configure/build/CTest against the resolved LVGL and cJSON dependencies; record any baseline failure before feature work.
- [ ] Run a candidate ESP-IDF build before feature additions; record image size and confirm it fits the 8 MiB slot.

### Task 2: Add a tested Codex snapshot and bridge client

**Files:**
- Create: `Jarvis_OS/GeekTool-IDF/main/codex_data.h`
- Create: `Jarvis_OS/GeekTool-IDF/main/codex_data.c`
- Create: `Jarvis_OS/GeekTool-IDF/main/codex_client.h`
- Create: `Jarvis_OS/GeekTool-IDF/main/codex_client.c`
- Create: `Jarvis_OS/GeekTool-IDF/tests/codex/codex_data_tests.c`
- Create: `Jarvis_OS/GeekTool-IDF/tests/codex/CMakeLists.txt`
- Modify: `Jarvis_OS/GeekTool-IDF/main/CMakeLists.txt`
- Modify: `Jarvis_OS/GeekTool-IDF/tests/host/CMakeLists.txt`

**Interfaces:**
- `codex_snapshot_t` carries `p5_percent`, `p7_percent` (−1 when absent), `reset_s`, `tokens`, `cost_c`, `plan`, `activity_state`, `active_tasks`, sync/radar timestamps, stale/online/has-snapshot flags, and at most 16 radar rows (`model`, `epoch`, `iq`).
- `bool codex_snapshot_parse(const char *json, size_t len, codex_snapshot_t *out)` parses into a temporary value and only replaces `out` for a valid JSON object; null usage windows remain unavailable.
- `void codex_snapshot_mark_offline(codex_snapshot_t *snapshot, const char *reason)` changes connectivity/error state without clearing cached usage/activity/radar.
- Client API: `codex_client_start()`, `codex_client_set_visible(bool)`, `codex_client_request_refresh()`, and `codex_client_snapshot(codex_snapshot_t *out)`.
- Client performs UDP auto-discovery and HTTP in a worker; `app_t` ticks read a copied snapshot only. It polls at 10-second intervals while visible and pauses when hidden.

- [ ] **Step 1: Write parser/client contract tests first.** Assert representative bridge JSON, `p5: null`, missing optional values, malformed JSON preserving the previous snapshot, radar truncation at 16, offline cache retention and stale-after-60-seconds behavior.
- [ ] **Step 2: Run the new Codex data test and verify it fails** because the snapshot API is not implemented.
- [ ] **Step 3: Implement the pure snapshot parser and transactional cache update** in `codex_data.c`.
- [ ] **Step 4: Run the new tests and verify they pass.**
- [ ] **Step 5: Implement the visible-only worker** using upstream `wifi_service_ready()` and `network_http_get()`. Send `AGENTBADGE/1` to UDP broadcast port 8790; on no discovery show offline instead of falling back to a fixed IP.
- [ ] **Step 6: Run the Codex data/client tests and upstream networking tests.**

### Task 3: Add the Codex app in native soRound OS UI

**Files:**
- Create: `Jarvis_OS/GeekTool-IDF/main/app_codex.c`
- Create: `Jarvis_OS/GeekTool-IDF/main/codex_ui_tests.c`
- Modify: `Jarvis_OS/GeekTool-IDF/main/app.h`
- Modify: `Jarvis_OS/GeekTool-IDF/main/launcher.c`
- Modify: `Jarvis_OS/GeekTool-IDF/main/companion_icons.h`
- Modify: `Jarvis_OS/GeekTool-IDF/main/companion_icons.c` / `.inc`
- Modify: `Jarvis_OS/GeekTool-IDF/main/CMakeLists.txt` and `tests/host/CMakeLists.txt` to register Codex source and UI test target.

**Interfaces:**
- `const app_t app_codex` owns Usage, Radar, Activity and VibeBuddy pages; `.visibility` pauses/restarts the Codex client and `.exit` stops visible polling.
- Append `app_codex` after existing launcher entries and append one matching monochrome Codex icon; existing app/icon indices remain unchanged.
- Use `launcher_set_title()`, `tools_ui.h` drawing/text helpers and existing app back/touch conventions.

- [ ] **Step 1: Write UI tests first.** Enter the app with a fixed snapshot; assert four pages, all 16 Radar rows in the safe area, and no clickable quota arcs.
- [ ] **Step 2: Run the new UI tests and verify they fail** because Codex is absent from the app registry.
- [ ] **Step 3: Implement the four pages** with black background, warm white/gray hierarchy, one red accent, native header/back and small local page transitions. Keep usage read-only; show explicit loading/offline/stale/error states; keep VibeBuddy inside Codex.
- [ ] **Step 4: Run the UI tests and verify they pass**, including a simulated drag over quota arcs that leaves values unchanged.
- [ ] **Step 5: Render 466×466 screenshots for every Codex page** and inspect the last Radar row, long labels, empty state, stale cache and VibeBuddy art for clipping/overlap.

### Task 4: Append the random expression clock as a native watchface

**Files:**
- Create: `Jarvis_OS/GeekTool-IDF/main/watchface_grok.c`
- Create: `Jarvis_OS/GeekTool-IDF/main/watchface_grok.h`
- Modify: `Jarvis_OS/GeekTool-IDF/main/watchface_ui.h`
- Modify: `Jarvis_OS/GeekTool-IDF/main/watchface_catalog.c`
- Modify: `Jarvis_OS/GeekTool-IDF/main/watchface_ui.c`
- Modify: `Jarvis_OS/GeekTool-IDF/main/watchface.c`
- Modify: `Jarvis_OS/GeekTool-IDF/main/watchface_picker.c`
- Modify: `Jarvis_OS/GeekTool-IDF/main/CMakeLists.txt`
- Modify: `Jarvis_OS/GeekTool-IDF/tests/watchfaces/watchface_tests.c`

**Interfaces:**
- Add `WATCHFACE_JARVIS_INDEX = 62`; increase count to 63 and theme count to 6; append `JARVIS / face` and theme `JARVIS` without moving 0–61.
- `uint8_t watchface_grok_next_expression(uint8_t previous, uint32_t random_value)` returns one of 13 valid expressions and never repeats `previous`.
- `watchface_grok_render(layer, area, data, expression, preview)` draws expression, time, date and battery using upstream white/gray/red and scales from 466px to the existing 233px preview.
- `watchface_show()` and transition from sleep to awake select a new expression; AOD/full-brightness transitions do not.

- [ ] **Step 1: Extend watchface tests first** for 63 entries, stable names/themes at old IDs, new ID 62, six picker targets, and expression bounds/non-repeat.
- [ ] **Step 2: Run the focused watchface test and verify it fails** on the current 62-entry catalog and missing expression API.
- [ ] **Step 3: Implement catalog/picker append and the pure expression selector.**
- [ ] **Step 4: Re-run the focused test and verify it passes.**
- [ ] **Step 5: Add the scaled vector renderer and lock/wake expression selection; keep preview expression deterministic and use existing time/battery snapshot.**
- [ ] **Step 6: Render the new full-size and preview face at midnight, end-of-day, low battery, charging and AOD; verify no clipping.**
- [ ] **Step 7: Run all watchface and persistence tests to prove prior indices remain stable.**

### Task 5: Rename the project and refresh entry-point docs

**Files:**
- Create/Modify: `Jarvis_OS/README.md`, `Jarvis_OS/UPSTREAM.md`, bridge README and scripts.
- Modify: workspace `README.md`, `PRODUCT.md`, `THIRD_PARTY.md`, `.gitignore` and existing build/install path references.
- Modify: `Jarvis_OS/.github/workflows/**` and repo-relative site/OTA scripts for the nested project path and new `Jarvis_OS.bin` build output; keep the existing OTA object URL compatible.
- Remove from workspace: old `agent-badge/` after moving it to a recoverable `/private/tmp` backup.

**Interfaces:**
- Root README links to the new `Jarvis_OS/` project and documents 27 apps, 63 watchfaces, Codex bridge, expression face, recording storage and build/flash commands.
- The Codex bridge install/start commands use paths under `Jarvis_OS/` and preserve the current port 8790 protocol.
- Do not alter the outer Git repository directory name; the requested firmware project directory becomes `Jarvis_OS/`.

- [ ] Update docs/scripts/CI and confirm no active instructions refer to `agent-badge/`.
- [ ] Check README inventory against launcher app count, watchface count and partition table.
- [ ] Move the old folder to the temporary backup only after Tasks 1–4 build and tests pass; verify the old uncommitted work remains recoverable.
- [ ] Review root and firmware README build, bridge, flash and recording instructions end-to-end.

### Task 6: Full verification and device installation

**Files:**
- No new product files; evidence comes from host suites, firmware build and device logs.

**Interfaces:**
- Candidate flash uses `/dev/cu.usbmodem1101` if still present; partition migration may erase old recordings as authorized. Preserve NVS/Wi-Fi credentials if possible.

- [ ] Configure/build and run the complete upstream host + Codex + watchface suites; expected: 0 failed tests.
- [ ] Build the full ESP-IDF firmware and confirm image fits the 8 MiB OTA slot.
- [ ] Flash firmware and partition table to the watch; clear only the old/new recording data region required for the new capsules filesystem, not the entire NVS by default.
- [ ] Capture boot logs through self-test and confirm launcher starts without panic/rollback.
- [ ] On-device smoke-check existing launcher/apps, Codex auto-discovery and read-only Usage, expression watchface selection/wake, and sound capsule record/play/delete; report any interaction that cannot be physically exercised from the host.
- [ ] Run `git diff --check`; inspect final path inventory and the new partition/table/app registry against the spec before claiming completion.
