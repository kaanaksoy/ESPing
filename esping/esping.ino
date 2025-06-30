#include <WiFi.h>
#include <PubSubClient.h>

#define LED_PIN         15
#define MQTT_CLIENT_ID  "esping-01"
#define STATE_TOPIC     "esping/state"
#define SLEEP_TOPIC     "esping/prevent-sleep"
#define WIFI_TIMEOUT    5000

#if __has_include("credentials.h")
#include "credentials.h"
#else
#include "credentials_example.h"
#endif

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

bool preventSleep = false;
unsigned long lastBlink = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  preventSleep = (length > 0 && payload[0] == '1');
}


void goToSleep() {
  mqttClient.disconnect();
  WiFi.disconnect(true);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT) {
    delay(5);
  }

  if (WiFi.status() != WL_CONNECTED) {
    goToSleep();
  }

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setKeepAlive(10);

  while (!mqttClient.connected()) {
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
        delay(25);
        mqttClient.publish(STATE_TOPIC, "pressed", /*retain=*/true);
    } 
    else {
      delay(50);
    }
  }  
  delay(1000);
  mqttClient.publish(STATE_TOPIC, "released", /*retain=*/true);

  delay(50);

  mqttClient.setCallback(callback);
  mqttClient.subscribe(SLEEP_TOPIC, 1);

  // Wait briefly to allow incoming messages
  unsigned long checkTime = millis();
  while (millis() - checkTime < 1000) {
    mqttClient.loop();
    delay(25);
  }
}

void loop() {
  if (!preventSleep) {
    goToSleep();
  }

  if (!mqttClient.connected()) {
    mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
    mqttClient.subscribe(SLEEP_TOPIC, 1);
  }

  mqttClient.loop();

  if (millis() - lastBlink >= 500) {
    lastBlink = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}
