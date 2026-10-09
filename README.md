# PTAC Robot

A small ESP32 + servo robot that turns the fan/mode knob on a PTAC unit (Applied Comfort `DMQB09K34S7A20`), controlled from a phone-friendly web page over MQTT (HiveMQ Cloud).

```
phone / browser (index.html, MQTT over WSS :8884)
        |
   HiveMQ Cloud  (topics under ptac/joel_a83f2/)
        |
ESP32 (MQTT over TLS :8883) --PWM--> servo --> PTAC fan/mode knob
```

## Layout

| Path | What |
|---|---|
| `index.html` | Control UI, served by GitHub Pages. Manual slider, per-mode GO / Set, OFF. |
| `firmware/ptac_firmware/` | ESP32 Arduino sketch. Copy `secrets.h.example` to `secrets.h` and fill it in. |
| `docs/hardware.md` | Parts list, power, the PTAC unit, mounting notes, known problems. |
| `docs/test-plan.md` | Gated bench test to run before the servo touches the knob. |
| `CLAUDE.md` | Context and rules for Claude Code sessions on this repo. |

## MQTT contract

Base topic: `ptac/joel_a83f2`

| Topic | Payload | Effect |
|---|---|---|
| `<base>/cmd/us` | `500`-`2500` | Move servo to that pulse width (manual slider) |
| `<base>/cmd/save/<MODE>` | `500`-`2500` | Store the preset in ESP32 flash and move there |
| `<base>/cmd/go/<MODE>` | anything | Move to the stored preset. Unknown preset: no move |
| `<base>/state` (retained) | `{"us":N,"reason":"..."}` | Last position and why. Reasons: `boot`, `us`, `save_<MODE>`, `go_<MODE>`, `rejected_*`, `unknown_<MODE>`, `save_failed_<MODE>`, `rate_limited` |
| `<base>/status` (retained) | `online` / `offline` | Robot presence. `offline` is the MQTT last-will, sent by the broker when the ESP32 drops |

| `<base>/diag` (retained) | `{"up":s,"rssi":dBm,"heap":bytes,"ev":[last 8 events]}` | Remote diagnostics: boot reset reason (poweron/brownout/...), attach/release/state events, wifi_lost |

Allowed modes (whitelisted in firmware): `OFF`, `HEAT3`, `HEAT2`, `COOL3`, `COOL2`.

## Firmware behavior

- The servo is powered only while moving plus 1 s, then detached. It never stays powered more than 6 s in a row; after that moves are refused for 6 s (`rate_limited`).
- Nothing moves at boot. The last position is restored from flash only as a number.
- Commands received in the first 2 s after subscribing are ignored, so a retained command is never replayed after a reboot. To clear a stuck retained command, publish an empty retained payload to the same topic.
- Payloads are digits-only and range-checked; rejected rather than clamped.
- TLS is verified against ISRG Root X1. The ESP32 needs NTP time before it will connect to the broker.
- Reconnects never block while the servo is powered.

## Firmware setup

1. Arduino IDE (or `arduino-cli`) with the ESP32 board package (tested with core 3.3.12). Libraries: `PubSubClient`, `ESP32Servo`.
2. `cp firmware/ptac_firmware/secrets.h.example firmware/ptac_firmware/secrets.h` and fill in Wi-Fi and HiveMQ credentials. `secrets.h` is git-ignored.
3. Board: ESP32 Dev Module. Upload without erasing flash, otherwise saved presets are lost.
4. Servo signal on GPIO 13.

## Broker setup (HiveMQ Cloud)

The web page login is public by design, so its permissions must be narrow. Create two credentials:

- **Web user** (in `index.html`): publish only `ptac/joel_a83f2/cmd/go/+` (no `cmd/us`, no `cmd/save/#`, no retain), subscribe only `ptac/joel_a83f2/state` and `ptac/joel_a83f2/status`. Limit its connections. Note: manual slider and Set need `cmd/us` and `cmd/save/#`, so either accept that wider scope or move calibration to a private credential.
- **Robot user** (in `secrets.h`): publish `state` and `status`, subscribe `cmd/#`.

Rotate any credential that was ever committed, pushed or built into a published page.

## Calibrating presets

Open the page, Enable Manual, move the slider until the knob sits on the right mark, press Set on that mode's row. Repeat per mode. Only do this after the bench tests in `docs/test-plan.md`.

## Status

As of 2026-10-08 the web page controls the servo end to end (page -> HiveMQ -> ESP32 -> servo). Still open: servo power for the long term, range finding, the knob mount, calibration and the soak/hold tests. See `docs/hardware.md` for hardware findings and `docs/test-plan.md` for the order to verify.