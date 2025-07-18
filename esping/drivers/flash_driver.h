#ifndef FLASH_DRIVER_H
#define FLASH_DRIVER_H

#include <Preferences.h>
#include "state.h"
#include "defines.h"

extern Preferences prefs;

void saveNewColor();
void saveNewBrightness();
void loadSavedConfig();

#endif // FLASH_DRIVER_H
