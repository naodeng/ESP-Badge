# MOTO GPS Map Visual Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Improve day and night readability of the existing Jarvis_OS circular navigation map without changing its map data or BLE protocol.

**Architecture:** Keep the current OSM/Protomaps road and building scene. Apply a day/night palette in the shared LVGL navigation UI, selected from the device local clock and refreshed by a lightweight LVGL timer. Record MotoNav as a visual/technical reference in `THIRD_PARTY.md`; do not copy its code or map assets.

**Tech Stack:** ESP-IDF C++17, LVGL, shared Moto GPS navigation UI, Markdown.

**Spec:** [`docs/superpowers/specs/2026-11-16-moto-gps-map-visual-design.md`](../specs/2026-11-16-moto-gps-map-visual-design.md)

## Global Constraints

- Preserve the 466 × 466 round display and current navigation-page layout.
- Preserve OSM/Protomaps input data, offline map queries, MapScene BLE schema, and current route projection.
- Use day palette from 06:00 through 17:59 local time; use night palette from 18:00 through 05:59.
- Use the night palette whenever the local clock is not calibrated.
- Do not add map features, new providers, BLE fields, or Android components.
- Keep the route white with a graphite halo and the vehicle marker red.
- Attribute MotoNav in `THIRD_PARTY.md`; do not copy its source, screenshots, map tiles, or other assets.
- Do not add or run tests in this implementation turn.

## Review Focus

- Invalid RTC/system time: must choose night palette, not interpret epoch zero as daytime.
- 05:59 / 06:00 / 17:59 / 18:00 boundaries: selector must return the documented palette; the visible change occurs on the next 60-second check.
- Navigation paused or stationary: palette must still change while the navigation page remains open.
- Route and vehicle marker contrast: both must stay visible on the light and dark map backgrounds.
- Timer lifecycle: the theme timer must not remain active after the UI screen is cleared for power-off.

---

### Task 1: Add the local-time map palette

**Files:**
- Modify: `packages/moto-gps/shared/nav_ui/src/moto_nav_ui.cpp`

**Interfaces:**
- Consumes: existing `ui.nav_map`, road/building polylines, route/shadow objects, and the device's POSIX local clock.
- Produces: a private palette selector and applier in the same translation unit; no public UI or BLE interface changes.

- [x] Add day/night color constants and a private palette enum. Keep all selection logic inside `moto_nav_ui.cpp`.
- [x] Add a private selector that uses `localtime_r`; return night if `time()` fails or `tm_year < 124`; otherwise select day for hours 6–17 and night for hours 18–5.
- [x] Add a palette applier that updates the map viewport background, road class colors/widths, and building outline colors. Preserve the route's white centerline, graphite halo, and red marker in both palettes.
- [x] Add a 60-second LVGL timer to re-check the palette while navigation is stationary; apply styles only when the selected palette changes.
- [x] Delete and null the palette timer before `reset_ui_state()` clears the `Ui` aggregate, and in `moto_nav_ui_show_power_off_screen()`; verify the timer callback cannot touch cleared LVGL objects.
- [x] Inspect the diff for unchanged protocol types, capacities, map data, page layout, and motion interpolation.

### Task 2: Record the MotoNav reference

**Files:**
- Modify: `THIRD_PARTY.md`
- Reference: `docs/design/mockups/moto-gps-day-night-map-concept.png`

**Interfaces:**
- Consumes: the user-approved visual concept and the published MotoNav repository/license.
- Produces: a concise source row stating that the project informed independent map visual hierarchy only and that no MotoNav code, assets, screenshots, or tiles were reused.

- [x] Add a MotoNav reference row to `THIRD_PARTY.md` with the repository URL and a plain-language scope statement.
- [x] Keep existing OSM/Protomaps ODbL attribution unchanged; do not add AMap/TDT data or SDK claims.
- [x] Review the attribution wording so it does not imply that MotoNav authored this repository's implementation or supplied its map data.

### Task 3: Review the completed change

**Files:**
- Review: `packages/moto-gps/shared/nav_ui/src/moto_nav_ui.cpp`
- Review: `THIRD_PARTY.md`
- Review: `docs/design/mockups/moto-gps-day-night-map-concept.png`

- [x] Compare the palette and element hierarchy against the approved concept image and design spec.
- [x] Inspect timer creation and teardown against the existing LVGL timer lifecycle.
- [x] Inspect the final diff for unintended edits and whitespace errors. No automated tests or test commands are part of this plan.
