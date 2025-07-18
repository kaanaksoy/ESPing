#include "button_driver.h"

void setupButton()
{
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonPressISR, HIGH);
}

void buttonPressISR()
{
  state.buttonPressedMessagePending = true;
  return;
}

void enable_button_for_sleep()
{
    esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK(BUTTON_PIN), ESP_EXT1_WAKEUP_ANY_HIGH); // ESP32-C6 only supports EXT1
}
