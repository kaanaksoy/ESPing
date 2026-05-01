#include "discovery.h"

void checkAndSendDiscovery()
{
    prefs.begin("esping", false);
    String storedVersion = prefs.getString("discoveryVer", "");
    prefs.end();

    // Re-send discovery whenever SW_VERSION changes — keeps HA in sync
    // when you add/rename entities. No more manual reset needed.
    if (storedVersion != SW_VERSION)
    {
        sendDiscovery();
        prefs.begin("esping", false);
        prefs.putString("discoveryVer", SW_VERSION);
        prefs.end();
    }
}

void handleResetDiscovery()
{
#ifdef SERIAL_DEBUG_ENABLED
    Serial.println("Resetting discovery and preferences...");
#endif

    prefs.begin("esping", false);
    prefs.clear();
    prefs.end();

    mqttClient.publish(STATE_TOPIC.c_str(), "resetting", true);
    delay(100);
    ESP.restart();
}

void sendDiscovery()
{
    if (!mqttClient.connected())
    {
        mqttClient.connect(DEVICE_ID.c_str(), MQTT_USER, MQTT_PASSWORD);
        delay(250);
    }

    // --- SWITCH: preventSleep ---
    {
        StaticJsonDocument<512> doc;
        String topic = String(DISCOVERY_PREFIX) + "/switch/" + DEVICE_ID + "_prevent_sleep/config";
        doc["name"] = "Prevent Sleep";
        doc["uniq_id"] = DEVICE_ID + "_prevent_sleep";
        doc["state_topic"] = STATE_TOPIC;
        doc["command_topic"] = COMMAND_TOPIC;
        doc["payload_on"] = "{\"preventSleep\": true}";
        doc["payload_off"] = "{\"preventSleep\": false}";
        doc["state_value_template"] = "{{ value_json.preventSleep }}";
        doc["retain"] = true;
        doc["icon"] = "mdi:sleep-off";
        JsonObject dev = doc.createNestedObject("device");
        dev["identifiers"] = DEVICE_ID;
        dev["name"] = DEVICE_NAME;
        dev["model"] = "ESP32-C6";
        dev["mf"] = "Kaan Aksoy";
        dev["sw"] = SW_VERSION;
        dev["hw"] = HW_VERSION;

        String payload;
        serializeJson(doc, payload);
        mqttClient.publish(topic.c_str(), payload.c_str(), /*retain=*/true);
        mqttClient.loop();
        delay(100);
    }

    // --- DEVICE TRIGGER: Button Pressed ---
    {
        StaticJsonDocument<512> doc;
        String topic = String(DISCOVERY_PREFIX) + "/device_automation/" + DEVICE_ID + "_button_press/config";
        doc["name"] = "Button Press";
        doc["uniq_id"] = DEVICE_ID + "_button_press";
        doc["automation_type"] = "trigger";
        doc["type"] = "button_short_press";
        doc["platform"] = "device_automation";
        doc["availability_topic"] = AVAILABILITY_TOPIC;
        doc["subtype"] = "button_1";
        doc["topic"] = BUTTON_TOPIC;
        doc["payload"] = "pressed";
        JsonObject dev = doc.createNestedObject("device");
        dev["identifiers"] = DEVICE_ID;
        dev["name"] = DEVICE_NAME;

        String payload;
        serializeJson(doc, payload);
        mqttClient.publish(topic.c_str(), payload.c_str(), /*retain=*/true);
        mqttClient.loop();
        delay(100);
    }

    // --- DEVICE TRIGGER: Button Released ---
    {
        StaticJsonDocument<512> doc;
        String topic = String(DISCOVERY_PREFIX) + "/device_automation/" + DEVICE_ID + "_button_release/config";
        doc["name"] = "Button Release";
        doc["uniq_id"] = DEVICE_ID + "_button_release";
        doc["platform"] = "device_automation";
        doc["automation_type"] = "trigger";
        doc["type"] = "button_short_release";
        doc["subtype"] = "button_1";
        doc["topic"] = BUTTON_TOPIC;
        doc["payload"] = "released";
        doc["availability_topic"] = AVAILABILITY_TOPIC;
        JsonObject dev = doc.createNestedObject("device");
        dev["identifiers"] = DEVICE_ID;
        dev["name"] = DEVICE_NAME;

        String payload;
        serializeJson(doc, payload);
        mqttClient.publish(topic.c_str(), payload.c_str(), /*retain=*/true);
        mqttClient.loop();
        delay(100);
    }

    // --- SENSOR: Battery ---
    {
        StaticJsonDocument<512> doc;
        String topic = String(DISCOVERY_PREFIX) + "/sensor/" + DEVICE_ID + "_battery/config";
        doc["name"] = "Battery";
        doc["uniq_id"] = DEVICE_ID + "_battery";
        doc["state_topic"] = BATTERY_TOPIC;
        doc["unit_of_measurement"] = "%";
        doc["device_class"] = "battery";
        doc["state_class"] = "measurement";
        doc["icon"] = "mdi:battery";
        doc["value_template"] = "{{ value_json.batteryPercentage }}";
        JsonObject dev = doc.createNestedObject("device");
        dev["identifiers"] = DEVICE_ID;
        dev["name"] = DEVICE_NAME;

        String payload;
        serializeJson(doc, payload);
        mqttClient.publish(topic.c_str(), payload.c_str(), true);
        mqttClient.loop();
        delay(100);
    }
}
