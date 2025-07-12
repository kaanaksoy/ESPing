#include "discovery.h"

void checkAndSendDiscovery()
{
    prefs.begin("esping", false);
    bool discoverySent = prefs.getBool("discoverySent", false);
    prefs.end();

    // if (!discoverySent)
    // {
    sendDiscovery();
    prefs.begin("esping", false);
    prefs.putBool("discoverySent", true);
    prefs.end();
    // }
}

void handleResetDiscovery()
{
#ifdef SERIAL_DEBUG_ENABLED
    Serial.println("Resetting discovery and preferences...");
#endif

    // Clear everything including brightness/color
    prefs.begin("esping", false);
    prefs.clear();
    prefs.end();

    mqttClient.publish(STATE_TOPIC, "resetting", true);
    delay(100); // Let the MQTT message flush out
    ESP.restart();
}

void sendDiscovery()
{
    // ensure we're online
    if (!mqttClient.connected())
    {
        mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
        delay(25);
    }

    // --- SWITCH: preventSleep ---
    {
        StaticJsonDocument<512> doc;
        String topic = String(DISCOVERY_PREFIX) + "/switch/" + MQTT_CLIENT_ID + "_prevent_sleep/config";
        doc["name"] = "ESPing Sleep Prevention";
        doc["uniq_id"] = String(MQTT_CLIENT_ID) + "_prevent_sleep";
        doc["state_topic"] = STATE_TOPIC;
        doc["command_topic"] = COMMAND_TOPIC;
        doc["payload_on"] = "{\"preventSleep\": true}";
        doc["payload_off"] = "{\"preventSleep\": false}";
        doc["state_value_template"] = "{{ value_json.preventSleep }}"; // Use JSON path to get the state
        doc["retain"] = true;                                          // Retain the state
        doc["icon"] = "mdi:sleep-off";
        // device info
        JsonObject dev = doc.createNestedObject("device");
        dev["identifiers"] = MQTT_CLIENT_ID;
        dev["name"] = DEVICE_NAME;
        dev["model"] = "ESP32-C6";
        dev["mf"] = "Kaan Aksoy";
        dev["sw"] = SW_VERSION;
        dev["hw"] = HW_VERSION;

        String payload;
        serializeJson(doc, payload);
        bool ok = mqttClient.publish(topic.c_str(), payload.c_str(), /*retain=*/true);
#ifdef SERIAL_DEBUG_ENABLED
        Serial.println("--- MQTT DISCOVERY → SWITCH ---");
        Serial.println("Topic:   " + topic);
        Serial.println("Payload: " + payload);
        Serial.printf("  publish ok? %d\n\n", ok);
#endif
        mqttClient.loop();
        delay(50);
    }

    // --- BUTTON: resetDiscovery ---
    {
        StaticJsonDocument<512> doc;
        String topic = String(DISCOVERY_PREFIX) + "/button/" + MQTT_CLIENT_ID + "_reset_discovery/config";
        doc["name"] = "Reset ESPing HAAS Discovery";
        doc["uniq_id"] = String(MQTT_CLIENT_ID) + "_reset_discovery";
        doc["device_class"] = "restart";
        doc["command_topic"] = COMMAND_TOPIC;
        doc["payload_press"] = "{\"resetDiscovery\": true}";
        doc["availability_topic"] = AVAILABILITY_TOPIC;
        doc["retain"] = true; // Retain the reset command
        doc["qos"] = 1;       // Ensure the command is delivered at least once
        doc["icon"] = "mdi:restart";
        // device info
        JsonObject dev = doc.createNestedObject("device");
        dev["identifiers"] = MQTT_CLIENT_ID;
        dev["name"] = DEVICE_NAME;

        String payload;
        serializeJson(doc, payload);
        bool ok = mqttClient.publish(topic.c_str(), payload.c_str(), /*retain=*/true);
#ifdef SERIAL_DEBUG_ENABLED
        Serial.println("--- MQTT DISCOVERY → SWITCH ---");
        Serial.println("Topic:   " + topic);
        Serial.println("Payload: " + payload);
        Serial.printf("  publish ok? %d\n\n", ok);
#endif
        mqttClient.loop();
        delay(50);
    }

    // --- DEVICE TRIGGER: Button Pressed ---
    {
        StaticJsonDocument<512> doc;
        String topic = String(DISCOVERY_PREFIX) + "/device_automation/" + MQTT_CLIENT_ID + "_button_press/config";
        doc["name"] = "ESPing Button Press";
        doc["uniq_id"] = String(MQTT_CLIENT_ID) + "_button_press";
        doc["automation_type"] = "trigger";
        doc["type"] = "button_short_press";
        doc["platform"] = "device_automation";
        doc["availability_topic"] = AVAILABILITY_TOPIC;
        doc["subtype"] = "button_1";
        doc["topic"] = BUTTON_TOPIC;
        doc["payload"] = "pressed";
        // device info
        JsonObject dev = doc.createNestedObject("device");
        dev["identifiers"] = MQTT_CLIENT_ID;
        dev["name"] = DEVICE_NAME;

        String payload;
        serializeJson(doc, payload);
        bool ok = mqttClient.publish(topic.c_str(), payload.c_str(), /*retain=*/true);
#ifdef SERIAL_DEBUG_ENABLED
        Serial.println("--- MQTT DISCOVERY → DEVICE TRIGGER ---");
        Serial.println("Topic:   " + topic);
        Serial.println("Payload: " + payload);
        Serial.printf("  publish ok? %d\n\n", ok);
#endif
        mqttClient.loop();
        delay(50);
    }

    // --- DEVICE TRIGGER: Button Released ---
    {
        StaticJsonDocument<512> doc;
        String topic = String(DISCOVERY_PREFIX) + "/device_automation/" + MQTT_CLIENT_ID + "_button_release/config";
        doc["name"] = "ESPing Button Release";
        doc["uniq_id"] = String(MQTT_CLIENT_ID) + "_button_release";
        doc["platform"] = "device_automation";
        doc["automation_type"] = "trigger";
        doc["type"] = "button_short_release";
        doc["subtype"] = "button_1";
        doc["topic"] = BUTTON_TOPIC;
        doc["payload"] = "released";
        doc["availability_topic"] = AVAILABILITY_TOPIC;
        // device info
        JsonObject dev = doc.createNestedObject("device");
        dev["identifiers"] = MQTT_CLIENT_ID;
        dev["name"] = DEVICE_NAME;

        String payload;
        serializeJson(doc, payload);
        bool ok = mqttClient.publish(topic.c_str(), payload.c_str(), /*retain=*/true);
#ifdef SERIAL_DEBUG_ENABLED
        Serial.println("--- MQTT DISCOVERY → DEVICE TRIGGER ---");
        Serial.println("Topic:   " + topic);
        Serial.println("Payload: " + payload);
        Serial.printf("  publish ok? %d\n\n", ok);
#endif
        mqttClient.loop();
        delay(50);
    }

    // --- LIGHT: RGB LED ---
    {
        StaticJsonDocument<512> doc;
        String topic = String(DISCOVERY_PREFIX) + "/light/" + MQTT_CLIENT_ID + "_led/config";
        doc["name"] = "ESPing LED";
        doc["uniq_id"] = String(MQTT_CLIENT_ID) + "_led";
        doc["state_topic"] = STATE_TOPIC;
        doc["command_topic"] = COMMAND_TOPIC;
        doc["schema"] = "json";
        doc["brightness"] = true;
        doc["color_mode"] = true;
        JsonArray color_modes = doc.createNestedArray("supported_color_modes");
        color_modes.add("rgb");
        doc["brightness_scale"] = 255;
        doc["command_topic"] = COMMAND_TOPIC;
        doc["state_topic"] = STATE_TOPIC;
        doc["state_value_template"] = "{{ 'ON' if value_json.brightness > 0 else 'OFF' }}"; // Use JSON path to get the RGB color
        // doc["rgb_state_topic"] = STATE_TOPIC;
        // doc["rgb_value_template"] = "{{ value_json.color }}";             // Use JSON path to get the RGB color
        // doc["brightness_value_template"] = "{{ value_json.brightness }}"; // Use JSON path
        doc["icon"] = "mdi:led-strip-variant";
        JsonObject dev = doc.createNestedObject("device");
        dev["identifiers"] = MQTT_CLIENT_ID;
        dev["name"] = DEVICE_NAME;
        doc["retain"] = true; // Retain the state
        doc["qos"] = 1;       // Ensure the command is delivered at least once
        String payload;
        serializeJson(doc, payload);
        bool ok = mqttClient.publish(topic.c_str(), payload.c_str(), true);
#ifdef SERIAL_DEBUG_ENABLED
        Serial.println("--- MQTT DISCOVERY → LIGHT ---");
        Serial.println("Topic:   " + topic);
        Serial.println("Payload: " + payload);
        Serial.printf("  publish ok? %d\n\n", ok);
#endif
        mqttClient.loop();
        delay(50);
    }

    // --- SENSOR: Battery ---
    {
        StaticJsonDocument<512> doc;
        String topic = String(DISCOVERY_PREFIX) + "/sensor/" + MQTT_CLIENT_ID + "_battery/config";
        doc["name"] = "ESPing Battery";
        doc["uniq_id"] = String(MQTT_CLIENT_ID) + "_battery";
        doc["state_topic"] = BATTERY_TOPIC;
        doc["unit_of_measurement"] = "%%"; // % needs escaping
        doc["device_class"] = "battery";
        doc["state_class"] = "measurement";
        doc["icon"] = "mdi:battery";
        doc["value_template"] = "{{ value_json.batteryPercentage }}"; // Use JSON path to get the battery percentage
        doc["availability_topic"] = AVAILABILITY_TOPIC;
        // device info
        JsonObject dev = doc.createNestedObject("device");
        dev["identifiers"] = MQTT_CLIENT_ID;
        dev["name"] = DEVICE_NAME;

        String payload;
        serializeJson(doc, payload);
        bool ok = mqttClient.publish(topic.c_str(), payload.c_str(), true);
#ifdef SERIAL_DEBUG_ENABLED
        Serial.println("--- MQTT DISCOVERY → SENSOR ---");
        Serial.println("Topic:   " + topic);
        Serial.println("Payload: " + payload);
        Serial.printf("  publish ok? %d\n\n", ok);
#endif
        mqttClient.loop();
        delay(50);
    }
}
