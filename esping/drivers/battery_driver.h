#ifndef BATTERY_DRIVER_H
#define BATTERY_DRIVER_H

#include "defines.h"
#include "state.h"

void setupBattery();
float checkBatteryLevel();

extern State state;


#endif // BATTERY_DRIVER_H
