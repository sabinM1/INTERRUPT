#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "Config.h"
#include <SoftwareWire.h>

// Direction Constants
constexpr int FRONT_LEFT = 0;
constexpr int FRONT_RIGHT = 1;
constexpr int SIDE_LEFT = 2;
constexpr int SIDE_RIGHT = 3;
constexpr int CENTER = 4;
constexpr int BOTH_SIDES = 5;

namespace Sensors {
  extern bool js40fStates[4]; // Exposed for debug access

  bool isDebugModeEnabled();
  bool isLineInvertEnabled();

  void init();
  void update(unsigned long currentTime);

  bool isCenterLineDetected();

  // IR Start Module (Protocol: START-STOP.md)
  bool isIRModuleConnected();
  bool hasMatchStarted();
  bool shouldStopRobot();
  void resetStartStopState();
  void handleIRStartSequence();
  void handleIRStopSequence();

  // Opponent Logic
  bool isOpponentDetected();
  int getOpponentDirection();  // Returns -1 if none, else 0-4 (see constants)
  int getOpponentPosition();   // Returns sensor index

  // JS40F Digital Sensors
  bool isJS40FDetected(int sensorIndex); // 0=FL, 1=FR, 2=SL, 3=SR
  bool isAnyJS40FDetected();
  int getJS40FDetectionPattern(); // Bitmask output

  // VL53L0X ToF
  bool isVL53L0XReady();
  uint16_t getVL53L0XDistance(); // mm (0 if out of range)
  bool isVL53L0XDisabled();
  bool isOpponentInFrontCenter();

  // Internal Processing
  void processJS40Sensors(unsigned long currentTime);
  void processVL53Sensor(unsigned long currentTime);
  void processLineSensors();
}

#endif
