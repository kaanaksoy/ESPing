#ifndef NETWORKING_H
#define NETWORKING_H

#include <WiFi.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include "defines.h"
#include "globals.h"
#include "drivers.h"
#include "esp_wifi.h"
#include <ArduinoJson.h>

extern WiFiClient wifiClient;
extern PubSubClient mqttClient;

void setupWifi();
void setupMqtt();
void connectToWifi();
void connectToMqtt();
void loopNetworking();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void checkAndPublishBattery();
void publishState();

#endif // NETWORKING_H
