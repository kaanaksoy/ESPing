#include "battery_driver.h"

void setupBattery()
{
  pinMode(BATT_ADC_PIN, INPUT);
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
