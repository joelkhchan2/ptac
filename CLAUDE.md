# CLAUDE.md

ESP32 + servo robot that turns the fan/mode knob on a PTAC unit, controlled via a static web page over MQTT (HiveMQ Cloud). Read `README.md` for layout and the MQTT contract, `docs/hardware.md` for hardware and open problems.

## Rules

- Never commit `firmware/ptac_firmware/secrets.h` or any Wi-Fi / MQTT credential. This repo is public. `index.html` already contains a scoped web-client MQTT login on purpose (the page must be public); do not add other credentials to it.
- Keep the MQTT topic and payload contract stable (see README) so `index.html` and the firmware stay compatible. If you change one, change the other in the same commit.
- The servo must never be held powered when idle. Keep the attach-move-hold-detach behavior in the firmware. Do not add boot-time movement.
- Validate all MQTT payloads and preset names; reject rather than clamp.
- Do not touch the PTAC's mains wiring. Mechanical actuation of the knob only.
- Presets (calibrated knob positions) live in the ESP32's flash under NVS namespace `ptac`. Do not rename the namespace or the keys, and note that erasing flash loses calibration.
- Firmware compiles clean (arduino-cli, esp32:esp32:esp32, core 3.3.12, ESP32Servo 3.2.1, PubSubClient) as of 2026-10-05 but has not been flashed or bench-tested. Bench-test before declaring it working.

## Conventions

- Firmware: Arduino sketch, folder name equals sketch name. Libraries: PubSubClient, ESP32Servo.
- Web UI: single static `index.html`, no build step, served by GitHub Pages.
- Comments only where logic is non-obvious.

## Owner preferences

Concise answers, no emojis, make reasonable calls instead of asking, do only what was asked and mention other findings in one sentence. Windows 11, PowerShell syntax.

## Open work

1. Verify the original MG996R is not damaged (docs/hardware.md problem 1), then flash the new firmware.
2. Dedicated servo power supply plus capacitor.
3. Pick a lower-torque servo or stepper after measuring knob torque.
4. Design the knob cap and servo bracket (OpenSCAD or similar) once measurements exist.
5. Move electronics off the discharge grille.
6. Optional: LWT / online indicator in the UI, strict TLS in the firmware.
