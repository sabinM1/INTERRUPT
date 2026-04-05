#include <Arduino.h>
#include "StateMachine.h"
#include "Sensors.h"
#include "Motors.h"
#include "Config.h"
#include "Utils.h"

namespace StateMachine {

  // Opener strategy helper function declarations
  static void executeOpenerStrategy(unsigned long currentTime);
  static void executeLowerRightFlank(unsigned long elapsed);
  static void executeLowerLeftFlank(unsigned long elapsed);
  static void executeDiagonalRightHook(unsigned long elapsed);
  static void executeDiagonalLeftHook(unsigned long elapsed);

  static State currentState = State::STARTUP;
  static State previousState = State::STARTUP;
  static unsigned long stateEnterTime = 0;

  // Strategy Memory
  static uint8_t lastActionTaken = 0;
  static int lastOpponentDirection = -1;
  static unsigned long lastOpponentSeen = 0;

  // Opener Strategy Variables
  static unsigned long openerStartTime = 0;
  static int currentStrategy = 0;
  static bool openerCompleted = false;
  static int openerPhase = 0;
  static float openerHeadingCorrection = 0.0f;

  // Search pattern vars
  static bool seekingRightward = true;
  static unsigned long lastSeekPatternChange = 0;

  // Anti-Stall
  static unsigned long attackDirectionStartTime = 0;
  static int lastAttackDirection = -1;

  // Attack duration tracking
  static unsigned long attackStartTime = 0;

  void init() {
    currentState = State::STARTUP;
    previousState = State::STARTUP;
    stateEnterTime = 0;
    lastActionTaken = 0;
    lastOpponentDirection = -1;
    lastOpponentSeen = 0;
    openerStartTime = 0;
    currentStrategy = 0;
    openerCompleted = false;
    openerPhase = 0;
    openerHeadingCorrection = 0.0f;
    seekingRightward = true;
    lastSeekPatternChange = 0;
    attackDirectionStartTime = 0;
    lastAttackDirection = -1;
    attackStartTime = 0;
  }

  void update(unsigned long currentTime) {
    // 0. Stop Check (highest priority - stop robot immediately if IR stop requested)
    if (Sensors::shouldStopRobot()) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.println(F("[STOP] IR stop requested"));
      }
      #endif
      currentState = State::STOPPED;
      Motors::setTargetSpeed(0, 0);
      return;
    }

    // If already in STOPPED state, exit immediately (prevents any further processing)
    if (currentState == State::STOPPED) {
      return;
    }

    // 1. Sensor Status
    bool opponentDetected = Sensors::isOpponentDetected();
    int currentOpponentDir = Sensors::getOpponentDirection();
    if (currentOpponentDir != -1) {
      updateOpponentMemory(currentOpponentDir);
    }

    // 2. Edge Check (excludes RETREAT_TURN, STOPPED, and STARTUP states)
    bool centerEdge = Sensors::isCenterLineDetected();
    if (centerEdge && currentState != State::RETREAT_TURN && currentState != State::STOPPED && currentState != State::STARTUP) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[EDGE] Line detected! State: "));
        Serial.print(getStateName());
        Serial.print(F(" Loop: "));
        Serial.print(Utils::getLoopDuration());
        Serial.print(F("ms Max: "));
        Serial.print(Utils::getMaxLoopDuration());
        Serial.println(F("ms"));
      }
      #endif
      lastActionTaken = (uint8_t)!lastActionTaken;
      currentState = State::RETREAT_TURN;
      stateEnterTime = currentTime;
      Motors::setTargetSpeed(-Config::Motor::Speeds::RETREAT_POWER, -Config::Motor::Speeds::RETREAT_POWER);
      return;
    }

    // Handle OPENER state transitions
    if (currentState == State::OPENER) {
      unsigned long elapsed = currentTime - openerStartTime;
      
      // TEST_OPENER_ONLY mode: Force stop after opener duration
      #ifdef TEST_OPENER_ONLY
      if (TEST_OPENER_ONLY && elapsed > Config::Timing::OPENER_MAX_DURATION) {
        currentState = State::STOPPED;
        Motors::setTargetSpeed(0, 0);
        return;
      }
      #endif
      
      // Check for opponent detection during opener (after 75% completion threshold)
      #if !defined(TEST_OPENER_ONLY) || !TEST_OPENER_ONLY
      if (opponentDetected && currentOpponentDir != -1) {
        // Check if we've completed at least 75% of the current strategy
        bool earlyExitAllowed = false;
        switch (currentStrategy) {
          case 0: earlyExitAllowed = elapsed > Config::Timing::OpenerEarlyExit::STRATEGY_0; break;
          case 1: earlyExitAllowed = false; break; // No early exit for flank strategies
          case 2: earlyExitAllowed = false; break; // No early exit for flank strategies
          case 3: earlyExitAllowed = false; break; // No early exit for diagonal right hook
          case 4: earlyExitAllowed = false; break; // No early exit for diagonal left hook
          case 5: earlyExitAllowed = false; break; // Reserved (not used)
          case 6: earlyExitAllowed = false; break; // Reserved (not used)
          case 7: earlyExitAllowed = false; break; // Reserved (not used)
        }
        
        if (earlyExitAllowed) {
          #ifdef DEBUG_MODE_ENABLED
          if (DEBUG_MODE_ENABLED) {
            Serial.print(F("[OPENER] Early attack after 50% completion, strategy: "));
            Serial.print(currentStrategy);
            Serial.print(F(", elapsed: "));
            Serial.print(elapsed);
            Serial.println(F("ms"));
          }
          #endif
          currentState = State::ATTACK;
          stateEnterTime = currentTime;
          attackStartTime = currentTime;
          return;
        }
      }
      #endif
      // Continue to motor actuation switch which will execute opener strategy
    }

    // 3. Main State Machine
    switch (currentState) {
      case State::STARTUP: {


        if (currentState == State::STARTUP && Sensors::hasMatchStarted()) {
          currentStrategy = Sensors::getStrategyPattern();
          #ifdef DEBUG_MODE_ENABLED
          if (DEBUG_MODE_ENABLED) {
            Serial.print(F("[COMPETITION] Starting opener, strategy: "));
            Serial.println(currentStrategy);
          }
          #endif
          currentState = State::OPENER;
          openerStartTime = currentTime;
          openerCompleted = false;
          openerPhase = 0;
          openerHeadingCorrection = 0.0f;
          return;
        }
        break;
      }

      case State::OPENER: {
        executeOpenerStrategy(currentTime);
        // If opener completed and we're in TEST_OPENER_ONLY mode, state may have changed to STOPPED
        if (currentState == State::STOPPED) {
          return; // Exit immediately to prevent further processing
        }
        break;
      }

      case State::SEARCH: {
        // Determine search speed based on opponent memory
        int baseSpeed = Config::Motor::Speeds::SEARCH_BASE;
        if (currentTime - lastOpponentSeen < Config::Timing::MEMORY_PERSISTENCE) {
          baseSpeed = Config::Motor::Speeds::SEARCH_BOOSTED;
        }

        // Search pattern: alternate between slight left/right curves
        if (currentTime - lastSeekPatternChange > Config::Timing::SEEK_PATTERNS) {
          lastSeekPatternChange = currentTime;
          seekingRightward = !seekingRightward;
        }

        int leftSpeed = baseSpeed;
        int rightSpeed = baseSpeed;

        if (seekingRightward) {
          leftSpeed = Config::Motor::Speeds::SEARCH_CURVE_OUTSIDE;
          rightSpeed = Config::Motor::Speeds::SEARCH_CURVE_INSIDE;
        } else {
          leftSpeed = Config::Motor::Speeds::SEARCH_CURVE_INSIDE;
          rightSpeed = Config::Motor::Speeds::SEARCH_CURVE_OUTSIDE;
        }

        // Add wiggle for better scanning
        if ((currentTime / 300) % 2 == 0) {
          leftSpeed += Config::Motor::Speeds::SEARCH_WIGGLE_DIFF;
          rightSpeed -= Config::Motor::Speeds::SEARCH_WIGGLE_DIFF;
        } else {
          leftSpeed -= Config::Motor::Speeds::SEARCH_WIGGLE_DIFF;
          rightSpeed += Config::Motor::Speeds::SEARCH_WIGGLE_DIFF;
        }

        // Check for opponent detection to transition to TRACKING
        if (opponentDetected && currentOpponentDir != -1) {
          currentState = State::TRACKING;
          stateEnterTime = currentTime;
        }

        Motors::setTargetSpeed(leftSpeed, rightSpeed);
        break;
      }

      case State::TRACKING: {
        int leftTrackSpeed = Config::Motor::Speeds::TRACK_BASE;
        int rightTrackSpeed = Config::Motor::Speeds::TRACK_BASE;

        if (currentOpponentDir == FRONT_LEFT || currentOpponentDir == SIDE_LEFT) {
          rightTrackSpeed = Config::Motor::Speeds::TRACK_BASE + Config::Motor::Speeds::TRACK_TURN_DIFF;
          leftTrackSpeed = max(Config::Motor::Speeds::TRACK_BASE / 2, Config::Motor::Speeds::TRACK_BASE - Config::Motor::Speeds::TRACK_TURN_DIFF);
        } else if (currentOpponentDir == FRONT_RIGHT || currentOpponentDir == SIDE_RIGHT) {
          leftTrackSpeed = Config::Motor::Speeds::TRACK_BASE + Config::Motor::Speeds::TRACK_TURN_DIFF;
          rightTrackSpeed = max(Config::Motor::Speeds::TRACK_BASE / 2, Config::Motor::Speeds::TRACK_BASE - Config::Motor::Speeds::TRACK_TURN_DIFF);
        }

        // Check if opponent is centered for transition to ATTACK
        if (currentOpponentDir == CENTER) {
          currentState = State::ATTACK;
          stateEnterTime = currentTime;
          attackStartTime = currentTime;
        }

        // Check if opponent lost for transition to SEARCH
        if (!opponentDetected) {
          if (currentTime - stateEnterTime > Config::Timing::OPPONENT_LOST_TIMEOUT) {
            currentState = State::SEARCH;
            lastSeekPatternChange = currentTime;
          }
        }

        Motors::setTargetSpeed(leftTrackSpeed, rightTrackSpeed);
        break;
      }

      case State::RETREAT_TURN: {
        unsigned long elapsed = currentTime - stateEnterTime;

        // Phase 1: Back up straight
        if (elapsed < Config::Movement::RETREAT_DURATION) {
          Motors::setTargetSpeed(-Config::Motor::Speeds::RETREAT_POWER, -Config::Motor::Speeds::RETREAT_POWER);
        }
        // Phase 2: J-Turn
        else if (elapsed < (Config::Movement::RETREAT_DURATION + Config::Movement::TURN_DURATION)) {
          bool turnRight = (lastActionTaken == 0); // Toggle this logic if needed
          if (turnRight) Motors::setTargetSpeed(Config::Motor::Speeds::RETREAT_TURN_POWER, -Config::Motor::Speeds::RETREAT_TURN_POWER);
          else Motors::setTargetSpeed(-Config::Motor::Speeds::RETREAT_TURN_POWER, Config::Motor::Speeds::RETREAT_TURN_POWER);
        }
        else {
          // Check if we're still on the edge line after retreat sequence
          if (Sensors::isCenterLineDetectedRaw()) {
            // Still on edge, restart retreat with opposite turn direction
            lastActionTaken = (uint8_t)!lastActionTaken;
            stateEnterTime = currentTime;
            Motors::setTargetSpeed(-Config::Motor::Speeds::RETREAT_POWER, -Config::Motor::Speeds::RETREAT_POWER);
          } else {
            // Successfully off edge, resume searching or stop based on TEST_OPENER_ONLY
            #ifdef TEST_OPENER_ONLY
            if (TEST_OPENER_ONLY) {
              currentState = State::STOPPED;
              Motors::setTargetSpeed(0, 0);
            } else {
              currentState = State::SEARCH;
              lastSeekPatternChange = currentTime;
            }
            #else
            currentState = State::SEARCH;
            lastSeekPatternChange = currentTime;
            #endif
          }
        }
        break;
      }

      case State::ATTACK: {
        // Debounce loss of target
        if (!opponentDetected) {
          if (currentTime - stateEnterTime > Config::Timing::ATTACK_LOST_TIMEOUT) {
            currentState = State::SEARCH;
            lastSeekPatternChange = currentTime;
            break;
          }
        }

        // Anti-stall: If we're attacking in same direction for too long, add randomness
        if (currentOpponentDir != -1 && currentOpponentDir != lastAttackDirection) {
          attackDirectionStartTime = currentTime;
          lastAttackDirection = currentOpponentDir;
        }
        
        if (currentTime - attackDirectionStartTime > Config::Timing::STALL_DETECTION) {
          // Randomize attack direction briefly to break stalemate
          if ((currentTime / 100) % 2 == 0) {
            Motors::setTargetSpeed(Config::Motor::Speeds::ATTACK_FRONT_TURN, Config::Motor::Speeds::ATTACK_SIDE);
          } else {
            Motors::setTargetSpeed(Config::Motor::Speeds::ATTACK_SIDE, Config::Motor::Speeds::ATTACK_FRONT_TURN);
          }
          break;
        }

        // Attack based on opponent position
        bool isLimited = isAttackPowerLimited(currentTime);
        int attackSpeed = isLimited ? Config::Motor::Speeds::ATTACK_LIMITED : Config::Motor::Speeds::ATTACK_FULL;

        if (currentOpponentDir == CENTER) {
          // Center attack - full power forward
          if (currentTime - lastOpponentSeen < Config::Timing::CENTER_BOOST_WINDOW) {
            // Recent center detection bonus
            attackSpeed += Config::Motor::Speeds::MEMORY_ATTACK_BONUS;
          }
          Motors::setTargetSpeed(attackSpeed, attackSpeed);
        } else if (currentOpponentDir == BOTH_SIDES) {
          // Both sides detected - moderate forward
          Motors::setTargetSpeed(Config::Motor::Speeds::BOTH_SIDES, Config::Motor::Speeds::BOTH_SIDES);
        } else if (currentOpponentDir == FRONT_LEFT || currentOpponentDir == SIDE_LEFT) {
          // Turn right to face opponent
          Motors::setTargetSpeed(Config::Motor::Speeds::ATTACK_SIDE, Config::Motor::Speeds::ATTACK_FRONT_TURN);
        } else if (currentOpponentDir == FRONT_RIGHT || currentOpponentDir == SIDE_RIGHT) {
          // Turn left to face opponent
          Motors::setTargetSpeed(Config::Motor::Speeds::ATTACK_FRONT_TURN, Config::Motor::Speeds::ATTACK_SIDE);
        } else {
          // No current direction, use memory-based attack
          if (lastOpponentDirection != -1) {
            if (lastOpponentDirection == FRONT_LEFT || lastOpponentDirection == SIDE_LEFT) {
              Motors::setTargetSpeed(Config::Motor::Speeds::ATTACK_SIDE, Config::Motor::Speeds::ATTACK_FRONT_TURN);
            } else if (lastOpponentDirection == FRONT_RIGHT || lastOpponentDirection == SIDE_RIGHT) {
              Motors::setTargetSpeed(Config::Motor::Speeds::ATTACK_FRONT_TURN, Config::Motor::Speeds::ATTACK_SIDE);
            } else if (lastOpponentDirection == CENTER) {
              Motors::setTargetSpeed(attackSpeed, attackSpeed);
            }
          } else {
            // Default forward if no memory
            Motors::setTargetSpeed(attackSpeed, attackSpeed);
          }
        }
        break;
      }

      case State::BOTH_SIDES_ATTACK: {
        #ifdef DEBUG_MODE_ENABLED
        if (DEBUG_MODE_ENABLED) {
          Serial.println(F("[BOTH_SIDES] Both sides attack"));
        }
        #endif
        // Moderate forward power when opponent detected on both sides
        Motors::setTargetSpeed(Config::Motor::Speeds::BOTH_SIDES, 
                              Config::Motor::Speeds::BOTH_SIDES);
        break;
      }

      case State::STOPPED: {
        #ifdef DEBUG_MODE_ENABLED
        if (DEBUG_MODE_ENABLED) {
          static bool stoppedPrinted = false;
          if (!stoppedPrinted) {
            Serial.print(F("[STOPPED] Entering STOPPED state"));
            #ifdef TEST_OPENER_ONLY
            if (TEST_OPENER_ONLY) {
              Serial.print(F(" (TEST_OPENER_ONLY mode)"));
            }
            #endif
            Serial.println();
            stoppedPrinted = true;
          }
        }
        #endif
        Motors::setTargetSpeed(0, 0);
        return; // Exit immediately, don't process further
      }
    }
  }

  // --- Helper Functions ---

  State getCurrentState() { return currentState; }

  const char* getStateName() {
    switch (currentState) {
      case State::STARTUP: return "STARTUP";
      case State::OPENER: return "OPENER";
      case State::SEARCH: return "SEARCH";
      case State::TRACKING: return "TRACKING";
      case State::ATTACK: return "ATTACK";
      case State::RETREAT_TURN: return "RETREAT";
      case State::BOTH_SIDES_ATTACK: return "BOTH_SIDES_ATTACK";
      case State::STOPPED: return "STOPPED";
      default: return "UNKNOWN";
    }
  }

  const char* getStrategyName(int strategy) {
    switch (strategy) {
      case 0: return "NO_STRATEGY";
      case 1: return "LOWER_RIGHT_FLANK";
      case 2: return "LOWER_LEFT_FLANK";
      case 3: return "DIAGONAL_RIGHT_HOOK";
      case 4: return "DIAGONAL_LEFT_HOOK";
      case 5: return "RESERVED_5";
      case 6: return "RESERVED_6";
      case 7: return "RESERVED_7";
      default: return "UNKNOWN";
    }
  }

  void updateOpponentMemory(int direction) {
    if (direction < 0 || direction > 4) return;
    lastOpponentDirection = direction;
    lastOpponentSeen = millis();
  }

  int getLastOpponentDirection() { return lastOpponentDirection; }

  const char* getDirectionName(int direction) {
    switch (direction) {
      case FRONT_LEFT: return "FL";
      case FRONT_RIGHT: return "FR";
      case SIDE_LEFT: return "SL";
      case SIDE_RIGHT: return "SR";
      case CENTER: return "C";
      case BOTH_SIDES: return "BOTH";
      default: return "NONE";
    }
  }

  unsigned long getLastAttackTime() { return attackStartTime; }

  bool isAttackPowerLimited(unsigned long currentTime) {
    if (attackStartTime == 0) return false;
    return (currentTime - attackStartTime) < Config::Timing::ATTACK_POWER_LIMIT_TIME;
  }

  // --- Opener Strategy Implementation ---

  static void executeOpenerStrategy(unsigned long currentTime) {
    unsigned long elapsed = currentTime - openerStartTime;
    
    // Execute strategy based on DIP switch pattern (0-7)
    switch (currentStrategy) {
      case 0: // Strategy 0: NO_STRATEGY - immediate transition to SEARCH
        openerCompleted = true; // Immediately mark as completed
        break;
      case 1: // Strategy 1: LOWER RIGHT FLANK
        executeLowerRightFlank(elapsed);
        break;
      case 2: // Strategy 2: LOWER LEFT FLANK
        executeLowerLeftFlank(elapsed);
        break;
      case 3: // Strategy 3: DIAGONAL RIGHT DASH + REAR HOOK
        executeDiagonalRightHook(elapsed);
        break;
      case 4: // Strategy 4: DIAGONAL LEFT DASH + REAR HOOK
        executeDiagonalLeftHook(elapsed);
        break;
      case 5: // Strategy 5: RESERVED (not used)
        openerCompleted = true; // Immediately transition to SEARCH
        break;
      case 6: // Strategy 6: RESERVED (not used)
        openerCompleted = true; // Immediately transition to SEARCH
        break;
      case 7: // Strategy 7: RESERVED (not used)
        openerCompleted = true; // Immediately transition to SEARCH
        break;
    }
    
    // Check if opener should complete
    if (elapsed > Config::Timing::OPENER_MAX_DURATION || openerCompleted) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[OPENER] Completed - "));
        if (elapsed > Config::Timing::OPENER_MAX_DURATION) {
          Serial.print(F("Timeout "));
          Serial.print(elapsed);
          Serial.print(F("ms > "));
          Serial.print(Config::Timing::OPENER_MAX_DURATION);
          Serial.print(F("ms"));
        } else {
          Serial.print(F("Strategy finished"));
        }
        #ifdef TEST_OPENER_ONLY
        if (TEST_OPENER_ONLY) {
          Serial.print(F(" (TEST MODE)"));
        }
        #endif
        Serial.println();
      }
      #endif
      
      // Transition to SEARCH or STOPPED based on TEST_OPENER_ONLY mode
      #ifdef TEST_OPENER_ONLY
      if (TEST_OPENER_ONLY) {
        #ifdef DEBUG_MODE_ENABLED
        if (DEBUG_MODE_ENABLED) {
          Serial.println(F("[TEST] Opener complete - entering STOPPED state"));
        }
        #endif
        currentState = State::STOPPED;
        Motors::setTargetSpeed(0, 0); // Immediate stop in test mode
      } else {
        currentState = State::SEARCH;
      }
      #else
      currentState = State::SEARCH;
      #endif
      stateEnterTime = currentTime;
    }
  }

  // Strategy 1: LOWER RIGHT FLANK
  static void executeLowerRightFlank(unsigned long elapsed) {
    const unsigned long TURN_RIGHT_MS = Config::Movement::Opener::TURN_RIGHT_MS;
    const unsigned long DASH_MS = Config::Movement::Opener::DASH_MS;
    const unsigned long TURN_LEFT_MS = Config::Movement::Opener::TURN_LEFT_MS;
    
    // Phase 1: Hard pivot turn right (~90°)
    if (elapsed < TURN_RIGHT_MS) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[FLANK-R] Phase 1: Right pivot "));
        Serial.print(elapsed);
        Serial.print(F("/"));
        Serial.print(TURN_RIGHT_MS);
        Serial.println(F("ms"));
      }
      #endif
      // Left motor full forward, right motor full reverse for in-place right turn
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED, 
                            -Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED);
    }
    // Phase 2: Forward dash with edge detection
    else if (elapsed < (TURN_RIGHT_MS + DASH_MS)) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[FLANK-R] Phase 2: Forward dash "));
        Serial.print(elapsed - TURN_RIGHT_MS);
        Serial.print(F("/"));
        Serial.print(DASH_MS);
        Serial.println(F("ms"));
      }
      #endif
      // Check for edge line during dash - abort if detected
      if (Sensors::isCenterLineDetected()) {
        #ifdef DEBUG_MODE_ENABLED
        if (DEBUG_MODE_ENABLED) {
          Serial.println(F("[FLANK-R] Edge detected during dash - aborting"));
        }
        #endif
        openerCompleted = true; // Abort strategy
        return;
      }
      // Drive straight forward at reduced speed
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::DASH_SPEED, 
                            Config::Motor::Speeds::Opener::DASH_SPEED);
    }
    // Phase 3: Hard pivot turn left (~90°) to face opponent
    else if (elapsed < (TURN_RIGHT_MS + DASH_MS + TURN_LEFT_MS)) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[FLANK-R] Phase 3: Left pivot "));
        Serial.print(elapsed - TURN_RIGHT_MS - DASH_MS);
        Serial.print(F("/"));
        Serial.print(TURN_LEFT_MS);
        Serial.println(F("ms"));
      }
      #endif
      // Right motor full forward, left motor full reverse for in-place left turn
      Motors::setTargetSpeed(-Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED,
                            Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED);
    }
    // Strategy completed
    else {
      openerCompleted = true;
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.println(F("[FLANK-R] Lower Right Flank completed"));
      }
      #endif
    }
  }

  // Strategy 2: LOWER LEFT FLANK (mirror of Strategy 1)
  static void executeLowerLeftFlank(unsigned long elapsed) {
    const unsigned long TURN_LEFT_MS = Config::Movement::Opener::STRAT2_TURN_LEFT_MS;
    const unsigned long DASH_MS = Config::Movement::Opener::DASH_MS;
    const unsigned long TURN_RIGHT_MS = Config::Movement::Opener::STRAT2_TURN_RIGHT_MS;
    
    // Phase 1: Hard pivot turn left (~90°)
    if (elapsed < TURN_LEFT_MS) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[FLANK-L] Phase 1: Left pivot "));
        Serial.print(elapsed);
        Serial.print(F("/"));
        Serial.print(TURN_LEFT_MS);
        Serial.println(F("ms"));
      }
      #endif
      // Right motor full forward, left motor full reverse for in-place left turn
      Motors::setTargetSpeed(-Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED,
                            Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED);
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[FLANK-L] Motor L:"));
        Serial.print(-Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED);
        Serial.print(F(" R:"));
        Serial.println(Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED);
      }
      #endif
    }
    // Phase 2: Forward dash with edge detection
    else if (elapsed < (TURN_LEFT_MS + DASH_MS)) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[FLANK-L] Phase 2: Forward dash "));
        Serial.print(elapsed - TURN_LEFT_MS);
        Serial.print(F("/"));
        Serial.print(DASH_MS);
        Serial.println(F("ms"));
      }
      #endif
      // Check for edge line during dash - abort if detected
      if (Sensors::isCenterLineDetected()) {
        #ifdef DEBUG_MODE_ENABLED
        if (DEBUG_MODE_ENABLED) {
          Serial.println(F("[FLANK-L] Edge detected during dash - aborting"));
        }
        #endif
        openerCompleted = true; // Abort strategy
        return;
      }
      // Drive straight forward at reduced speed
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::DASH_SPEED, 
                            Config::Motor::Speeds::Opener::DASH_SPEED);
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[FLANK-L] Motor L:"));
        Serial.print(Config::Motor::Speeds::Opener::DASH_SPEED);
        Serial.print(F(" R:"));
        Serial.println(Config::Motor::Speeds::Opener::DASH_SPEED);
      }
      #endif
    }
    // Phase 3: Hard pivot turn right (~90°) to face opponent
    else if (elapsed < (TURN_LEFT_MS + DASH_MS + TURN_RIGHT_MS)) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[FLANK-L] Phase 3: Right pivot "));
        Serial.print(elapsed - TURN_LEFT_MS - DASH_MS);
        Serial.print(F("/"));
        Serial.print(TURN_RIGHT_MS);
        Serial.println(F("ms"));
      }
      #endif
      // Left motor full forward, right motor full reverse for in-place right turn
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED,
                            -Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED);
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[FLANK-L] Motor L:"));
        Serial.print(Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED);
        Serial.print(F(" R:"));
        Serial.println(-Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED);
      }
      #endif
    }
    // Strategy completed
    else {
      openerCompleted = true;
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.println(F("[FLANK-L] Lower Left Flank completed"));
      }
      #endif
    }
  }

  // Strategy 3: DIAGONAL RIGHT DASH + REAR HOOK
  static void executeDiagonalRightHook(unsigned long elapsed) {
    const unsigned long HOOK_DASH_MS = Config::Movement::Opener::HOOK_DASH_MS;
    const unsigned long HOOK_TURN_LEFT_MS = Config::Movement::Opener::HOOK_TURN_LEFT_MS;
    
    // Phase 1: Full-speed forward dash along pre-set diagonal right direction
    if (elapsed < HOOK_DASH_MS) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[HOOK-R] Phase 1: Forward dash "));
        Serial.print(elapsed);
        Serial.print(F("/"));
        Serial.print(HOOK_DASH_MS);
        Serial.println(F("ms"));
      }
      #endif
      // Check for edge line during dash - abort if detected
      if (Sensors::isCenterLineDetected()) {
        #ifdef DEBUG_MODE_ENABLED
        if (DEBUG_MODE_ENABLED) {
          Serial.println(F("[HOOK-R] Edge detected during dash - aborting"));
        }
        #endif
        openerCompleted = true; // Abort strategy
        return;
      }
      // Drive straight forward at full speed (100% PWM)
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::DASH_SPEED, 
                            Config::Motor::Speeds::Opener::DASH_SPEED);
    }
    // Phase 2: Hard left pivot (~135°) to hook around opponent's rear
    else if (elapsed < (HOOK_DASH_MS + HOOK_TURN_LEFT_MS)) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[HOOK-R] Phase 2: Left hook turn "));
        Serial.print(elapsed - HOOK_DASH_MS);
        Serial.print(F("/"));
        Serial.print(HOOK_TURN_LEFT_MS);
        Serial.println(F("ms"));
      }
      #endif
      // Right motor full forward, left motor full reverse for in-place left turn (~135°)
      Motors::setTargetSpeed(-Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED,
                            Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED);
    }
    // Strategy completed
    else {
      openerCompleted = true;
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.println(F("[HOOK-R] Diagonal Right Dash + Rear Hook completed"));
      }
      #endif
    }
  }

  // Strategy 4: DIAGONAL LEFT DASH + REAR HOOK (mirror of Strategy 3)
  static void executeDiagonalLeftHook(unsigned long elapsed) {
    const unsigned long HOOK_DASH_MS = Config::Movement::Opener::HOOK_DASH_MS;
    const unsigned long HOOK_TURN_RIGHT_MS = Config::Movement::Opener::HOOK_TURN_RIGHT_MS;
    
    // Phase 1: Full-speed forward dash along pre-set diagonal left direction
    if (elapsed < HOOK_DASH_MS) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[HOOK-L] Phase 1: Forward dash "));
        Serial.print(elapsed);
        Serial.print(F("/"));
        Serial.print(HOOK_DASH_MS);
        Serial.println(F("ms"));
      }
      #endif
      // Check for edge line during dash - abort if detected
      if (Sensors::isCenterLineDetected()) {
        #ifdef DEBUG_MODE_ENABLED
        if (DEBUG_MODE_ENABLED) {
          Serial.println(F("[HOOK-L] Edge detected during dash - aborting"));
        }
        #endif
        openerCompleted = true; // Abort strategy
        return;
      }
      // Drive straight forward at full speed (100% PWM)
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::DASH_SPEED, 
                            Config::Motor::Speeds::Opener::DASH_SPEED);
    }
    // Phase 2: Hard right pivot (~135°) to hook around opponent's rear
    else if (elapsed < (HOOK_DASH_MS + HOOK_TURN_RIGHT_MS)) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[HOOK-L] Phase 2: Right hook turn "));
        Serial.print(elapsed - HOOK_DASH_MS);
        Serial.print(F("/"));
        Serial.print(HOOK_TURN_RIGHT_MS);
        Serial.println(F("ms"));
      }
      #endif
      // Left motor full forward, right motor full reverse for in-place right turn (~135°)
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED,
                            -Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED);
    }
    // Strategy completed
    else {
      openerCompleted = true;
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.println(F("[HOOK-L] Diagonal Left Dash + Rear Hook completed"));
      }
      #endif
    }
  }
}