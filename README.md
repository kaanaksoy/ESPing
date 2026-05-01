# ESPing – A Wi-Fi Button That Sleeps

**ESPing** is a battery-friendly ESP32-C6 project that acts as a stateless smart button.  
On wake, it connects to Wi-Fi, fires an MQTT event, and goes straight back to deep sleep — typically in under a second on subsequent presses.

---

## ✨ Features

- Fast wake → Wi-Fi → MQTT → sleep (~400–800 ms after the first connect)
- WiFi fast-reconnect via cached BSSID/channel/IP in RTC memory (no router config needed)
- Ultra-low-power operation (ideal for 18650 battery)
- MQTT with Home Assistant auto-discovery — appears as a native HA device with button triggers, battery sensor, and config switch
- `pressed` / `released` device-trigger events, with `retain=false` so HA doesn't replay stale presses on reconnect
- Last Will & Testament so HA correctly marks the device offline if it dies mid-session
- `preventSleep` mode for OTA flashing and live config tweaks
- Color-configurable status NeoPixel (settable from HA via MQTT)
- Battery percentage reported on every press
- **Per-board unique ID derived from chip MAC** — flash the same firmware to every unit, no per-device editing

---

## 📦 MQTT Topics

All topics are prefixed with the runtime device ID, e.g. `esping-a1b2c3d4e5f6/`.

| Topic                          | Direction | Description                                                  |
|--------------------------------|-----------|--------------------------------------------------------------|
| `<id>/button`                  | publish   | `pressed` then `released` on each wake (not retained)        |
| `<id>/battery`                 | publish   | JSON `{"batteryPercentage": 87.4}` (retained)                |
| `<id>/state`                   | publish   | JSON state: `preventSleep`, color, brightness (retained)     |
| `<id>/availability`            | publish   | `online` / `offline` via LWT (retained)                      |
| `<id>/command`                 | subscribe | JSON commands — see below                                    |
| `homeassistant/+/<id>_*/config`| publish   | HA discovery payloads (retained, version-gated)              |

### Command payloads

Send JSON to `<id>/command`:

```json
{"preventSleep": true}                           // stay awake (e.g. for OTA)
{"color": {"r": 255, "g": 0, "b": 128}}          // set status LED color
{"brightness": 128}                              // 0–255
{"resetDiscovery": true}                         // wipe prefs and restart
```

Color and brightness are persisted to flash and survive deep sleep.

---

## 🚀 How It Works

1. Button press wakes the ESP32 from deep sleep via EXT1 wakeup.
2. WiFi attempts a fast reconnect using cached BSSID/channel/IP from RTC memory. Falls back to a full scan + DHCP if that fails, then re-caches.
3. MQTT connects with Last Will configured.
4. `pressed` is published immediately — this is the critical path.
5. Availability, discovery (if version changed), LED fade, battery read, and `released` event follow.
6. Briefly listens for `preventSleep` or other commands, then deep-sleeps.

If `preventSleep` is set, the device stays awake in `loop()` until the flag is cleared.

---

## 🔧 Setup

### Requirements

- ESP32-C6 board (developed on the DFRobot Beetle ESP32-C6 V1.0)
- LiPo or 18650 battery (optional for portable use)
- MQTT broker (e.g. Mosquitto, including the Home Assistant Mosquitto add-on)
- Home Assistant with the MQTT integration enabled (optional but recommended)

### Configuration

Copy `credentials_example.h` to `credentials.h` and edit:

```cpp
#define WIFI_SSID     "YourWiFi"
#define WIFI_PASSWORD "YourPassword"
#define MQTT_SERVER   "192.168.1.100"
#define MQTT_PORT     1883
#define MQTT_USER     "username"
#define MQTT_PASSWORD "password"
```

Optionally tweak `defines.h`:

- `DEVICE_PREFIX` — the short tag in topics and IDs (default `esping`)
- `DEVICE_PREFIX_FRIENDLY` — the human-readable name shown in HA (default `ESPing`)
- `SW_VERSION` — bump to force HA discovery to re-publish
- `VOLTAGE_CALIBRATION_FACTOR` — per-board ADC tuning for battery readings
- `WIFI_TIMEOUT_FAST` / `WIFI_TIMEOUT_FULL` — connect timeouts

### Building multiple units

Flash the same firmware to every board. Each device derives its ID from the chip's factory MAC, so there are no client ID collisions or topic clashes. Boards appear in HA automatically with names like `ESPing a1b2c3d4e5f6` — rename them in the HA UI as you please.

### Pin assignments (Beetle ESP32-C6)

| Function          | GPIO  |
|-------------------|-------|
| Button (wake)     | 4     |
| NeoPixel data     | 21    |
| Battery ADC       | 0     |
| Built-in LED      | 15    |

---

## 🔄 Upgrading from earlier versions

If you previously ran ESPing with hardcoded `DEVICE_ID` (e.g. `esping01-2`), boards will appear as new devices in HA after flashing 0.4.0+. Clean up old retained discovery topics with MQTT Explorer or `mosquitto_pub -r -n` against `homeassistant/+/<old_id>_*/config` to remove the ghost devices.
