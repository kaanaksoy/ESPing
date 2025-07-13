#ifndef DEFINES_H
#define DEFINES_H

// User Configurable Settings
#define DEVICE_NAME "ESPing01-1" // Name of the device as it appears in MQTT
#define DEVICE_ID "esping01-1"   // Make sure -x is unique
// #define DISCOVERY_ENABLED        // Set to false to disable MQTT discovery
// #define BUILT_IN_LED_ENABLED // Enable this to use the built-in LED on the Beetle ESP32-C6 V1.0

// Basic Pin Definitions
#define BUTTON_PIN GPIO_NUM_4
#define BUILTIN_LED_PIN GPIO_NUM_15
#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO) // 2 ^ GPIO_NUMBER in hex

// MQTT Configuration
#define MQTT_CLIENT_ID DEVICE_ID
#define MQTT_BUFFER_SIZE 1024
#define TOPIC_BASE DEVICE_ID
#define BUTTON_TOPIC TOPIC_BASE "/button"
#define STATE_TOPIC TOPIC_BASE "/state"
#define AVAILABILITY_TOPIC TOPIC_BASE "/availability"
#define COMMAND_TOPIC TOPIC_BASE "/command"
#define BATTERY_TOPIC TOPIC_BASE "/battery"
#define DISCOVERY_PREFIX "homeassistant" // Make sure this matches your HASS MQTT config
#define SW_VERSION "0.2.0"               // Software version for discovery
#define HW_VERSION "0.1"                 // Hardware version for discovery
#define WIFI_TIMEOUT 5000
#define MQTT_TIMEOUT 5000

// WS2812 LED strip
#define NEOPIXEL_PIN GPIO_NUM_21                                                     // Regular GPIO
#define NUM_LEDS 1                                                                   // Number of LEDs in the strip
#define FADE_DURATION 200                                                            // Time in ms for the fade effect
#define DEFAULT_LED_BRIGHTNESS 255                                                   // Max brightness of the LED strip (0-255)
#define LOOP_COLOR {0, 5, 255}                                                       // Color for the loop indication
#define DEF_MESS_COLOR_R 33                                                          // Default red value for the message color
#define DEF_MESS_COLOR_G 255                                                         // Default green value for the message color
#define DEF_MESS_COLOR_B 30                                                          // Default blue value for the message color
#define DEFAULT_MESSAGE_COLOR {DEF_MESS_COLOR_R, DEF_MESS_COLOR_G, DEF_MESS_COLOR_B} // Default color for the message indication
// Battery Monitoring
#define BATT_ADC_PIN 0                    // ADC pin for the battery monitoring (For the Beetle ESP32-C6 V1.0 it's GPIO 0)
#define VOLTAGE_CALIBRATION_FACTOR 1.0186 // Calibration factor for the battery voltage (differentiates between microcontrollers)
#define uS_TO_S_FACTOR 1000000ULL         /* Conversion factor for micro seconds to seconds */

// #define SERIAL_DEBUG_ENABLED // Enable this to use the serial debug output
// DEBUGGING
#endif
