#ifndef GLOBALS_H
#define GLOBALS_H

#include "defines.h"
#include <ArduinoJson.h>


struct State
{
  bool wifiConnected = false;
  bool mqttConnected = false;
  bool mqttSubscribed = false;
  bool availabilityPublished = false;

  bool discoverySent = false;
  bool buttonPressedMessagePending = false;
  bool buttonReleasedMessagePending = false;
  bool preventSleep = false;
  bool colorMessageReceived = false;
  bool brightnessMessageReceived = false;
  bool resetDiscoveryMessageReceived = false;
  bool savedConfigLoaded = false;
  unsigned long wifiConnectionStartTime = 0;
  unsigned long mqttConnectionStartTime = 0;
  unsigned long buttonMessageCooldown = 0;
  JsonObject messageColor;
  Color ledColor;
  uint8_t ledBrightness;
  uint8_t messageBrightness;
  FadeState fadeState;
};

#endif // GLOBALS_H
