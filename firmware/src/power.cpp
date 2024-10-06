#include <Arduino.h>
#include "config.h"
#include <hardware/adc.h>

unsigned long power_last_check_at = 0;

void power_setup() {
  adc_init();
  adc_gpio_init(CC1_PIN);
  adc_gpio_init(CC2_PIN);
  adc_gpio_init(V_SENSE_PIN);
  adc_gpio_init(I_SENSE_PIN);
  power_last_check_at = millis();
}

float _get_voltage(uint8_t pin) {
  adc_select_input(pin - 26);
  uint16_t reading = adc_read();
  float voltage = (reading / 4096.0f) * REFERENCE_VOLTAGE;
  return voltage;
}

float power_get_bus_voltage() {
  return _get_voltage(V_SENSE_PIN);
}

float power_get_amps() {
  auto voltage = _get_voltage(I_SENSE_PIN);
  return voltage * CURRENT_FACTOR;
}

float power_get_max_amps() {
  auto cc1 = _get_voltage(CC1_PIN);
  auto cc2 = _get_voltage(CC2_PIN);
  auto cc = max(cc1, cc2);

  if (cc < 0.25) {
    Serial.println("Power: No CC detected!");
    return 0.5;
  } else if (cc1 >= 0.25 && cc2 >= 0.25) {
    Serial.println("Power: Invalid two CCs detected!");
    return 0.5;
  } else if (cc < 0.7) {
    return 0.5;
  } else if (cc < 1.31) {
    return 1.5;
  } else if (cc < 2.04) {
    return 3.0;
  } else {
    Serial.println("Power: Invalid CC, voltage too high");
    return 0.5;
  }
}

void power_loop() {
  auto now = millis();
  if (power_last_check_at + 100 < now) {
    power_last_check_at = now;

    auto bus_voltage = power_get_bus_voltage();
    auto amps = power_get_amps();
    auto max_amps = power_get_max_amps();

    Serial.print(amps);
    Serial.print("A / ");
    Serial.print(max_amps);
    Serial.print("A");
    Serial.print(" @ ");
    Serial.print(bus_voltage);
    Serial.println("V");
  }
}
