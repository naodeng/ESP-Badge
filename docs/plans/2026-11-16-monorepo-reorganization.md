# Jarvis_OS MOTO GPS Integration Implementation Plan

> **For agentic workers:** Implement the checked tasks in order. Existing user changes are part of the input and must be preserved or intentionally replaced by the new product scope. Do not commit unless asked.

**Goal:** Deliver one Jarvis_OS firmware compatible with all MOTO GPS iOS device-facing features, together with the iOS app, Cloudflare/Node gateway, debug tools, and current project documentation.

**Architecture:** Keep the ESP-IDF firmware, iOS client, gateway, and portable navigation/protocol code in separate domain directories. Phone and gateway own location, search, route calculation, traffic, and map downloads; Jarvis_OS owns the round-display, BLE protocol endpoint, navigation rendering, sensors, and device commands.

**Tech Stack:** ESP-IDF 5.5.x, LVGL, NimBLE, C/C++, Swift/XcodeGen, Node.js, Cloudflare Workers/R2, CMake, Python.

**Spec:** `docs/design/2026-11-16-main-repo-monorepo-design.md`

## Global Constraints

- Project and firmware name are exactly `Jarvis_OS`; produce one formal firmware artifact, without stable/beta or OTA channels.
- Keep only the Jarvis core and MOTO GPS navigation product in the formal build. Keep two web debugging tools under `tools/debug/` and never publish them as product websites.
- Do not include Android, the upstream MOTO ESP32 firmware target, public marketing websites, or the old standalone Travel BLE protocol in the product.
- Keep MOTO GPS AMap key as Cloudflare Secret; do not commit it or local authentication material.
- Keep the current partition table, NVS format, and recording storage compatible; never erase user data during firmware installation.
- Preserve MOTO GPS PolyForm license, NOTICE, third-party notices, OSM attribution, and Jarvis/soRound provenance.
- Preserve user-authored, uncommitted changes except where the user explicitly replaced the product direction.

## Review Focus

- BLE service UUID, secure handshake, fragmentation, CRC, ACK ordering, and reconnect recovery match the iOS app exactly.
- Every MOTO GPS device-facing message has a Jarvis view/state/command implementation, including navigation, map scene, and media controls.
- App, Node service, Cloudflare Worker, and native build files resolve the relocated `packages/moto-gps/shared` paths.
- Removed OTA/site/extra-app workflows cannot produce or publish a second firmware or an unwanted website.
- Firmware install preserves partitions, NVS, settings, and recording data.

---

### Task 1: Establish the lean Jarvis_OS firmware tree

**Files:**
- Move: `Jarvis_OS/GeekTool-IDF/` → `firmware/jarvis_os/`
- Move: legacy non-navigation apps/tools → `archive/legacy-jarvis/`
- Modify: firmware app registry, build source list, ESP-IDF project name, release workflow, ignore rules

- [ ] Move tracked and unignored files while leaving `.toolchain`, `build`, `managed_components`, `sdkconfig`, and generated local state untouched.
- [ ] Keep navigation, boot/idle, brightness/settings, BLE, touch/button, display, and required sensor support.
- [ ] Remove OTA and unrelated app entries/sources from the formal build; archive recoverable legacy sources.
- [ ] Replace the standalone Travel BLE UUID/frame with the MOTO GPS protocol implementation in the navigation app.
- [ ] Change release metadata to the exact `Jarvis_OS` project/artifact name and one release binary.

### Task 2: Import MOTO GPS iOS, gateway, shared modules, and debug tools

**Files:**
- Move: `platforms/ios/` → `apps/ios/moto-gps/`
- Move: `backend/` → `services/moto-gps-gateway/`
- Move: required `shared/` → `packages/moto-gps/shared/`
- Move: current Jarvis GeekTwin and MOTO GPS WASM shell → `tools/debug/`
- Add: `licenses/moto-gps/` and upstream provenance

- [ ] Import only iOS, Node/Cloudflare gateway, shared protocol/navigation/map code, and the two requested debug tools.
- [ ] Exclude Android, MOTO's separate ESP-IDF target, hardware candidate packages, and product websites.
- [ ] Update XcodeGen, Gradle/NDK references where retained, Node tests/fixtures, Wrangler, and simulator paths.
- [ ] Configure the iOS app to use the deployed gateway by default while keeping Gateway Settings override support.
- [ ] Preserve the deployed Worker configuration, R2 bucket binding, and Cron schedule without copying Secret values.

### Task 3: Adapt Jarvis BLE and navigation UI

**Files:**
- Create/modify: `firmware/jarvis_os/main` MOTO GPS BLE service, protocol adapter, navigation app, and LVGL renderer
- Modify: host/native fixtures and app registry
- Use: `packages/moto-gps/shared/ble_protocol`, `nav_core`, `nav_app`, `nav_presenter`, `nav_ui`, `protocol`, `demo_fixture`, and offline-map data

- [ ] Match UUIDs, secure handshake, fragment/CRC/ACK rules and message sequencing to the iOS source.
- [ ] Render the full device-facing MOTO GPS state: turn cue, route progress, route/traffic status, speed/course, map scene, connection and app commands.
- [ ] Maintain Jarvis `app_t` visibility/tick/exit cleanup and round-screen safe-area rules.
- [ ] Remove the obsolete Travel-only GATT service and update its tests/fixtures to the MOTO GPS protocol.

### Task 4: Rebuild project documentation

**Files:**
- Modify: `AGENTS.md`, `README.md`, `PRODUCT.md`, `design.md`, `THIRD_PARTY.md`, `docs/README.md`
- Create: `docs/architecture/system-overview.md`, `docs/development/README.md`, `docs/deployment/README.md`, `docs/debugging/README.md`, `docs/process/README.md`, `docs/project-status.md`

- [ ] Document the single Jarvis_OS firmware, MOTO GPS iOS app, deployed gateway, two local debug tools, and current verification status.
- [ ] Add build/install/debug steps for firmware, iOS, Node gateway, Cloudflare Worker/R2, and local simulators.
- [ ] Remove stale website, Android, dual-firmware, OTA, and standalone Travel BLE product claims.
- [ ] Keep product release and device validation status distinct from source/build readiness.

### Task 5: Package and install for debugging

- [ ] Build exactly one formal Jarvis_OS firmware artifact.
- [ ] Build a MOTO GPS iOS simulator/app package using the installed Xcode toolchain and available signing setup.
- [ ] Check partition-table and NVS compatibility before flashing; back up device data first if installation would affect user data.
- [ ] Install and exercise pairing, navigation, map display, route updates, background reconnect, and media commands on available hardware/simulator.
- [ ] Record which checks were run and which require physical-device/network conditions.
