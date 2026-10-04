# Muse Gadget on a generic ESP32-C6-N16

Bringing Meta's [muse-gadget-sdk](https://github.com/facebookincubator/muse-gadget-sdk)
to a generic AliExpress ESP32-C6 module (16 MB flash, **no PSRAM**) — from a dead
board to a paired device, with two upstream PRs along the way.

## TL;DR

| | |
|---|---|
| Board | Generic ESP32-C6-N16 (AliExpress), no PSRAM |
| SDK | `muse-gadget-sdk/esp32`, ESP-IDF v6.0.1 |
| Result | Boots, advertises BLE as `MuseGadget-XXXXXX`, pairs with the Muse Android app, joins Wi-Fi, OTA-ready |
| Upstream | [PR #54](https://github.com/facebookincubator/muse-gadget-sdk/pull/54) (board support) · [PR #55](https://github.com/facebookincubator/muse-gadget-sdk/pull/55) (LED GPIO as Kconfig option) |

## The bring-up story

### 1. Wrong chip, wrong target
The first flash failed immediately: the manual builds for the **ESP32-C5** by
default, and `esptool` refused the board on the desk — *"This chip is ESP32-C3,
not ESP32-C5."* Fix: `idf.py set-target esp32c6`. Lesson: the default target is
C5; always `set-target` first.

### 2. The download-mode dance
The C6 didn't reliably enter flashing mode on plug-in. Fix: **hold BOOT while
plugging in USB** (or hold BOOT, tap RST, release BOOT). The manual documents
the same sequence — trust it.

### 3. The silent stall (the interesting one)
After a successful flash the board booted… and stopped. The log ended at:
```
I (914) link.app: HEAP after noise_ctrl_init
```
No BLE advertising (verified with `bluetoothctl` on a laptop whose scan worked
fine — ruling out phone-side issues — plus nRF Connect with location permission
granted), no Wi-Fi join, no reboot loop. Just silence.

Reading the firmware source showed why: `sdkconfig.defaults` is tuned for the
ESP32-C5 **with 8 MB PSRAM** — full NimBLE pools, 16 KB TLS buffers routed to
PSRAM, home-network tunnel — and **both watchdogs are disabled** (a C5 PSRAM
workaround). On a PSRAM-less C6, internal SRAM is starved during BLE bring-up,
and with the watchdogs off the stall is completely silent.

### 4. The fix
`devices/sdkconfig.muse-c6-nopsram` (in this repo), modeled on the repo's
Waveshare C6 overlay minus the display selection: PSRAM compiled out, mbedTLS
internal with small/dynamic buffers, tunnel off, trimmed Wi-Fi/lwIP/NimBLE
pools. Result: clean boot, `boot: unpaired - advertising (always on while
unpaired)`, pairing works.

### 5. Board quirks found along the way
- **Status LED**: the firmware hardcoded the WS2812 pin to GPIO27 (C5
  DevKitC-1). This board's RGB LED is on **GPIO8** — verified with the Arduino
  sketch in `tools/` (also confirmed GRB color order). Upstreamed as a Kconfig
  option: PR #55.
- **BOOT button**: default GPIO28 (C5); on C6 devkits it's **GPIO9** — remapped
  in the overlay. Needed for pairing confirmation and long-press setup reset.
- **Stale NVS**: reflashing preserves pairing/Wi-Fi settings; `idf.py
  erase-flash` for a truly clean slate (the manual says so — it's true).
- **C6 USB monitor flakiness**: the native USB-Serial/JTAG re-enumerates on
  reset and `idf_monitor` drops output. Don't trust a silent log alone; verify
  with `bluetoothctl` and your router's client list.

## What's in this repo

```
muse-gadget-c6-n16/
├── README.md                              # this file
├── devices/
│   └── sdkconfig.muse-c6-nopsram          # board overlay (also upstreamed as PR #54)
└── tools/
    └── rgb_led_test/
        └── rgb_led_test.ino               # Arduino sketch: find your board's RGB LED pin
```

## Build & flash

```sh
# ESP-IDF v6.0.1, installed per the SDK manual
. ~/esp/esp-idf-v6/export.sh
cd <muse-gadget-sdk>/esp32

# copy the overlay in, then build into its own directory
cp <this-repo>/devices/sdkconfig.muse-c6-nopsram devices/
idf.py -B build-c6-nopsram -DIDF_TARGET=esp32c6 \
  -DSDKCONFIG=build-c6-nopsram/sdkconfig \
  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;devices/sdkconfig.muse-c6-nopsram" build

# set your SDK token + Wi-Fi in this build dir, then flash fresh
idf.py -B build-c6-nopsram menuconfig
idf.py -p /dev/ttyACM0 erase-flash
idf.py -B build-c6-nopsram -p /dev/ttyACM0 flash monitor
```

Look for `boot: unpaired - advertising (always on while unpaired)` in the log,
then find `MuseGadget-XXXXXX` in a Bluetooth scan and pair from the Muse app
(Settings → Devices → Developer mode).

## Upstream contributions

- **PR #54** — generic ESP32-C6 (no PSRAM) board support: the overlay plus
  devices-table rows.
- **PR #55** — `HOMEHUB_LED_STRIP_GPIO` Kconfig option (int, default 27) so
  boards with the RGB LED elsewhere don't have to patch the source.

