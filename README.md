# ESPing – A Wi-Fi Button That Sleeps

**ESPing** is a battery-friendly ESP32-C6 project that acts as a stateless smart button.  
On reset, it quickly connects to Wi-Fi, sends an MQTT message, and goes straight back to deep sleep.

---

## ✨ Features

- (Almost) Instant boot → Wi-Fi → MQTT → sleep
- Ultra-low-power operation (ideal for 18650 battery)
- MQTT-based (perfect for Home Assistant, Node-RED, etc.)
- Publishes a "pressed" event on boot (customizable)
- Optional override to stay awake via MQTT
- Onboard LED for feedback (GPIO15)

---

## 📦 MQTT Topics

| Topic               | Description                         |
|--------------------|-------------------------------------|
| `esping/state`      | Publishes `pressed` then `released` |
| `esping/prevent-sleep` | If payload is `1`, prevents sleep (helps to retain the message) |

---

## 🚀 How It Works

1. On boot/reset, ESPing:
   - Connects to Wi-Fi and MQTT broker
   - Publishes a `pressed` → `released` event
   - Waits briefly for a `prevent-sleep` message
2. If no override is received (or retained) → enters deep sleep

---

## 🔧 Setup

### Requirements

- ESP32-C6 board (e.g. DFRobot Beetle-C6)
- 18650 battery (optional for portable use)
- MQTT broker (e.g. Mosquitto)
- Wi-Fi access

### Configuration

Copy `credentials_example.h` to `credentials.h` and edit with your values:

```cpp
#define WIFI_SSID     "YourWiFi"
#define WIFI_PASSWORD "YourPassword"
#define MQTT_SERVER   "192.168.1.100"
#define MQTT_PORT     1883
#define MQTT_USER     "username"
#define MQTT_PASSWORD "password"
