# MOTO GPS iOS App

This companion app works with Jarvis_OS. It handles iPhone location, place search, route planning, online/offline maps, navigation sync, and basic Apple Music controls. Search, route, and map requests use the HTTPS gateway; the device link uses encrypted BLE.

## Development

Requires Xcode 16+, the iOS 17 SDK, and XcodeGen. From this directory:

```sh
xcodegen generate
open MotoGPS.xcodeproj
```

Simulator build:

```sh
xcodebuild -project MotoGPS.xcodeproj -scheme MotoGPS \
  -destination 'generic/platform=iOS Simulator' build
```

Run Swift Package tests with `swift test` from this directory. Simulator can check UI flows that do not depend on Bluetooth, real location, background execution, or media controls. CoreBluetooth and ESP32 BLE require an iPhone.

## Gateway and signing

The app does not bundle anyone's Worker URL. Enter your own gateway root URL in Gateway Settings before first use. The Worker key is not shipped in the app or stored on the device. See the root [deployment guide](../../../docs/deployment/README.md).

Configure Bundle ID, Apple Team, signing certificates, and TestFlight credentials with your own Apple account. Privacy copy and permissions are in `App/DataUseView.swift`, `App/Info.plist`, and `App/PrivacyInfo.xcprivacy`.

See the root [system design](../../../design.md) and `../../../packages/moto-gps/shared/protocol/` for the BLE contract.
