#include <WiFi.h>
#include "esp_wifi.h"
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#include "driver/rtc_io.h"
#include "defines.h"
#include "globals.h"
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
State state;

// Function prototypes
void goToSleep();

// --- Program Begin
void setup()
{
#ifdef SERIAL_DEBUG_ENABLED
  Serial.begin(115200);
  delay(1000); // Take some time to open up the Serial Monitor
  Serial.println("Booting ESPing... With a 1 second delay");
#endif

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
    state.buttonPressedMessagePending = true;
  }

}

void loop()
{



  mqttClient.loop();


// --- Program End





// Function to put the device into deep sleep
// It turns off the LED, disconnects from MQTT, and WiFi
// It also sets the wakeup source to an external signal on BUTTON_PIN
// The function uses the ESP-IDF API to configure the wakeup source
// It disables pull-up on the button pin and enables pull-down
// Finally, it starts deep sleep mode, which will wake up when the button is pressed
void goToSleep()
{
  if (state.preventSleep || state.buttonPressedMessagePending) return;
  turn_off_led();
  turnOffNetworking();
  esp_deep_sleep_start();
}
