# Jarvis Wear-Inspired UI Redesign

## Intent

Redesign Jarvis’s 466 × 466 circular interface around quick status checks and simple touch navigation. The device is used both on-wrist and on a desk. The primary glance targets are time and Codex/Meet activity; the application carousel remains the path to the rest of the tools.

## Goals

- Replace the Tools/Play two-page icon grid with a horizontal carousel that focuses on one application per screen.
- Lead the carousel card with useful status for Clock, Codex, and Meet; keep the other application cards as direct, uncluttered launchers.
- Redraw every application icon and the shared status/control icons as one coherent vector set.
- Redesign unlock so an upward swipe can begin anywhere on the lock screen, shows progress while dragging, and does not depend on grabbing a narrow bottom handle.
- Move battery information out of the center capsule. Keep a small battery status control at the top; tap it or swipe down to open quick controls. Keep full Settings as an application card.
- Apply circular safe-area spacing and keep application state, recording, timers, and the existing Codex bridge behavior intact.
- Use Jarvis as the visible product name in firmware and documentation.

## Non-goals

- Port the firmware to Android or add Android/Compose dependencies. Wear OS is a design reference; the implementation stays ESP-IDF and LVGL.
- Change the behavior or data sources of Codex, Meet recording, Games, Water, Orbit, or Waveshare demos.
- Rename internal C symbols, NVS namespaces, protocol identifiers, or the existing ESP-IDF project directory.
- Bring back the purple-red route motif or yellow outer ring.

## Anti-goals

- Clipped labels, tiny edge-only controls, settings and battery bundled into an unexplained central capsule, or more than one primary action competing on a card.
- Decorative glow, color used without state meaning, or an icon set that depends on tiny detail to tell applications apart.
- Returning to the carousel after every wake when an application was active; unlock should preserve the current continuation behavior.

## Constraints

- Target: Waveshare ESP32-S3-Touch-AMOLED-1.75C, ESP32-S3R8, 32 MB Flash, 8 MB PSRAM, 466 × 466 circular AMOLED.
- Runtime: ESP-IDF 5.5.4 and LVGL 9.5. Current screen, touch, power, and application lifecycle interfaces remain in place.
- Navigation uses touch and the existing PWR button; the device has no Android system navigation, rotary crown, or Wear OS runtime.
- Existing applications: Clock, Codex (Usage, Radar, Status, VibeBuddy), Meet, Calc, Waveshare, Games, Water, Orbit, and Settings.
- Preserve the lock-screen time/date/battery and its rotation among 13 Grok-style expressions.

## Approaches

1. **Keep two app-grid pages.** Rebalance the icon grid and replace the central Settings/battery capsule. This changes the least, but keeps the home screen focused on app launch rather than glanceable status.
2. **Single-app carousel, status first (selected).** Each screen focuses on one application. Clock, Codex, and Meet show their most useful current state; remaining apps are simple launch cards. This gives the small round display a clear focal point and matches the requested Wear OS glance-first reference.
3. **Single-app carousel with adjacent-card peeks.** Show the active app and pieces of its neighbors. This exposes more navigation context but reduces the usable area for status and app names.

The selected composition is the state-first carousel. The clickable prototype also compares icon-first and adjacent-card layouts under the same interaction model.

## Interaction and information architecture

### Wake and unlock

1. PWR wakes the display into the low-power lock screen with time, date, battery, and the next expression.
2. The hint and progress indicator explain the unlock gesture. An upward drag can start anywhere on the circular screen; progress follows the finger. Releasing past the threshold unlocks; an incomplete drag returns to the lock screen.
3. If a live app was active before sleep, unlock resumes that app. Otherwise it opens the carousel at the last focused application.
4. PWR still turns the screen off. Existing idle timeout and touch-wake rules remain intact.

### Application carousel

- One focused card occupies the screen. Left/right swipes move exactly one application and update a readable position indicator; the same gesture never opens the card.
- A tap opens the selected app. The existing app handle remains the clear return path and returns to the focused card.
- Initial order: Clock, Codex, Meet, Calc, Waveshare, Games, Water, Orbit, Settings. Keep Settings last because quick controls stay available from the battery status area.
- Clock shows current time/date. Codex shows connection and activity/task state. Meet shows recording readiness or active recording state. Other cards show the custom icon, app name, and one short launch hint.
- While the screen is awake and the Codex card is selected, refresh its shared bridge snapshot every 30 seconds. Show the update time; mark a snapshot stale after 60 seconds. Keep the last known state with an explicit offline/stale label after a failed fetch. Do not poll from the lock screen or while another app is running. Codex’s existing 10-second refresh while Codex is open remains unchanged.

### Battery and Settings

- The top status area shows battery percentage and charging state without a central capsule.
- Tapping the battery status or swiping down from the top opens Quick Controls with battery, brightness, sound, auto-rotate, and time-sync state.
- Quick Controls are available only on the lock screen and carousel, and closing them restores the screen they covered. Running apps keep their touch area and gestures; return to the carousel before opening Quick Controls.
- Quick Controls show time-sync state but do not start synchronization. The full Settings card retains the explicit sync action.
- The Settings card opens the full existing settings list. Changing settings continues to persist through the current NVS configuration.

### Application screens

- Preserve each app’s current jobs and lifecycle. Standardize safe margins, page title placement, touch feedback, icon grammar, and secondary-state copy.
- Keep task-specific interactions intact: Codex’s four pages, Meet’s record/stop flow, Waveshare’s Motion/Spectrum selection, and immersive game/simulation touch controls.
- Ensure status, loading, offline, unavailable, busy, empty, and recording states remain visible and do not overlap controls.

## Visual and icon system

- Use an OLED-black base with a stationary, low-contrast natural-water illustration: soft blue-green haze, a few off-center ripple contours, and tiny water highlights. Leave the center quiet behind text and controls. It should feel calm on-wrist and remain legible at desk distance. Use restrained slate surfaces, cool blue/cyan for focus and active state, and coral only for errors or warnings. Keep the previous purple-red route and yellow ring out of the design.
- Build a custom geometric vector family for all nine app marks plus battery, settings, back/close, record, and carousel controls. Keep stroke weight, optical size, corner treatment, and state colors consistent. Distinguish each app with its silhouette, not color alone.
- Use high-contrast text, a compact but legible type scale, large primary actions, and percentage-based circular insets. Do not apply Android dp measurements literally; tune target sizes against the device’s 466 px screen and actual touch behavior.
- Motion communicates card changes, unlock progress, and open/close state only. It must not delay a tap or interfere with active recording or games.

## Prototype and references

- Clickable prototype: `agent-badge/sim/wear_carousel_prototype.html`.
- [Wear OS design overview](https://developer.android.com/design/ui/wear?hl=zh-cn)
- [Wear OS common layouts](https://developer.android.com/design/ui/wear/guides/foundations/common-layouts?hl=zh-cn)
- [Wear OS circular-screen quality guidance](https://developer.android.com/design/ui/wear/guides/foundations/quality-tiers?hl=zh-cn)
- [Wear OS visual quality guidance](https://developer.android.com/docs/quality-guidelines/wear-app-quality?hl=zh-cn)

## Builder decisions

- Quick Controls stay out of running apps so they cannot intercept game, recording, or sensor gestures.
- The Codex summary uses a timestamped shared snapshot and limited polling while it is the selected home card. This favors glanceable freshness while keeping radio use off in the lock screen and other apps.
- Manual time sync remains in full Settings to keep the quick panel shallow and predictable.

## Acceptance criteria

- The simulator and device show the lock screen, unlock progress, carousel, app entry, return path, and quick controls without clipped text or inaccessible tap targets.
- Every app and shared system icon is replaced by a distinct member of the Jarvis vector set.
- Settings and battery are independently understandable and reachable; battery state updates correctly, and charging is visible.
- Each retained app still opens, runs its existing task, and exits safely. Meet recording and Codex bridge behavior remain intact.
- Build and device smoke checks complete on the ESP32-S3-Touch-AMOLED-1.75C before calling the redesign complete.
