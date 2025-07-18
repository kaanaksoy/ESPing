#include "drivers.h"
#include "state.h"



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

