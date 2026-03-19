#include "SoilSensor.h"

SoilSensor::SoilSensor(int pin, int dryValue, int wetValue)
  : _pin(pin), _dry(dryValue), _wet(wetValue) {}

void SoilSensor::begin() {
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
}

int SoilSensor::readRaw() {
  int raw = 0;
  for (int i = 0; i < 10; i++) {
    raw += analogRead(_pin);
    delay(10);
  }
  return raw / 10;
}

int SoilSensor::readPercent() {
  int raw = readRaw();
  int percent = map(raw, _dry, _wet, 0, 100);
  percent = constrain(percent, 0, 100);

  Serial.print("Pin ");
  Serial.print(_pin);
  Serial.print(" | ADC: ");
  Serial.print(raw);
  Serial.print("  |  Soil humidity: ");
  Serial.print(percent);
  Serial.println(" %");

  return percent;
}