#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Configuration defines (replace DIP switches)
#define VL_ENABLED true           // Set to false to disable VL53L0X sensor
#define DEBUG_MODE_ENABLED true  // Set to true to enable debug mode
#define TEST_OPENER_ONLY false    // Set to true to test only opener strategy then stop
#define LINE_INVERT false         // Set to true to invert line sensor logic (HIGH = line detected)


namespace Config {
  namespace Pins {
    // I/O
    constexpr uint8_t CENTER_LINE     = A2;
    constexpr uint8_t IR_START_MODULE = A0;
    constexpr uint8_t TRIMPOT         = A3;
    constexpr uint8_t STATUS_LED      = 9;

    // Strategy Selection (formerly DIP switches)
    constexpr uint8_t STRATEGY_1 = 5; // Strategy bit 1
    constexpr uint8_t STRATEGY_2 = 6; // Strategy bit 2
    constexpr uint8_t STRATEGY_3 = 7; // Strategy bit 3

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
    constexpr unsigned long SEEK_PATTERNS       = 1200;
    constexpr unsigned long OPPONENT_LOST_TIMEOUT = 500;
    constexpr unsigned long LINE_SENSOR_DISPLAY = 50;

    // State transition timings (milliseconds)
    constexpr unsigned long ATTACK_LOST_TIMEOUT = 150;   // Time before exiting attack if opponent lost
    constexpr unsigned long MEMORY_PERSISTENCE = 400;    // How long to remember last opponent position
    constexpr unsigned long STALL_DETECTION = 800;       // Time before stall detection activates
    constexpr unsigned long CENTER_BOOST_WINDOW = 80;    // Time window for center boost after detection
    constexpr unsigned long ATTACK_POWER_LIMIT_TIME = 800; // Time before using full attack power

    // Opener Strategy Timing (milliseconds)
    constexpr unsigned long OPENER_MAX_DURATION = 1500;    // Maximum opener duration before fallback to SEARCH
    constexpr unsigned long OPENER_EARLY_EXIT_THRESHOLD = 50; // Minimum time in opener before opponent detection triggers attack

    // Strategy-specific early exit thresholds (50% of total duration)
    namespace OpenerEarlyExit {
      constexpr unsigned long STRATEGY_0 = 0;    // 0% of 0ms - NO STRATEGY
      constexpr unsigned long STRATEGY_1 = 450;  // 50% of 900ms (was 675ms at 75%)
      constexpr unsigned long STRATEGY_2 = 550;  // 50% of 1100ms (was 825ms at 75%)
      constexpr unsigned long STRATEGY_3 = 550;  // 50% of 1100ms (was 825ms at 75%)
      constexpr unsigned long STRATEGY_4 = 450;  // 50% of 900ms (was 675ms at 75%)
      constexpr unsigned long STRATEGY_5 = 500;  // 50% of 1000ms (was 750ms at 75%)
      constexpr unsigned long STRATEGY_6 = 400;  // 50% of 800ms (was 600ms at 75%)
      constexpr unsigned long STRATEGY_7 = 500;  // 50% of 1000ms (was 750ms at 75%)
    }
  }

  namespace Motor { // PWM (0-255)
    constexpr int MAX_ACCEL_DELTA = 25; // Soft start ramp for motor acceleration

    // State-specific speeds (0-100 scale, negative for reverse)
    namespace Speeds {
      // Search state: Robot scans for opponent
      constexpr int SEARCH_BASE = 12;        // Base speed when no recent opponent seen (50% of 25)
      constexpr int SEARCH_BOOSTED = 17;     // Speed when opponent recently detected (50% of 35)
      constexpr int SEARCH_CURVE_INSIDE = 10; // Inner wheel speed when curving toward last known position (50% of 25)
      constexpr int SEARCH_CURVE_OUTSIDE = 45; // Outer wheel speed when curving toward last known position (50% of 30)
      constexpr int SEARCH_WIGGLE_DIFF = 10;  // Speed difference for search wiggle pattern (increased for better turning sensitivity)

      // Tracking state: Gentle realignment toward opponent
      constexpr int TRACK_BASE = 15;          // Base tracking speed (both wheels)
      constexpr int TRACK_TURN_DIFF = 25;    // Speed difference between wheels for turning (increased for better turning sensitivity)

      // Attack state: Aggressive engagement with opponent
      constexpr int ATTACK_FULL = 100;       // Full forward speed for centered opponent
      constexpr int ATTACK_LIMITED = 75;     // Limited attack power after cooldown period
      constexpr int ATTACK_SIDE = 35;        // Speed when opponent detected on side sensors
      constexpr int ATTACK_FRONT_TURN = 90;  // Turning speed when opponent on front-left/right (increased for better turning sensitivity)
      constexpr int MEMORY_ATTACK_BONUS = 30; // Speed bonus for recently centered opponent

      // Both sides attack state: Opponent detected on both side sensors
      constexpr int BOTH_SIDES = 30;         // Forward speed when opponent on both sides

      // Retreat state: Edge avoidance and recovery
      constexpr int RETREAT_POWER = 100;     // Reverse power during retreat phase
      constexpr int RETREAT_TURN_POWER = 100; // Turning power during J-turn phase

      // Opener Strategy Speeds (0-100 scale) - AGGRESSIVE VERSION
      namespace Opener {
        constexpr int SLOW_SCAN_SPEED = 0;            // Strategy 0: NO STRATEGY - not used
        constexpr int PIVOT_SCAN_SPEED = 85;          // Strategy 1: Pivot rotation speed
        constexpr int CHARGE_SPEED = 100;             // Full charge speed for burst strategies
        constexpr int ARC_SPEED_INSIDE = 50;          // Strategy 2: Inside wheel speed for arc
        constexpr int ARC_SPEED_OUTSIDE = 75;         // Strategy 2: Outside wheel speed for arc
        constexpr int MICRO_SWEEP_SPEED = 60;         // Strategy 6: Micro-sweep rotation speed
        constexpr int REVERSE_BAIT_SPEED = 70;        // Strategy 4: Reverse bait speed
        constexpr int RAMP_START_SPEED = 50;          // Strategy 5: Ramp starting speed
        constexpr int ALIGNMENT_SPEED = 40;           // Final alignment speed for all strategies
        constexpr int WIGGLE_DIFFERENCE = 18;         // Wiggle difference for scanning strategies (increased for better turning sensitivity)
        constexpr int HARD_PIVOT_SPEED = 95;          // Hard pivot speed for aggressive turns
        constexpr int POWER_SLIDE_SPEED = 90;         // Power slide speed for dramatic movements
        constexpr int RAPID_OSCILLATION_SPEED = 80;   // Rapid oscillation speed
      }
    }
  }

  namespace Movement { // ms
    constexpr unsigned long RETREAT_DURATION = 320;
    constexpr unsigned long TURN_DURATION    = 105;

    // Opener Strategy Durations (milliseconds) - EXTENDED VERSION
    namespace Opener {
      // Strategy 0: NO STRATEGY - immediate transition to SEARCH
      constexpr unsigned long SLOW_SCAN_PHASE = 0;
      constexpr unsigned long SLOW_SCAN_TOTAL = 0;

      // Strategy 1: Quick pivot scan then charge
      constexpr unsigned long PIVOT_PHASE = 500;
      constexpr unsigned long CHARGE_PHASE = 300;
      constexpr unsigned long PIVOT_TOTAL = 900;

      // Strategy 2: Forward arc acquire
      constexpr unsigned long ARC_PHASE = 800;
      constexpr unsigned long STRAIGHTEN_PHASE = 250;
      constexpr unsigned long ARC_TOTAL = 1100;

      // Strategy 3: Delayed burst with front-check
      constexpr unsigned long HOLD_PHASE = 400;
      constexpr unsigned long PULSE_PHASE = 200;
      constexpr unsigned long BURST_PHASE = 400;
      constexpr unsigned long DELAYED_TOTAL = 1100;

      // Strategy 4: Reverse bait arc
      constexpr unsigned long REVERSE_PHASE = 300;
      constexpr unsigned long FORWARD_ARC_PHASE = 400;
      constexpr unsigned long BAIT_TOTAL = 900;

      // Strategy 5: Ramped charge with mid-correction
      constexpr unsigned long RAMP_PHASE = 500;
      constexpr unsigned long CORRECTION_PHASE = 100;
      constexpr unsigned long FULL_POWER_PHASE = 300;
      constexpr unsigned long RAMP_TOTAL = 1000;

      // Strategy 6: Double micro-sweep then burst
      constexpr unsigned long MICRO_SWEEP_PHASE = 200;
      constexpr unsigned long BURST_AFTER_SWEEP = 400;
      constexpr unsigned long MICRO_TOTAL = 800;

      // Strategy 7: Aggressive power slide (new)
      constexpr unsigned long POWER_SLIDE_PHASE = 400;
      constexpr unsigned long RECOVERY_PHASE = 200;
      constexpr unsigned long FINAL_CHARGE_PHASE = 300;
      constexpr unsigned long POWER_SLIDE_TOTAL = 1000;
    }
  }
}
#endif
