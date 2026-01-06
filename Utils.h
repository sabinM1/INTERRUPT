#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include "Config.h"

namespace Utils {
  void init();

  // Periodic tasks (LED, Serial)
  void update(unsigned long currentTime);

  // LED Indicators
  void setStatusLED(bool state);
  void blinkStatusLED(unsigned long interval);

  // Telemetry
  void printStatus();
}

#endif
