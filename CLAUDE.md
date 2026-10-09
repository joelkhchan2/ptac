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
- Status (2026-10-08): firmware flashed and controlling the spare MG996R from the web page; servo output verified (50 Hz, correct pulse widths) and the servo moves on GO/slider. Not yet done: servo range finding, mounting on the knob, calibration, soak and hold tests (`docs/test-plan.md` gates 2-4), permanent servo power.

## Conventions

- Firmware: Arduino sketch, folder name equals sketch name. Libraries: PubSubClient, ESP32Servo. TLS root cert in `isrg_root_x1.h`.
- Web UI: single static `index.html`, no build step, served by GitHub Pages. The mqtt.js script is version-pinned with an SRI hash; update both together.
- Comments only where logic is non-obvious.

## Owner preferences

Concise answers, no emojis, make reasonable calls instead of asking, do only what was asked and mention other findings in one sentence. Windows 11, PowerShell syntax.

## Open work

1. Decide permanent servo power: ESP32 VIN off the white USB charger (read its rating) or the Arkare 5V 2A adapter with short thick wires; add a 470-1000 uF capacitor either way. The Arkare + green terminal + Dupont chain did not move a servo earlier (unresolved: adapter, contact, or the dead first servo).
2. Bench-find the servo range, then narrow `SERVO_MIN_US` / `SERVO_MAX_US` and the UI mapping together (test-plan gate 2).
3. Design the knob cap and servo bracket (OpenSCAD or similar) once measurements exist; mount the servo coaxial with or geared to the knob. Move electronics off the discharge grille.
4. Calibrate presets on the knob, then the 30-minute cycle and 24-hour hold tests (gates 3-4); add a detent or periodic re-assert if the knob drifts.
5. HiveMQ: restrict the web user, add a separate robot user, rotate credentials that were ever pushed (README "Broker setup").

## Lessons (do not repeat)

- A servo's 3-pin plug (GND, 5V, signal) must never be plugged straight onto the ESP32 pins VIN, GND, D13: the order is reversed and puts 5V backwards on the servo. One jumper per wire.
- Before blaming the servo or library, measure the ESP32 output in-chip (sample the pin with micros()); that proved the signal was correct and isolated the fault to wiring/power.
- Power the servo from VIN/adapter, never the 3V3 pin. Read pin labels off the board, not from memory.
- Remote diagnostics: subscribe to the retained `ptac/joel_a83f2/diag` topic for reset reason (brownout!), attach/release events and RSSI.