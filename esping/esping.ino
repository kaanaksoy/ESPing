#include <WiFi.h>
#include "esp_wifi.h"
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include "driver/rtc_io.h"
#include "defines.h"
#include "discovery.h"

#if __has_include("credentials.h")
#include "credentials.h"
#else
#include "credentials_example.h"
#endif

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Adafruit_NeoPixel statusLed(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Preferences prefs;

// --- RTC memory: survives deep sleep, lost on power cycle / reset.
// We cache enough to skip the WiFi scan and DHCP on subsequent wakes.
RTC_DATA_ATTR struct {
  bool     valid;
  uint8_t  bssid[6];
  uint8_t  channel;
  uint32_t local_ip;
  uint32_t gateway;
  uint32_t subnet;
  uint32_t dns;
} rtcWifi;

struct Color
{
  uint8_t r, g, b;
};

Color messageColor = DEFAULT_MESSAGE_COLOR;
uint8_t messageBrightness = DEFAULT_LED_BRIGHTNESS;

bool preventSleep = false;
unsigned long lastBlink = 0;

// Function prototypes
void mqttCallback(char *topic, byte *payload, unsigned int length);
#ifdef SERIAL_DEBUG_ENABLED
void print_wakeup_reason();
#endif
void indicate_with_fade(Color color);
void write_led_color(Color color, int brightness);
void goToSleep();
void turn_off_led();
void loadSavedValues();
void saveNewValues(Color messageColor, uint8_t messageBrightness);
bool connectWifiFast();
bool connectWifiFull();
void cacheWifiState();
bool connectMqtt(unsigned long timeout_ms);
void publishState();
void checkAndPublishBattery();

// --- Program Begin
void setup()
{
#ifdef SERIAL_DEBUG_ENABLED
  Serial.begin(115200);
  delay(1000);
  print_wakeup_reason();
  Serial.println("Booting ESPing...");
#endif

#ifdef BUILT_IN_LED_ENABLED
  pinMode(BUILTIN_LED_PIN, OUTPUT);
  digitalWrite(BUILTIN_LED_PIN, HIGH);
#endif

  // ---- WiFi: try fast reconnect first, fall back to full scan ----
  bool wifiUp = false;
  if (rtcWifi.valid) {
    wifiUp = connectWifiFast();
#ifdef SERIAL_DEBUG_ENABLED
    Serial.printf("Fast reconnect: %s (%lu ms)\n",
                  wifiUp ? "OK" : "FAIL", millis());
#endif
  }
  if (!wifiUp) {
    wifiUp = connectWifiFull();
#ifdef SERIAL_DEBUG_ENABLED
    Serial.printf("Full connect: %s (%lu ms)\n",
                  wifiUp ? "OK" : "FAIL", millis());
#endif
    if (wifiUp) cacheWifiState();
  }
  if (!wifiUp) {
    // Couldn't get on the network at all — invalidate cache so next wake
    // does a clean scan, then sleep. The press is lost; nothing to do.
    rtcWifi.valid = false;
    goToSleep();
  }

  // ---- MQTT ----
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setKeepAlive(10);
  mqttClient.setBufferSize(1024);
  mqttClient.setCallback(mqttCallback);

  statusLed.begin();
  statusLed.clear();
  statusLed.show();

  if (!connectMqtt(MQTT_CONNECT_TIMEOUT)) {
    // Broker unreachable. Don't spin forever — sleep and try next press.
#ifdef SERIAL_DEBUG_ENABLED
    Serial.println("MQTT connect failed, sleeping");
#endif
    goToSleep();
  }

  // ---- Publish the press IMMEDIATELY — this is the critical path ----
  // retain=false: button events are transient. Retained "pressed" causes
  // spurious HA triggers on reconnect.
  mqttClient.publish(BUTTON_TOPIC, "pressed", /*retain=*/false);
  mqttClient.loop();

  // Mark online for HA availability after the event is on the wire.
  mqttClient.publish(AVAILABILITY_TOPIC, "online", /*retain=*/true);

  // Subscribe to commands so preventSleep / color updates can land
  mqttClient.subscribe(COMMAND_TOPIC, 1);

#ifdef DISCOVERY_ENABLED
  checkAndSendDiscovery();
#endif
  loadSavedValues();
  indicate_with_fade(messageColor);
  checkAndPublishBattery();

  // Brief release event so HA gets both edges
  delay(50);
  mqttClient.publish(BUTTON_TOPIC, "released", /*retain=*/false);
  mqttClient.loop();

  // ---- Stay awake briefly to receive any commands (color/preventSleep) ----
  unsigned long start = millis();
  while (!preventSleep && millis() - start < MQTT_LOOP_WAIT)
  {
    mqttClient.loop();
    delay(25);
  }
  if (!preventSleep)
  {
    goToSleep();
  }
}

void loop()
{
  if (!preventSleep)
  {
    goToSleep();
  }

  // Reconnect WiFi if disconnected
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.reconnect();
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_FULL)
    {
      delay(50);
    }
    if (WiFi.status() != WL_CONNECTED)
    {
      goToSleep();
    }
  }

  // Reconnect MQTT with timeout
  if (!mqttClient.connected())
  {
    if (!connectMqtt(MQTT_RECONNECT_TIMEOUT)) {
      goToSleep();
    }
  }

  mqttClient.loop();

  if (millis() - lastBlink >= BLINK_INTERVAL)
  {
    lastBlink = millis();
#ifdef BUILT_IN_LED_ENABLED
    digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
#endif
    indicate_with_fade(LOOP_COLOR);
  }
}

// --- Program End

// ---- WiFi helpers ----

bool connectWifiFast()
{
  WiFi.mode(WIFI_STA);
  // Re-apply the cached IP so we skip DHCP entirely.
  IPAddress ip(rtcWifi.local_ip);
  IPAddress gw(rtcWifi.gateway);
  IPAddress sn(rtcWifi.subnet);
  IPAddress dns(rtcWifi.dns);
  WiFi.config(ip, gw, sn, dns);
  // Pass channel + BSSID to skip the scan.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, rtcWifi.channel, rtcWifi.bssid);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_FAST)
  {
    delay(5);
  }
  return WiFi.status() == WL_CONNECTED;
}

bool connectWifiFull()
{
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);
  // Clear any static IP config from the fast attempt — fall back to DHCP.
  WiFi.config(IPAddress((uint32_t)0), IPAddress((uint32_t)0),
              IPAddress((uint32_t)0), IPAddress((uint32_t)0));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_FULL)
  {
    delay(5);
  }
  return WiFi.status() == WL_CONNECTED;
}

void cacheWifiState()
{
  uint8_t *bssid = WiFi.BSSID();
  if (bssid) {
    memcpy(rtcWifi.bssid, bssid, 6);
  }
  rtcWifi.channel  = WiFi.channel();
  rtcWifi.local_ip = (uint32_t)WiFi.localIP();
  rtcWifi.gateway  = (uint32_t)WiFi.gatewayIP();
  rtcWifi.subnet   = (uint32_t)WiFi.subnetMask();
  rtcWifi.dns      = (uint32_t)WiFi.dnsIP();
  rtcWifi.valid    = true;
}

// ---- MQTT helper ----

bool connectMqtt(unsigned long timeout_ms)
{
  unsigned long t0 = millis();
  while (!mqttClient.connected() && millis() - t0 < timeout_ms)
  {
    // LWT: if we drop without a clean disconnect, broker marks us offline.
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD,
                           AVAILABILITY_TOPIC, 1, true, "offline"))
    {
      return true;
    }
    delay(50);
  }
  return mqttClient.connected();
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  bool updated = false;
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error)
    return;

  if (doc.containsKey("resetDiscovery") && doc["resetDiscovery"] == true)
  {
    handleResetDiscovery();
    return;
  }

  if (doc.containsKey("preventSleep"))
  {
    preventSleep = doc["preventSleep"];
    updated = true;
  }

  if (doc.containsKey("color"))
  {
    JsonObject colorObj = doc["color"].as<JsonObject>();
    if (colorObj.containsKey("r")) messageColor.r = colorObj["r"].as<uint8_t>();
    if (colorObj.containsKey("g")) messageColor.g = colorObj["g"].as<uint8_t>();
    if (colorObj.containsKey("b")) messageColor.b = colorObj["b"].as<uint8_t>();
    updated = true;
  }

  if (doc.containsKey("brightness"))
  {
    int b = doc["brightness"].as<int>();
    messageBrightness = constrain(b, 0, 255);
    updated = true;
  }

  if (updated)
  {
    saveNewValues(messageColor, messageBrightness);
    write_led_color(messageColor, messageBrightness);
    indicate_with_fade(messageColor);
    publishState();
  }
}

float checkBatteryLevel()
{
  float voltage = analogReadMilliVolts(BATT_ADC_PIN) * 2.0 * VOLTAGE_CALIBRATION_FACTOR / 1000.0;
  float percentage = (voltage - 3.0) * 100.0 / (4.2 - 3.0);
  return constrain(percentage, 0, 100);
}

void write_led_color(Color color, int brightness)
{
  float factor = brightness / 255.0;
  statusLed.setPixelColor(0, color.r * factor, color.g * factor, color.b * factor);
  statusLed.show();
}

#ifdef SERIAL_DEBUG_ENABLED
void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:    Serial.println("Wakeup: EXT0"); break;
  case ESP_SLEEP_WAKEUP_EXT1:    Serial.println("Wakeup: EXT1"); break;
  case ESP_SLEEP_WAKEUP_TIMER:   Serial.println("Wakeup: timer"); break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:Serial.println("Wakeup: touch"); break;
  case ESP_SLEEP_WAKEUP_ULP:     Serial.println("Wakeup: ULP"); break;
  default: Serial.printf("Wakeup: not deep sleep (%d)\n", wakeup_reason); break;
  }
}
#endif

void indicate_with_fade(Color color)
{
  unsigned long startTime = millis();
  while (millis() - startTime < FADE_DURATION)
  {
    float progress = (float)(millis() - startTime) / FADE_DURATION;
    float brightness = sin(progress * PI) * messageBrightness;
    brightness = constrain(brightness, 0, messageBrightness);
    write_led_color(color, brightness);
    delay(1);
  }
  statusLed.clear();
  statusLed.show();
}

void turn_off_led()
{
  statusLed.clear();
  statusLed.show();
  pinMode(NEOPIXEL_PIN, OUTPUT);
  digitalWrite(NEOPIXEL_PIN, LOW);
}

void goToSleep()
{
  turn_off_led();
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK(BUTTON_PIN), ESP_EXT1_WAKEUP_ANY_HIGH);

  // Clean shutdown: mark offline, flush, disconnect.
  if (mqttClient.connected()) {
    mqttClient.publish(AVAILABILITY_TOPIC, "offline", /*retain=*/true);
    mqttClient.loop();
    delay(PUBLISH_FLUSH_DELAY);
    mqttClient.disconnect();
  }
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();

#ifdef BUILT_IN_LED_ENABLED
  digitalWrite(BUILTIN_LED_PIN, LOW);
#endif

  esp_deep_sleep_start();
}

void loadSavedValues()
{
  prefs.begin("esping", false);
  messageBrightness = prefs.getUChar("brightness", DEFAULT_LED_BRIGHTNESS);
  messageColor.r = prefs.getUChar("r", DEF_MESS_COLOR_R);
  messageColor.g = prefs.getUChar("g", DEF_MESS_COLOR_G);
  messageColor.b = prefs.getUChar("b", DEF_MESS_COLOR_B);
  prefs.end();
}

void saveNewValues(Color messageColor, uint8_t messageBrightness)
{
  prefs.begin("esping", false);
  prefs.putUChar("brightness", messageBrightness);
  prefs.putUChar("r", messageColor.r);
  prefs.putUChar("g", messageColor.g);
  prefs.putUChar("b", messageColor.b);
  prefs.end();
}

void publishState()
{
  StaticJsonDocument<256> doc;
  doc["preventSleep"] = preventSleep;
  JsonObject colorObj = doc.createNestedObject("color");
  colorObj["r"] = messageColor.r;
  colorObj["g"] = messageColor.g;
  colorObj["b"] = messageColor.b;
  doc["brightness"] = messageBrightness;
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
