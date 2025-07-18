#include "flash_driver.h"

void saveNewColor()
{
    if (!state.colorMessageReceived) return;
    state.colorMessageReceived = false;

    prefs.begin("esping", false);
    if (state.messageColor.containsKey("r"))
      prefs.putUChar("r", state.ledColor.r);
    if (state.messageColor.containsKey("g"))
      prefs.putUChar("g", state.ledColor.g);
    if (state.messageColor.containsKey("b"))
      prefs.putUChar("b", state.ledColor.b);
  prefs.end();

#ifdef SERIAL_DEBUG_ENABLED
  Serial.printf("Saved brightness: %d\n", state.messageBrightness);
  Serial.printf("Saved color: R=%d G=%d B=%d\n", messageColor.r, messageColor.g, messageColor.b);
#endif
}

void saveNewBrightness()
{
    if (!state.brightnessMessageReceived) return;
    state.brightnessMessageReceived = false;

    prefs.begin("esping", false);
}

void loadSavedConfig()
{
    if (state.savedConfigLoaded) return;
    state.savedConfigLoaded = true;

    prefs.begin("esping", false);
    state.messageBrightness = prefs.getUChar("brightness", DEFAULT_LED_BRIGHTNESS);
    state.ledColor.r = prefs.getUChar("r", DEF_MESS_COLOR_R);
    state.ledColor.g = prefs.getUChar("g", DEF_MESS_COLOR_G);
    state.ledColor.b = prefs.getUChar("b", DEF_MESS_COLOR_B);
    state.ledBrightness = prefs.getUChar("brightness", DEFAULT_LED_BRIGHTNESS);
    prefs.end();

#ifdef SERIAL_DEBUG_ENABLED
  Serial.printf("Loaded brightness: %d\n", state.messageBrightness);
  Serial.printf("Loaded color: R=%d G=%d B=%d\n", state.ledColor.r, state.ledColor.g, state.ledColor.b);
#endif
}
