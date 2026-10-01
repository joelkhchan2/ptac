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
| `CLAUDE.md` | Context and rules for Claude Code sessions on this repo. |

## MQTT contract

Base topic: `ptac/joel_a83f2`

| Topic | Payload | Effect |
|---|---|---|
| `<base>/cmd/us` | `500`-`2500` | Move servo to that pulse width (manual slider) |
| `<base>/cmd/save/<MODE>` | `500`-`2500` | Store the preset in ESP32 flash and move there |
| `<base>/cmd/go/<MODE>` | anything | Move to the stored preset. Unknown preset: no move |
| `<base>/state` (retained) | `{"us":N,"reason":"..."}` | Last position and why. Reasons: `boot`, `us`, `save_<MODE>`, `go_<MODE>`, `rejected_*`, `unknown_<MODE>` |

Modes used by the UI: `OFF`, `HEAT3`, `HEAT2`, `COOL3`, `COOL2`. Preset names are 1-15 chars of `A-Za-z0-9_`.

The servo is only driven while moving plus 1 s, then detached so it draws no current and does not fight the knob.

## Firmware setup

1. Arduino IDE with the ESP32 board package. Libraries: `PubSubClient`, `ESP32Servo`.
2. `cp firmware/ptac_firmware/secrets.h.example firmware/ptac_firmware/secrets.h` and fill in Wi-Fi and HiveMQ credentials. `secrets.h` is git-ignored.
3. Board: ESP32 Dev Module. Upload without erasing flash, otherwise saved presets are lost.
4. Servo signal on GPIO 13.

## Calibrating presets

Open the page, Enable Manual, move the slider until the knob sits on the right mark, press Set on that mode's row. Repeat per mode.

## Status

Works, but see `docs/hardware.md` for the open issues: the MG996R overheated and stopped, the shared 5V 2A supply is undersized, and the knob coupling (duct tape) needs a proper printed mount.
