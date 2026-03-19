#pragma once
#include <Arduino.h>

class SoilSensor {
public:
  SoilSensor(int pin, int dryValue = 2600, int wetValue = 830);
  void begin();
  int readPercent();
  int readRaw();

private:
  int _pin;
  int _dry;
  int _wet;
};