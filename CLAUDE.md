# CLAUDE.md

ESP32 + servo robot that turns the fan/mode knob on a PTAC unit, controlled via a static web page over MQTT (HiveMQ Cloud). Read `README.md` for layout, the MQTT contract and broker setup, `docs/hardware.md` for hardware and open problems, `docs/test-plan.md` for the verification order.

## Rules

- Never commit `firmware/ptac_firmware/secrets.h` or any Wi-Fi / MQTT credential. This repo is public. `index.html` already contains a scoped web-client MQTT login on purpose (the page must be public); do not add other credentials to it. Do not print credentials in chat.
- Keep the MQTT topic and payload contract stable (see README) so `index.html` and the firmware stay compatible. If you change one, change the other in the same commit.
- The servo must never be held powered when idle. Keep attach-move-hold-detach, the 6 s continuous cap plus cooldown, and no boot-time movement. Never run a blocking network call while the servo is attached.
- Validate all MQTT payloads; presets are a fixed whitelist (OFF, HEAT3, HEAT2, COOL3, COOL2). Reject rather than clamp.
- Ignore commands in the first 2 s after subscribe (retained-command replay protection). Do not remove the `status` last-will.
- Do not touch the PTAC's mains wiring. Mechanical actuation of the knob only.
- Presets (calibrated knob positions) live in the ESP32's flash under NVS namespace `ptac`. Do not rename the namespace or the keys; erasing flash loses calibration.
- `SERVO_MIN_US` / `SERVO_MAX_US` and the UI deg-to-us mapping must be changed together, only after bench-finding the real range.
- Status: firmware flashed 2026-10-08 and confirmed booting, joining Wi-Fi and connecting to the broker with verified TLS. Servo behavior is not yet bench-tested. Do not call it working until `docs/test-plan.md` gates 1-4 pass.

## Conventions

- Firmware: Arduino sketch, folder name equals sketch name. Libraries: PubSubClient, ESP32Servo. TLS root cert in `isrg_root_x1.h`.
- Web UI: single static `index.html`, no build step, served by GitHub Pages. The mqtt.js script is version-pinned with an SRI hash; update both together.
- Comments only where logic is non-obvious.

## Owner preferences

Concise answers, no emojis, make reasonable calls instead of asking, do only what was asked and mention other findings in one sentence. Windows 11, PowerShell syntax.

## Open work

1. Gate 0 of the test plan: record both power supplies, confirm the servo ground is tied to an ESP32 GND pin, check the ESP32 and adapter after the servo failure.
2. HiveMQ: restrict the web user, add a separate robot user, rotate credentials that were ever pushed (see README "Broker setup").
3. Dedicated servo supply, common ground, capacitor, pull-down (docs/hardware.md).
4. Bench-find the servo range, then flash and run the test plan with the spare MG996R.
5. Design the knob cap and servo bracket (OpenSCAD or similar) once measurements exist. Move electronics off the discharge grille.
6. 24-hour hold test with the servo released; add a detent or periodic re-assert if the knob drifts.