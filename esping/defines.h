#ifndef DEFINES_H
#define DEFINES_H

// User Configurable Settings
// DEVICE_ID is now derived at runtime from the chip's MAC (see device_id.cpp).
// DEVICE_PREFIX is the short tag used in topics:  esping-a1b2c3d4e5f6
// DEVICE_PREFIX_FRIENDLY is the human-readable prefix shown in HA.
#define DEVICE_PREFIX "esping"
#define DEVICE_PREFIX_FRIENDLY "ESPing"
#define DISCOVERY_ENABLED // Comment out to disable MQTT discovery
// #define BUILT_IN_LED_ENABLED // Enable this to use the built-in LED on the
// Beetle ESP32-C6 V1.0

// Basic Pin Definitions
#define BUTTON_PIN GPIO_NUM_4
#define BUILTIN_LED_PIN GPIO_NUM_15
#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO) // 2 ^ GPIO_NUMBER in hex

// MQTT Configuration
// MQTT_CLIENT_ID, topics, etc. are now built at runtime — see device_id.h
#define DISCOVERY_PREFIX                                                       \
  "homeassistant"          // Make sure this matches your HASS MQTT config
#define SW_VERSION "0.4.0" // Software version for discovery
#define HW_VERSION "0.2"   // Hardware version for discovery

// Timing — tuned for snappy + reliable press events
#define WIFI_TIMEOUT_FAST 2500 // Fast-reconnect attempt (cached BSSID/channel)
#define WIFI_TIMEOUT_FULL 8000 // Full connect fallback (scan + DHCP)
#define MQTT_CONNECT_TIMEOUT 3000 // Bounded MQTT connect (was unbounded — bug)
#define MQTT_RECONNECT_TIMEOUT 5000
#define MQTT_LOOP_WAIT 5000
#define BLINK_INTERVAL 500
#define PUBLISH_FLUSH_DELAY 50 // Let TCP bytes leave the NIC before sleep

// WS2812 LED strip
#define NEOPIXEL_PIN GPIO_NUM_21   // Regular GPIO
#define NUM_LEDS 1                 // Number of LEDs in the strip
#define FADE_DURATION 200          // Time in ms for the fade effect
#define DEFAULT_LED_BRIGHTNESS 255 // Max brightness of the LED strip (0-255)
#define LOOP_COLOR {0, 5, 255}     // Color for the loop indication
#define DEF_MESS_COLOR_R 33        // Default red value for the message color
#define DEF_MESS_COLOR_G 255       // Default green value for the message color
#define DEF_MESS_COLOR_B 30        // Default blue value for the message color
#define DEFAULT_MESSAGE_COLOR                                                  \
  {DEF_MESS_COLOR_R, DEF_MESS_COLOR_G, DEF_MESS_COLOR_B}                       \
  // Default color for the message indication

// Battery Monitoring
#define BATT_ADC_PIN                                                           \
  0 // ADC pin for the battery monitoring (For the Beetle ESP32-C6 V1.0 it's
    // GPIO 0)
#define VOLTAGE_CALIBRATION_FACTOR                                             \
  1.0186 // Calibration factor for the battery voltage (differentiates between
         // microcontrollers). For a fleet, consider per-unit calibration
         // stored in Preferences instead.
#define uS_TO_S_FACTOR                                                         \
  1000000ULL /* Conversion factor for micro seconds to seconds */

// #define SERIAL_DEBUG_ENABLED // Enable this to use the serial debug output
// DEBUGGING
#endif
