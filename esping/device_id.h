#ifndef DEVICE_ID_H
#define DEVICE_ID_H

#include <Arduino.h>

// Runtime-derived device identity. Populated once in initDeviceId() at boot.
// Derived from the chip's MAC, so every board gets a unique ID with zero
// per-device configuration — flash the same firmware on every unit.
extern String DEVICE_ID;
extern String DEVICE_NAME;

// Topic strings built from DEVICE_ID at boot.
extern String BUTTON_TOPIC;
extern String STATE_TOPIC;
extern String AVAILABILITY_TOPIC;
extern String COMMAND_TOPIC;
extern String BATTERY_TOPIC;

void initDeviceId();

#endif // DEVICE_ID_H
