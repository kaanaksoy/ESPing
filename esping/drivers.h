#ifndef DRIVERS_H
#define DRIVERS_H

#include "defines.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include "state.h"


struct Color
{
  uint8_t r, g, b;
};

extern Adafruit_NeoPixel statusLed;
extern unsigned long lastBlink;

float checkBatteryLevel();
void setupBattery();
void write_led_color(Color color, int brightness);
void indicate_with_fade(Color color);
void turn_off_led();

#endif // DRIVERS_H 
