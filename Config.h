#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Configuration defines (replace DIP switches)
#define VL_ENABLED true           // Set to false to disable VL53L0X sensor
#define DEBUG_MODE_ENABLED false  // Set to true to enable debug mode (serial telemetry + speed limiting)
#define SERIAL_DEBUG_ONLY true   // Set to true to enable serial telemetry only — no speed limiting
#define TEST_OPENER_ONLY false    // Set to true to test opener strategy then stop (requires IR start module)
#define LINE_INVERT false         // Set to true to invert line sensor logic (HIGH = line detected)
// Double-check line detection for noise immunity vs speed:
// true = 2 consecutive reads required (reduces false positives, ~0.2ms latency with immediate second read)
// false = single read detection (maximum reaction speed, risk of EMI false triggers)
#define LINE_DOUBLE_CHECK false


namespace Config {
  namespace Pins {
    // I/O
    constexpr uint8_t CENTER_LINE     = A2;
    constexpr uint8_t IR_START_MODULE = A0;
    constexpr uint8_t TRIMPOT         = A3;
    constexpr uint8_t STATUS_LED      = 9;

    // Strategy Selection (formerly DIP switches)
    constexpr uint8_t STRATEGY_1 = 7; // Strategy bit 1
    constexpr uint8_t STRATEGY_2 = 6; // Strategy bit 2
    constexpr uint8_t STRATEGY_3 = 5; // Strategy bit 3

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
    constexpr unsigned long SEEK_PATTERNS       = 800;
    constexpr unsigned long OPPONENT_LOST_TIMEOUT = 350;
    constexpr unsigned long LINE_SENSOR_DISPLAY = 50;
    constexpr unsigned long SPIN_REVERSE_TIME   = 800; // Time to spin in one direction before reversing

    // State transition timings (milliseconds)
    constexpr unsigned long ATTACK_LOST_TIMEOUT = 100;   // Time before exiting attack if opponent lost
    constexpr unsigned long MEMORY_PERSISTENCE = 350;    // How long to remember last opponent position
    constexpr unsigned long STALL_DETECTION = 750;       // Time before stall detection activates
    constexpr unsigned long CENTER_BOOST_WINDOW = 150;    // Time window for center boost after detection
    constexpr unsigned long ATTACK_POWER_LIMIT_TIME = 800; // Time before using full attack power

    // Opener Strategy Timing (milliseconds)
    constexpr unsigned long OPENER_MAX_DURATION = 1500;    // Maximum opener duration before fallback to SEARCH
    constexpr unsigned long OPENER_EARLY_EXIT_THRESHOLD = 50; // Minimum time in opener before opponent detection triggers attack

    // Strategy-specific early exit thresholds (50% of total duration)
    namespace OpenerEarlyExit {
      constexpr unsigned long STRATEGY_0 = 0;    // 0% of 0ms - NO STRATEGY
      constexpr unsigned long STRATEGY_1 = 0; // No early exit for flank strategies
      constexpr unsigned long STRATEGY_2 = 0; // No early exit for flank strategies
      // Strategies 3-4: Rear hook strategies (no early exit)
      constexpr unsigned long STRATEGY_3 = 0; // No early exit for diagonal right dash + rear hook
      constexpr unsigned long STRATEGY_4 = 0; // No early exit for diagonal left dash + rear hook
      // Strategies 5-7: Placeholder (not used)
      constexpr unsigned long STRATEGY_5 = 0;
      constexpr unsigned long STRATEGY_6 = 0;
      constexpr unsigned long STRATEGY_7 = 0;
    }
  }

  namespace Motor { // PWM (0-255)
    constexpr int MAX_ACCEL_DELTA = 25; // Soft start ramp for motor acceleration

    // State-specific speeds (0-100 scale, negative for reverse)
    namespace Speeds {
      // Search state: Robot scans for opponent
      constexpr int SEARCH_BASE = 20;        // Base speed when no recent opponent seen (50% of 25)
      constexpr int SEARCH_BOOSTED = 25;     // Speed when opponent recently detected (50% of 35)
      constexpr int SEARCH_CURVE_INSIDE = 12; // Inner wheel speed when curving toward last known position (50% of 25)
      constexpr int SEARCH_CURVE_OUTSIDE = 45; // Outer wheel speed when curving toward last known position (50% of 30)
      constexpr int SEARCH_WIGGLE_DIFF = 10;  // Speed difference for search wiggle pattern (increased for better turning sensitivity)

      // Spin search: Aggressive rotation when no opponent detected
      constexpr int SPIN_SPEED = 60;         // Wheel speed for aggressive spin search

      // Tracking state: Gentle realignment toward opponent
      constexpr int TRACK_BASE = 25;          // Base tracking speed (both wheels)
      constexpr int TRACK_TURN_DIFF = 45;    // Speed difference between wheels for turning (increased for better turning sensitivity)

      // Attack state: Aggressive engagement with opponent
      constexpr int ATTACK_FULL = 100;       // Full forward speed for centered opponent
      constexpr int ATTACK_LIMITED = 65;     // Limited attack power after cooldown period
      constexpr int ATTACK_SIDE = 35;        // Speed when opponent detected on side sensors
      constexpr int ATTACK_FRONT_TURN = 70;  // Turning speed when opponent on front-left/right (increased for better turning sensitivity)
      constexpr int MEMORY_ATTACK_BONUS = 30; // Speed bonus for recently centered opponent

      // Both sides attack state: Opponent detected on both side sensors
      constexpr int BOTH_SIDES = 30;         // Forward speed when opponent on both sides

      // Retreat state: Edge avoidance and recovery
      constexpr int RETREAT_POWER = 100;     // Reverse power during retreat phase
      constexpr int RETREAT_TURN_POWER = 100; // Turning power during J-turn phase

      // Opener Strategy Speeds (0-100 scale) - AGGRESSIVE VERSION
      namespace Opener {
        constexpr int SLOW_SCAN_SPEED = 0;            // Strategy 0: NO_STRATEGY - not used
        // Flank strategy constants (shared by strategies 1 and 2)
        constexpr int HARD_PIVOT_SPEED = 100;         // Full speed for hard pivot turns
        constexpr int DASH_SPEED = 100;               // Maximum PWM for forward dash (100%, used by all strategies)
        // Strategy 5: Wide Sweep Charge
        constexpr int SWEEP_INNER_SPEED = 30;         // Inner wheel speed during arc (outer = DASH_SPEED)
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

      // Flank strategy timing constants (shared by strategies 1 and 2)
      constexpr unsigned long TURN_RIGHT_MS = 70;     // ~90° hard pivot turn right (180-250ms tunable)
      constexpr unsigned long TURN_LEFT_MS = 165;      // ~90° hard pivot turn left (180-250ms tunable)
      constexpr unsigned long DASH_MS = 220;           // Forward dash distance (120-250ms tunable)

      // Strategy 2 specific turn constants (independent tuning)
      constexpr unsigned long STRAT2_TURN_LEFT_MS = 70;   // ~90° left turn for Strategy 2 (quick like Strategy 1 right turn)
      constexpr unsigned long STRAT2_TURN_RIGHT_MS = 120; // ~90° right turn for Strategy 2 (longer like Strategy 1 left turn)

      // Hook strategy timing constants (for strategies 3 and 4)
      constexpr unsigned long HOOK_DASH_MS = 450;      // Dash distance for hook strategies (tunable)
      constexpr unsigned long HOOK_TURN_LEFT_MS = 170;  // ~135° left hook turn (320-420ms tunable)
      constexpr unsigned long HOOK_TURN_RIGHT_MS = 170; // ~135° right hook turn (320-420ms tunable)

      // Strategy 1: LOWER RIGHT FLANK
      constexpr unsigned long RIGHT_FLANK_TOTAL = TURN_RIGHT_MS + DASH_MS + TURN_LEFT_MS;

      // Strategy 2: LOWER LEFT FLANK
      constexpr unsigned long LEFT_FLANK_TOTAL = STRAT2_TURN_LEFT_MS + DASH_MS + STRAT2_TURN_RIGHT_MS;

      // Strategy 3: DIAGONAL RIGHT DASH + REAR HOOK
      constexpr unsigned long DIAGONAL_RIGHT_TOTAL = HOOK_DASH_MS + HOOK_TURN_LEFT_MS;

      // Strategy 4: DIAGONAL LEFT DASH + REAR HOOK
      constexpr unsigned long DIAGONAL_LEFT_TOTAL = HOOK_DASH_MS + HOOK_TURN_RIGHT_MS;

      // Strategy 5: WIDE SWEEP CHARGE (smooth arc right then full charge)
      constexpr unsigned long SWEEP_CURVE_MS = 350;   // Arc duration (L=100%, R=30%) sweeping right
      constexpr unsigned long SWEEP_DASH_MS  = 450;   // Full-speed straight charge after arc
      constexpr unsigned long WIDE_SWEEP_TOTAL = SWEEP_CURVE_MS + SWEEP_DASH_MS;

      // Strategy 6: BACKSTEP LUNGE (reverse + pivot right ~45° then charge)
      constexpr unsigned long BACKSTEP_REVERSE_MS = 100; // Reverse to create distance & confuse sensors
      constexpr unsigned long BACKSTEP_PIVOT_MS   = 60;  // Hard pivot right ~45°
      constexpr unsigned long BACKSTEP_LUNGE_MS   = 550; // Full speed lunge
      constexpr unsigned long BACKSTEP_LUNGE_TOTAL = BACKSTEP_REVERSE_MS + BACKSTEP_PIVOT_MS + BACKSTEP_LUNGE_MS;

      // Strategy 7: Placeholder (not used)
      constexpr unsigned long STRATEGY_7_TOTAL = 0;
    }
  }
}
#endif
