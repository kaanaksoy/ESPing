#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H

#include "state.h"
#include "defines.h"

void setupButton();
void buttonPressISR();
void enable_button_for_sleep();

extern State state;

#endif // BUTTON_DRIVER_H
