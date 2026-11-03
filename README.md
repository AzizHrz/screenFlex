# ScreenFlex Starter

ScreenFlex Starter is ESP32 firmware for a display controller. It connects to the ScreenFlex service, receives control messages over MQTT, controls a TV using infrared, and evaluates a weekly schedule using the real-time clock.

## Features

- TV control using stored infrared commands
- TV state detection from a hardware input
- MQTT control modes: `on`, `off`, and `schedule`
- Weekly schedule evaluation using the RTC
- Ethernet support through a W5500 module
- Wi-Fi provisioning through an ESP32 access point and setup portal
- Device registration with the ScreenFlex service
- LittleFS-based local data storage
- Firmware version checks and OTA update support
- RGB status indication

## Hardware

The firmware targets an ESP32 DevKit-compatible board. Pin assignments are defined in `include/Hardware.h` and the main application source; verify them against your exact hardware before wiring.

| Component | Purpose |
|---|---|
| ESP32 DevKit | Main controller |
| W5500 Ethernet module | Wired network connectivity |
| IR receiver and emitter | Capture and send TV commands |
| TV status input | Detect TV state |
| RTC module | Time and date for scheduling |
| RGB LED | Device status indication |
| Button | Device interaction and reset functions |

## Getting started

### Requirements

- PlatformIO Core or the PlatformIO IDE
- ESP32 DevKit-compatible board and USB connection

### Clone and initialize dependencies

```bash
git clone https://github.com/AzizHrz/screenFlex.git
cd screenFlex
git checkout aziz
git submodule update --init --recursive
```

### Build, flash, and monitor

```bash
pio run
pio run --target upload
pio device monitor -b 115200
```

The repository also includes a `Makefile` for common tasks:

```bash
make build
make flash
make monitor
```

To upload the filesystem image:

```bash
pio run --target uploadfs
```

The PlatformIO environment is declared in `platformio.ini` and uses the `esp32doit-devkit-v1` target.

## Network setup

The firmware can use Ethernet or Wi-Fi. When Wi-Fi provisioning is required, it starts an ESP32 access point and a configuration web server.

### Configure Wi-Fi through the setup portal

1. Power on the ESP32.
2. Connect your phone or computer to the ESP32 access point.
3. Open `http://192.168.4.1` in a browser.
4. Enter your Wi-Fi credentials and submit the form.
5. After the ESP32 connects to the Wi-Fi network, use the IP shown in the portal or serial monitor to access the device.

`192.168.4.1` is the access-point address used during setup. It is not the device's address on your local Wi-Fi network.

If a hostname is configured in the firmware, you may also be able to access the device through:

```text
http://<mDNS-hostname>.local
```

## Device behavior

### Infrared control

At startup, the firmware checks LittleFS for saved IR data. If data is present, it loads the stored command for control. If no data is present, the device can enter capture mode to learn and save a command. Holding the setup button for several seconds clears the stored IR data and restarts the ESP32 so it can learn a new command.

The IR module supports multiple protocols, including NEC, Samsung, LG, and pulse-distance formats.

### MQTT control and schedules

The firmware builds MQTT device topics from the registered user and token.

| Topic | Purpose |
|---|---|
| `<user>/<token>/order` | Receives control orders |
| `<user>/<token>/state` | Publishes the TV state |
| `<user>/<token>/firmware` | Firmware-related messages |

Control orders support:

- `on` — turn the TV on
- `off` — turn the TV off
- `schedule` — decide the state based on the current RTC time and weekly schedule

The schedule includes timezone settings, enabled weekdays, and time slots.

## Project layout

| Path | Purpose |
|---|---|
| `platformio.ini` | Build and environment configuration |
| `include/` | Module interfaces and firmware configuration |
| `src/` | Main firmware and implementation files |
| `lib/` | Project libraries and submodules |
| `data/` | LittleFS data files |
| `test/` | Testing firmware and experiments |
| `scripts/` | Helper scripts used by PlatformIO |
| `Images/` | Project diagrams and visuals |
| `docs/` | Generated documentation |
| `Makefile` | Build, flash, and monitor shortcuts |

## Dependencies

PlatformIO declares the main libraries, including ArduinoJson and PubSubClient. Submodules provide the rest of the functionality for IR, Ethernet, RTC, and Wake-on-LAN support.

Initialize them with:

```bash
git submodule update --init --recursive
```

## Contribution

Use a feature branch for each change and keep commit messages clear and descriptive.

---

This project is for ESP32-based automation and TV control workflows. For hardware-specific questions, compare the wiring and pin map in `include/Hardware.h` with your actual device before flashing.
