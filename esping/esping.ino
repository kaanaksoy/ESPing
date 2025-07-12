#include <WiFi.h>
#include "esp_wifi.h"
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

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

struct Color
{
  uint8_t r, g, b;
};

Color messageColor = DEFAULT_MESSAGE_COLOR;
uint8_t messageBrightness = DEFAULT_LED_BRIGHTNESS; // Default brightness for the message indication

bool preventSleep = false;
unsigned long lastBlink = 0;

// Function prototypes
void mqttCallback(char *topic, byte *payload, unsigned int length);
void print_wakeup_reason();
void indicate_with_fade(Color color);
void write_led_color(Color color, int brightness);
void goToSleep();
void turn_off_led();
void loadSavedValues();
void saveNewValues(Color messageColor, uint8_t messageBrightness);

// --- Program Begin
void setup()
{
#ifdef SERIAL_DEBUG_ENABLED
  Serial.begin(115200);
  delay(1000); // Take some time to open up the Serial Monitor

  // Increment boot number and print it every reboo
  // Print the wakeup reason for ESP32
  print_wakeup_reason();
  Serial.println("Booting ESPing...");
#endif

#ifdef BUILT_IN_LED_ENABLED
  pinMode(BUILTIN_LED_PIN, OUTPUT);
  digitalWrite(BUILTIN_LED_PIN, HIGH);
#endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT)
  {
    delay(5);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    goToSleep();
  }

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setKeepAlive(10);
  mqttClient.setBufferSize(1024);
  statusLed.begin();
  statusLed.clear();
  statusLed.show();
  while (!mqttClient.connected())
  {
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD))
    {
      delay(25);
      mqttClient.publish(BUTTON_TOPIC, "pressed", true);
    }
    else
    {
      delay(50);
    }
  }
  delay(1000);
  mqttClient.publish(BUTTON_TOPIC, "released", /*retain=*/true);

  mqttClient.setCallback(mqttCallback);
  mqttClient.subscribe(COMMAND_TOPIC, 1);
  checkAndSendDiscovery();
  loadSavedValues();
  indicate_with_fade(messageColor);
  checkAndPublishBattery();
  delay(50);

  // Wait briefly to allow incoming messages
  unsigned long start = millis();
  while (!preventSleep && millis() - start < 5000)
  {
    mqttClient.loop();
    delay(25);
  }
}

void loop()
{
  if (!preventSleep)
  {
    goToSleep();
  }

  while (!mqttClient.connected())
  {
    mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
    delay(100);
  }

  mqttClient.publish(AVAILABILITY_TOPIC, "online", true);
  // publishState();
  mqttClient.loop();

  if (millis() - lastBlink >= 500)
  {
    lastBlink = millis();
#ifdef BUILT_IN_LED_ENABLED
    digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
#endif
    indicate_with_fade(LOOP_COLOR);
  }
}

// --- Program End

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
    preventSleep = doc["preventSleep"];
    updated = true;
  }

  if (doc.containsKey("color"))
  {
    JsonObject colorObj = doc["color"].as<JsonObject>();

    if (colorObj.containsKey("r"))
      messageColor.r = colorObj["r"].as<uint8_t>();
    if (colorObj.containsKey("g"))
      messageColor.g = colorObj["g"].as<uint8_t>();
    if (colorObj.containsKey("b"))
      messageColor.b = colorObj["b"].as<uint8_t>();

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

// Function to check the battery level
// It reads the ADC value from the battery pin, calculates the voltage,
// and then calculates the percentage based on the voltage range
// The voltage is calibrated using a factor to account for differences in microcontrollers
// The percentage is constrained between 0 and 100
// The function returns the battery percentage as a float
float checkBatteryLevel()
{
  float voltage = analogReadMilliVolts(BATT_ADC_PIN) * 2.0 * VOLTAGE_CALIBRATION_FACTOR / 1000.0;
  float percentage = (voltage - 3.0) * 100.0 / (4.2 - 3.0);
  return constrain(percentage, 0, 100);
}

// Function to write a color to the LED strip
// It takes a Color struct and a brightness value (0-255)
// It calculates the brightness factor based on the provided brightness
// and sets the pixel color accordingly
// Finally, it updates the LED strip to show the new color
// The color is set using the NeoPixel library's setPixelColor method
// The brightness is applied by scaling the RGB values based on the brightness factor
void write_led_color(Color color, int brightness)
{
  float factor = brightness / 255.0;
  statusLed.setPixelColor(0, color.r * factor, color.g * factor, color.b * factor);
  statusLed.show();
}

// Function to print the wakeup reason for the ESP32
// It retrieves the wakeup cause using esp_sleep_get_wakeup_cause()
// and prints a message based on the cause
// The function uses a switch statement to handle different wakeup causes
// It prints a message to the Serial Monitor indicating the cause of the wakeup
// If the wakeup cause is not recognized, it prints a default message
// This is useful for debugging and understanding how the device woke up from deep sleep
void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

// Function to indicate a color with a fade effect
// It takes a Color struct and fades the LED strip to that color over a specified duration
void indicate_with_fade(Color color)
{
  unsigned long startTime = millis();
  while (millis() - startTime < FADE_DURATION)
  {
    float progress = (float)(millis() - startTime) / FADE_DURATION; // Calculate the progress of the fade
    float brightness = sin(progress * PI) * messageBrightness;      // Calculate the brightness of the fade
    brightness = constrain(brightness, 0, messageBrightness);       // Constrain the brightness to the DEFAULT_LED_BRIGHTNESS

    write_led_color(color, brightness); // Write the color to the led
    delay(1);                           // Delay for 1ms
  }
  statusLed.clear(); // Set all pixels to 'off'
  statusLed.show();  // Update the strip
}

// We need to actively turn off the LED when going to sleep to prevent leakage current on the data line from draining the battery
void turn_off_led()
{
  statusLed.clear(); // Set all pixels to 'off'
  statusLed.show();  // Update the strip

  // Now explicitly set the pin low
  pinMode(NEOPIXEL_PIN, OUTPUT);
  digitalWrite(NEOPIXEL_PIN, LOW);
}

// Function to put the device into deep sleep
// It turns off the LED, disconnects from MQTT, and WiFi
// It also sets the wakeup source to an external signal on BUTTON_PIN
// The function uses the ESP-IDF API to configure the wakeup source
// It disables pull-up on the button pin and enables pull-down
// Finally, it starts deep sleep mode, which will wake up when the button is pressed
void goToSleep()
{
  turn_off_led();
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK(BUTTON_PIN), ESP_EXT1_WAKEUP_ANY_HIGH);
  mqttClient.publish(AVAILABILITY_TOPIC, "offline", true);
  mqttClient.loop(); // Ensure the last message is sent before going to sleep
  delay(100);        // Give some time for the MQTT message to be sent
  mqttClient.disconnect();
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();

#ifdef BUILT_IN_LED_ENABLED
  // Turn off the built-in LED
  digitalWrite(BUILTIN_LED_PIN, LOW);
#endif

  esp_deep_sleep_start();
}

void loadSavedValues()
{
  prefs.begin("esping", false); // Namespace

  // Load saved values
  messageBrightness = prefs.getUChar("brightness", DEFAULT_LED_BRIGHTNESS);
  messageColor.r = prefs.getUChar("r", DEF_MESS_COLOR_R); // Default red value
  messageColor.g = prefs.getUChar("g", DEF_MESS_COLOR_G); // Default green value
  messageColor.b = prefs.getUChar("b", DEF_MESS_COLOR_B); // Default blue value

  prefs.end();

#ifdef SERIAL_DEBUG_ENABLED
  Serial.printf("Loaded brightness: %d\n", messageBrightness);
  Serial.printf("Loaded color: R=%d G=%d B=%d\n", messageColor.r, messageColor.g, messageColor.b);
#endif
}

void saveNewValues(Color messageColor, uint8_t messageBrightness)
{
  prefs.begin("esping", false);

  prefs.putUChar("brightness", messageBrightness);
  prefs.putUChar("r", messageColor.r);
  prefs.putUChar("g", messageColor.g);
  prefs.putUChar("b", messageColor.b);

  prefs.end();

#ifdef SERIAL_DEBUG_ENABLED
  Serial.printf("Saved brightness: %d\n", messageBrightness);
  Serial.printf("Saved color: R=%d G=%d B=%d\n", messageColor.r, messageColor.g, messageColor.b);
#endif
}

void publishState()
{
  StaticJsonDocument<128> doc;
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
