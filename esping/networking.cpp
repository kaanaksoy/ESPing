#include "networking.h"
#include "globals.h"


void networking_routine()
{
  static bool wifi_connected = false;
  static bool mqtt_connected = false;
  static unsigned long wifi_connection_start_time = 0;
  static unsigned long mqtt_connection_start_time = 0;

  if (state.operationState == INITIALIZING) {
    setupWifi();
    return;
  }

  if (state.operationState == CONNECTING) {
    wifi_connected = 
    return;
  }
  
}

// Used in INITIALIZING state
void setupWifi()
{
  WiFi.mode(WIFI_STA);
  connectToWifi();
  return;
}

void connectToWifi()
{
  if (state.wifiConnected) return;
  state.wifiConnectionStartTime = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  state.wifiConnected = (WiFi.status() == WL_CONNECTED);
  if (state.wifiConnected) {
    state.wifiConnectionStartTime = 0;
  }
  return;
}

void setupMqtt()
{
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE); // Mainly for the discovery messages
  connectToMqtt();
  return;
}

void connectToMqtt()
{
  if (state.mqttConnected || !state.wifiConnected) return;
  state.mqttConnectionStartTime = millis();
  mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
  state.mqttConnected = (mqttClient.state() == MQTT_CONNECTED);
  if (state.mqttConnected) {
    state.mqttConnectionStartTime = 0;
  }
  return;
}

void turnOffNetworking()
{
  mqttClient.publish(AVAILABILITY_TOPIC, "offline", true);
  mqttClient.loop();
  delay(100);
  mqttClient.disconnect();
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();
}

void sendButtonPressMessage(bool pressed)
{
  if (!state.buttonPressedMessagePending || !state.mqttConnected) return;
  mqttClient.publish(BUTTON_TOPIC, pressed ? "pressed" : "released", true);
  if (pressed) {
    state.buttonMessageCooldown = millis();
    indicate_with_fade(DEFAULT_MESSAGE_COLOR);
    mqttClient.publish(BUTTON_TOPIC, "pressed", true);
    checkAndPublishBattery();
    state.buttonPressedMessagePending = false;
    state.buttonReleasedMessagePending = true;
  } else {
    if (millis() - state.buttonMessageCooldown > 1000) {
      mqttClient.publish(BUTTON_TOPIC, "released", true);
      state.buttonReleasedMessagePending = false;
    }
  }
  return;
}

void subscribeToTopics()
{
  if (!state.mqttConnected) return;
  mqttClient.setCallback(mqttCallback); // Purposefully setting the callback here to avoid delays in button messages.
  state.mqttSubscribed = mqttClient.subscribe(COMMAND_TOPIC, 1);
  return;
}

void publishAvailability()
{
  if (!state.mqttConnected || state.availabilityPublished) return;
  state.availabilityPublished = mqttClient.publish(AVAILABILITY_TOPIC, "online", true);
  return;
}

void publishState()
{
  StaticJsonDocument<128> doc;
  doc["preventSleep"] = state.preventSleep;
  JsonObject colorObj = doc.createNestedObject("color");
  colorObj["r"] = state.ledColor.r;
  colorObj["g"] = state.ledColor.g;
  colorObj["b"] = state.ledColor.b;
  doc["brightness"] = state.ledBrightness;
  doc["resetDiscovery"] = false;

  String payload;
  serializeJson(doc, payload);
  mqttClient.publish(STATE_TOPIC, payload.c_str(), true);
}

void checkAndPublishBattery()
{
  pinMode(BATT_ADC_PIN, INPUT);
  float batteryLevel = checkBatteryLevel();
  StaticJsonDocument<64> doc;
  doc["batteryPercentage"] = batteryLevel;
  String payload;
  serializeJson(doc, payload);
  mqttClient.publish(BATTERY_TOPIC, payload.c_str(), true);
}

// Callback function for MQTT messages
// This function is called when a message is received on a subscribed topic
// In this case, we use it to prevent the device from going to sleep
// If the message is "true", we prevent sleep; if "false", we allow sleep
// The payload is expected to be a JSON boolean value
// Example payload: {"preventSleep": true}
// If the payload is not valid JSON, we do nothing
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  bool updated = false;
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error)
    return;

  // Handle resetDiscovery early
  if (doc.containsKey("resetDiscovery") && doc["resetDiscovery"] == true)
  {
    handleResetDiscovery();
    return; // skip rest after reset
  }

  if (doc.containsKey("preventSleep"))
  {
    state.preventSleep = doc["preventSleep"];
  }

  if (doc.containsKey("color"))
  {
    state.messageColor = doc["color"].as<JsonObject>();
    state.colorMessageReceived = true;
  }

  if (doc.containsKey("brightness"))
  {
    int b = doc["brightness"].as<int>();
    state.messageBrightness = constrain(b, 0, 255);
    state.brightnessMessageReceived = true;
  }
}
