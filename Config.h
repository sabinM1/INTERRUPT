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
    constexpr float DEBUG_SPEED_MULTIPLIER = 0.4;
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

    // State transition timings (milliseconds)
    constexpr unsigned long ATTACK_LOST_TIMEOUT = 200;   // Time before exiting attack if opponent lost
    constexpr unsigned long MEMORY_PERSISTENCE = 500;    // How long to remember last opponent position
    constexpr unsigned long STALL_DETECTION = 1000;      // Time before stall detection activates
    constexpr unsigned long CENTER_BOOST_WINDOW = 100;   // Time window for center boost after detection
    constexpr unsigned long ATTACK_POWER_LIMIT_TIME = 1000; // Time before using full attack power
  }

  namespace Motor { // PWM (0-255)
    constexpr int MAX_ACCEL_DELTA = 15; // Soft start ramp for motor acceleration

    // State-specific speeds (0-100 scale, negative for reverse)
    namespace Speeds {
      // Search state: Robot scans for opponent
      constexpr int SEARCH_BASE = 10;        // Base speed when no recent opponent seen
      constexpr int SEARCH_BOOSTED = 30;     // Speed when opponent recently detected
      constexpr int SEARCH_CURVE_INSIDE = 10; // Inner wheel speed when curving toward last known position
      constexpr int SEARCH_CURVE_OUTSIDE = 35; // Outer wheel speed when curving toward last known position
      constexpr int SEARCH_WIGGLE_DIFF = 10;  // Speed difference for search wiggle pattern

      // Tracking state: Gentle realignment toward opponent
      constexpr int TRACK_BASE = 20;         // Base tracking speed (both wheels)
      constexpr int TRACK_TURN_DIFF = 10;    // Speed difference between wheels for turning

      // Attack state: Aggressive engagement with opponent
      constexpr int ATTACK_FULL = 100;       // Full forward speed for centered opponent
      constexpr int ATTACK_LIMITED = 65;     // Limited attack power after cooldown period
      constexpr int ATTACK_SIDE = 20;        // Speed when opponent detected on side sensors
      constexpr int ATTACK_FRONT_TURN = 70;  // Turning speed when opponent on front-left/right
      constexpr int MEMORY_ATTACK_BONUS = 25; // Speed bonus for recently centered opponent

      // Both sides attack state: Opponent detected on both side sensors
      constexpr int BOTH_SIDES = 20;         // Forward speed when opponent on both sides

      // Retreat state: Edge avoidance and recovery
      constexpr int RETREAT_POWER = 100;     // Reverse power during retreat phase
      constexpr int RETREAT_TURN_POWER = 100; // Turning power during J-turn phase
    }
  }

  namespace Movement { // ms
    constexpr unsigned long RETREAT_DURATION = 320;
    constexpr unsigned long TURN_DURATION    = 105;
  }
}
#endif
