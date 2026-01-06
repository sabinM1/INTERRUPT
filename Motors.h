#ifndef MOTORS_H
#define MOTORS_H

#include <Arduino.h>
#include <xmotionV3.h>
#include "Config.h"

namespace Motors {
  void init();

  // Handle acceleration ramp and hardware output
  void update(unsigned long currentTime);

  // Range: -100 to 100 (Percentage)
  void setTargetSpeed(int left, int right);

  int getActualLeftSpeed();
  int getActualRightSpeed();

  void stop();
}

#endif
