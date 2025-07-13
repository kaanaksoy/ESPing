#include "drivers.h"
#include "state.h"

void setupBattery()
{
  pinMode(BATT_ADC_PIN, INPUT);
}

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

void setupLed()
{
  statusLed.begin();
  statusLed.clear();
  statusLed.show();

#ifdef BUILT_IN_LED_ENABLED
  pinMode(BUILTIN_LED_PIN, OUTPUT);
  digitalWrite(BUILTIN_LED_PIN, HIGH);
#endif // BUILT_IN_LED_ENABLED
return;
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


// Function to indicate a color with a fade effect
// It takes a Color struct and fades the LED strip to that color over a specified duration
void indicate_with_fade(Color color) {
    state.fadeState.isActive = true;
    state.fadeState.startTime = millis();
    state.fadeState.targetColor = color;
  }

// New function to handle fade updates - call this in your main loop
void update_fade() {
    if (!state.fadeState.isActive) {
      return;
    }
  
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - state.fadeState.startTime;
  
    if (elapsedTime >= FADE_DURATION) {
      // Animation complete
      state.fadeState.isActive = false;
      statusLed.clear();
      statusLed.show();
      return;
    }
  
    float progress = (float)elapsedTime / FADE_DURATION;
    float brightness = sin(progress * PI) * messageBrightness;
    brightness = constrain(brightness, 0, messageBrightness);
    write_led_color(state.fadeState.targetColor, brightness);
}

// We need to actively turn off the LED when going to sleep to prevent leakage current on the data line from draining the battery
void turn_off_led()
{
  statusLed.clear(); // Set all pixels to 'off'
  statusLed.show();  // Update the strip

  // Now explicitly set the pin low
  pinMode(NEOPIXEL_PIN, OUTPUT);
  digitalWrite(NEOPIXEL_PIN, LOW);
#ifdef BUILT_IN_LED_ENABLED
  // Turn off the built-in LED
  digitalWrite(BUILTIN_LED_PIN, LOW);
#endif
}

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
