#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <Preferences.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <PubSubClient.h>
#include "defines.h"

#if __has_include("credentials.h")
#include "credentials.h"
#else
#include "credentials_example.h"
#endif

extern Preferences prefs;
extern PubSubClient mqttClient;

void checkAndSendDiscovery();
void handleResetDiscovery();
void sendDiscovery();

#endif // DISCOVERY_H
