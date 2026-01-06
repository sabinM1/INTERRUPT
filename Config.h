#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

namespace Config {
  namespace Pins {
    // I/O
    constexpr uint8_t CENTER_LINE     = A2;
    constexpr uint8_t IR_START_MODULE = A0;
    constexpr uint8_t TRIMPOT         = A3;
    constexpr uint8_t STATUS_LED      = 9;

    // DIP Settings
    constexpr uint8_t DIP_VL_DISABLE  = 5; // Disable ToF sensor
    constexpr uint8_t DIP_DEBUG_MODE  = 6; // 10% Speed + Serial
    constexpr uint8_t DIP_LINE_INVERT = 7;

    // JS40F Digital Sensors
    constexpr uint8_t JS40F_FRONT_LEFT  = 1;
    constexpr uint8_t JS40F_FRONT_RIGHT = 2;
    constexpr uint8_t JS40F_SIDE_LEFT   = 0;
    constexpr uint8_t JS40F_SIDE_RIGHT  = 4;

    // I2C
    constexpr uint8_t VL53L0X_SDA = A4;
    constexpr uint8_t VL53L0X_SCL = A5;
  }

  namespace Debug {
    constexpr float DEBUG_SPEED_MULTIPLIER = 0.1;
  }

  namespace Timing { // ms
    constexpr unsigned long SENSOR_READ         = 15;
    constexpr unsigned long MOTOR_UPDATE        = 5;
    constexpr unsigned long DEBUG_PRINT         = 100;
    constexpr unsigned long BUTTON_DEBOUNCE     = 50;
    constexpr unsigned long LED_BLINK           = 250;
    constexpr unsigned long SEEK_PATTERNS       = 1500;
    constexpr unsigned long OPPONENT_LOST_TIMEOUT = 750;
    constexpr unsigned long LINE_SENSOR_DISPLAY = 50;
  }

  namespace Motor { // PWM (0-255)
    constexpr int FORWARD_SPEED   = 100;
    constexpr int TURN_SPEED      = 100;
    constexpr int RETREAT_SPEED   = 100;
    constexpr int MAX_ACCEL_DELTA = 15; // Soft start ramp
    constexpr int SEEK_SPEED      = 70;
    constexpr int ATTACK_SPEED    = 125;
  }

  namespace Movement { // ms
    constexpr unsigned long RETREAT_DURATION = 150;
    constexpr unsigned long TURN_DURATION    = 250;
  }
}
#endif
