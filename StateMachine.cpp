#include <Arduino.h>
#include "StateMachine.h"
#include "Sensors.h"
#include "Motors.h"
#include "Config.h"

namespace StateMachine {

  // Opener strategy helper function declarations
  static void executeOpenerStrategy(unsigned long currentTime);
  static void executeAggressiveScan(unsigned long elapsed);
  static void executeHardPivotCharge(unsigned long elapsed);
  static void executePowerSlideArc(unsigned long elapsed);
  static void executeDramaticBurst(unsigned long elapsed);
  static void executeAggressiveBait(unsigned long elapsed);
  static void executePowerRampCharge(unsigned long elapsed);
  static void executeWideSweepBurst(unsigned long elapsed);
  static void executeAggressivePowerSlide(unsigned long elapsed);

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



  // Tuning - now configured in Config.h

  void init() {
    currentState = State::STARTUP;
    stateEnterTime = 0;
    lastOpponentDirection = -1;
    lastOpponentSeen = 0;
    lastActionTaken = 0;
    attackDirectionStartTime = 0;
    lastAttackDirection = -1;
    attackStartTime = 0;
    openerCompleted = false;
    openerPhase = 0;
    openerHeadingCorrection = 0.0f;
    currentStrategy = 0;
  }

  void update(unsigned long currentTime) {
    // 1. Safety Check
    if (Sensors::shouldStopRobot()) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.println(F("[SAFETY] Stop requested"));
      }
      #endif
      currentState = State::STOPPED;
      Motors::setTargetSpeed(0, 0);
      return;
    }

    // TEST_OPENER_ONLY mode: After opener completes, stop the robot
    #ifdef TEST_OPENER_ONLY
    if (TEST_OPENER_ONLY && currentState != State::OPENER && currentState != State::STARTUP) {
      if (currentState != State::STOPPED) {
        #ifdef DEBUG_MODE_ENABLED
        if (DEBUG_MODE_ENABLED) {
          Serial.print(F("[TEST] Stopping robot after opener, state: "));
          Serial.println(getStateName());
        }
        #endif
        currentState = State::STOPPED;
        Motors::setTargetSpeed(0, 0);
      }
      return;
    }
    #endif

    // 2. State Transition Tracking
    if (currentState != previousState) {
      stateEnterTime = currentTime;
      previousState = currentState;
      attackDirectionStartTime = 0;
      lastAttackDirection = -1;

      // Track when attack starts
      if (currentState == State::ATTACK || currentState == State::BOTH_SIDES_ATTACK) {
        attackStartTime = currentTime;
      }
    }

    // 3. Sensor Snapshot
    bool opponentDetected = Sensors::isOpponentDetected();
    int currentOpponentDir = Sensors::getOpponentDirection();

    if (opponentDetected && currentOpponentDir != -1) {
      lastOpponentSeen = currentTime;
      lastOpponentDirection = currentOpponentDir;
    }

    if (!Sensors::hasMatchStarted()) {
      currentState = State::STARTUP;
      Motors::setTargetSpeed(0, 0);
      return;
    }

    // TEST_OPENER_ONLY mode: Auto-start match for testing
    #ifdef TEST_OPENER_ONLY
    static bool testMatchStarted = false;
    if (TEST_OPENER_ONLY && !testMatchStarted) {
      testMatchStarted = true;
      // Simulate match start
      if (currentState == State::STARTUP) {
        currentStrategy = Sensors::getStrategyPattern();
        #ifdef DEBUG_MODE_ENABLED
        if (DEBUG_MODE_ENABLED) {
          Serial.print(F("[TEST] Auto-starting opener, strategy: "));
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
    }
    #endif

    // Check if we should enter OPENER state (first time match starts)
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

    // 4. State Decision Logic
    bool centerEdge = Sensors::isCenterLineDetected();

    // Priority: Edge detection overrides everything except RETREAT_TURN
    if (centerEdge && currentState != State::RETREAT_TURN) {
      #ifdef DEBUG_MODE_ENABLED
      if (DEBUG_MODE_ENABLED) {
        Serial.print(F("[EDGE] Line detected! State: "));
        Serial.println(getStateName());
      }
      #endif
      lastActionTaken = (uint8_t)!lastActionTaken;
      currentState = State::RETREAT_TURN;
      stateEnterTime = currentTime;
      // Apply immediate reverse to stop before overshooting
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
          case 1: earlyExitAllowed = elapsed > Config::Timing::OpenerEarlyExit::STRATEGY_1; break;
          case 2: earlyExitAllowed = elapsed > Config::Timing::OpenerEarlyExit::STRATEGY_2; break;
          case 3: earlyExitAllowed = elapsed > Config::Timing::OpenerEarlyExit::STRATEGY_3; break;
          case 4: earlyExitAllowed = elapsed > Config::Timing::OpenerEarlyExit::STRATEGY_4; break;
          case 5: earlyExitAllowed = elapsed > Config::Timing::OpenerEarlyExit::STRATEGY_5; break;
          case 6: earlyExitAllowed = elapsed > Config::Timing::OpenerEarlyExit::STRATEGY_6; break;
          case 7: earlyExitAllowed = elapsed > Config::Timing::OpenerEarlyExit::STRATEGY_7; break;
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

    // Priority: Attack > Track/Search (edge already handled above)
    if (currentState != State::RETREAT_TURN && currentState != State::OPENER) {
        if (opponentDetected && currentOpponentDir != -1) {
          bool shouldAttack = (currentOpponentDir == FRONT_LEFT || currentOpponentDir == FRONT_RIGHT || currentOpponentDir == CENTER || currentOpponentDir == SIDE_LEFT || currentOpponentDir == SIDE_RIGHT);
          bool bothSidesDetected = (currentOpponentDir == BOTH_SIDES);

          if (bothSidesDetected) {
            if (currentState != State::BOTH_SIDES_ATTACK) {
              currentState = State::BOTH_SIDES_ATTACK;
              stateEnterTime = currentTime;

            }
          }
          else if (shouldAttack) {
            if (currentState != State::ATTACK) {
              currentState = State::ATTACK;
              stateEnterTime = currentTime;

            }
            else if (currentState == State::SEARCH) {
              currentState = State::TRACKING;
            }
          }
        }
        else if (!opponentDetected && lastOpponentDirection != -1 && (currentTime - lastOpponentSeen) < Config::Timing::MEMORY_PERSISTENCE) {
           // Short memory persistence
           if(currentState != State::ATTACK) currentState = State::TRACKING;
        }
        else if (currentState == State::TRACKING || currentState == State::SEARCH) {
          // Stay in current state if no opponent detected
        }
        else {
          // Default to SEARCH if no other state applies
          currentState = State::SEARCH;
        }
    }

    // 5. Motor Actuation per State
    switch (currentState) {
      case State::STARTUP: {
        Motors::setTargetSpeed(0, 0);
        break;
      }

      case State::OPENER: {
        executeOpenerStrategy(currentTime);
        break;
      }

      case State::SEARCH: {
        int baseSearchSpeed;

        // Boost speed if we recently saw something
        if (lastOpponentDirection == -1 || (currentTime - lastOpponentSeen) > Config::Timing::OPPONENT_LOST_TIMEOUT) {
          baseSearchSpeed = Config::Motor::Speeds::SEARCH_BASE;
        } else {
          baseSearchSpeed = Config::Motor::Speeds::SEARCH_BOOSTED;
        }

        int leftSpeed = baseSearchSpeed;
        int rightSpeed = baseSearchSpeed;

        // Memory Search: Curve towards last known position instead of spinning
        // This prevents "ghost" sensors from causing spin-outs
        if (lastOpponentDirection != -1 && (currentTime - lastOpponentSeen) <= Config::Timing::OPPONENT_LOST_TIMEOUT) {
            if (lastOpponentDirection == FRONT_LEFT || lastOpponentDirection == SIDE_LEFT) {
                leftSpeed = Config::Motor::Speeds::SEARCH_CURVE_INSIDE;
                rightSpeed = Config::Motor::Speeds::SEARCH_CURVE_OUTSIDE;
            } else if (lastOpponentDirection == FRONT_RIGHT || lastOpponentDirection == SIDE_RIGHT) {
                leftSpeed = Config::Motor::Speeds::SEARCH_CURVE_OUTSIDE;
                rightSpeed = Config::Motor::Speeds::SEARCH_CURVE_INSIDE;
            }
        } else {
          // Standard Search: Shallow wiggle
          if (currentTime - lastSeekPatternChange > Config::Timing::SEEK_PATTERNS) {
            lastSeekPatternChange = currentTime;
            seekingRightward = !seekingRightward;
          }
          if (seekingRightward) {
              leftSpeed += Config::Motor::Speeds::SEARCH_WIGGLE_DIFF;
              rightSpeed -= Config::Motor::Speeds::SEARCH_WIGGLE_DIFF;
          } else {
              leftSpeed -= Config::Motor::Speeds::SEARCH_WIGGLE_DIFF;
              rightSpeed += Config::Motor::Speeds::SEARCH_WIGGLE_DIFF;
          }
        }

        // Ensure minimum forward speed to prevent spinning
        leftSpeed = max(5, leftSpeed);
        rightSpeed = max(5, rightSpeed);

        leftSpeed = max(-100, min(100, leftSpeed));
        rightSpeed = max(-100, min(100, rightSpeed));
        Motors::setTargetSpeed(leftSpeed, rightSpeed);
        break;
      }

      case State::TRACKING: {
        // Gentle realignment with minimum forward speed
        int leftTrackSpeed = Config::Motor::Speeds::TRACK_BASE;
        int rightTrackSpeed = Config::Motor::Speeds::TRACK_BASE;

        if (lastOpponentDirection != -1) {
          if (lastOpponentDirection == FRONT_LEFT || lastOpponentDirection == SIDE_LEFT) {
             leftTrackSpeed = max(Config::Motor::Speeds::TRACK_BASE / 2, Config::Motor::Speeds::TRACK_BASE - Config::Motor::Speeds::TRACK_TURN_DIFF);
             rightTrackSpeed = Config::Motor::Speeds::TRACK_BASE + Config::Motor::Speeds::TRACK_TURN_DIFF;
          }
          else if (lastOpponentDirection == FRONT_RIGHT || lastOpponentDirection == SIDE_RIGHT) {
             leftTrackSpeed = Config::Motor::Speeds::TRACK_BASE + Config::Motor::Speeds::TRACK_TURN_DIFF;
             rightTrackSpeed = max(Config::Motor::Speeds::TRACK_BASE / 2, Config::Motor::Speeds::TRACK_BASE - Config::Motor::Speeds::TRACK_TURN_DIFF);
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
          if (Sensors::isCenterLineDetected()) {
            // Still on edge, restart retreat with opposite turn direction
            lastActionTaken = (uint8_t)!lastActionTaken;
            stateEnterTime = currentTime;
            // Immediate reverse
            Motors::setTargetSpeed(-Config::Motor::Speeds::RETREAT_POWER, -Config::Motor::Speeds::RETREAT_POWER);
          } else {
            // Successfully off edge, resume searching
            currentState = State::SEARCH;
            lastSeekPatternChange = currentTime;
          }
        }
        break;
      }

      case State::ATTACK: {
        // Debounce loss of target
        bool opponentStillDetected = Sensors::isOpponentDetected();
        if (!opponentStillDetected && (currentTime - lastOpponentSeen) > Config::Timing::ATTACK_LOST_TIMEOUT) {
          currentState = State::SEARCH;
          break;
        }

        int direction = Sensors::getOpponentDirection();
        int sensorPattern = Sensors::getCombinedSensorPattern();

        // Limit power if attacking for less than 1 second
        int attackPower = (currentTime - attackStartTime < Config::Timing::ATTACK_POWER_LIMIT_TIME)
                         ? Config::Motor::Speeds::ATTACK_LIMITED
                         : Config::Motor::Speeds::ATTACK_FULL;

        int leftSpeed = attackPower;
        int rightSpeed = attackPower;

        // Anti-Stall Detection
        if (direction != lastAttackDirection) {
            lastAttackDirection = direction;
            attackDirectionStartTime = currentTime;
        }
        bool stallDetected = (currentTime - attackDirectionStartTime > Config::Timing::STALL_DETECTION);

        // Check for combined sensor patterns first
        bool sideLeftDetected = (sensorPattern & (1 << SIDE_LEFT)) != 0;
        bool sideRightDetected = (sensorPattern & (1 << SIDE_RIGHT)) != 0;
        bool centerDetected = (sensorPattern & (1 << CENTER)) != 0;

        // Handle side left + center detection - turn slightly left (30 difference)
        if (sideLeftDetected && centerDetected) {
            leftSpeed = max(0, attackPower - 30);
            rightSpeed = attackPower;
        }
        // Handle side right + center detection - turn slightly right (30 difference)
        else if (sideRightDetected && centerDetected) {
            leftSpeed = attackPower;
            rightSpeed = max(0, attackPower - 30);
        }
        // If pushing same direction > 1s, force straight to break deadlock
        else if (stallDetected && (direction == FRONT_LEFT || direction == FRONT_RIGHT)) {
             leftSpeed = attackPower; rightSpeed = attackPower;
        }
        else if (direction == SIDE_LEFT) {
            leftSpeed = 0; rightSpeed = Config::Motor::Speeds::ATTACK_SIDE;
        }
        else if (direction == SIDE_RIGHT) {
            leftSpeed = Config::Motor::Speeds::ATTACK_SIDE; rightSpeed = 0;
        }
        else if (direction == FRONT_LEFT) {
            leftSpeed = 0; rightSpeed = min(Config::Motor::Speeds::ATTACK_FRONT_TURN, attackPower);
        }
        else if (direction == FRONT_RIGHT) {
            leftSpeed = min(Config::Motor::Speeds::ATTACK_FRONT_TURN, attackPower); rightSpeed = 0;
        }

        // Turbo boost if centered (but not for combined patterns)
        if ((currentTime - lastOpponentSeen) < Config::Timing::CENTER_BOOST_WINDOW &&
            direction == CENTER &&
            !(sideLeftDetected && centerDetected) &&
            !(sideRightDetected && centerDetected)) {
           leftSpeed = min(leftSpeed + Config::Motor::Speeds::MEMORY_ATTACK_BONUS, attackPower);
           rightSpeed = min(rightSpeed + Config::Motor::Speeds::MEMORY_ATTACK_BONUS, attackPower);
        }

        leftSpeed = max(-100, min(100, leftSpeed));
        rightSpeed = max(-100, min(100, rightSpeed));
        Motors::setTargetSpeed(leftSpeed, rightSpeed);

        break;
      }

      case State::STOPPED: {
        Motors::setTargetSpeed(0, 0);
        break;
      }

      case State::BOTH_SIDES_ATTACK: {
        // Check if opponent is still detected
        bool opponentStillDetected = Sensors::isOpponentDetected();
        int currentDir = Sensors::getOpponentDirection();

        if (!opponentStillDetected) {
          // Transition back to SEARCH if we lose opponent completely
          if ((currentTime - lastOpponentSeen) > Config::Timing::ATTACK_LOST_TIMEOUT) {
            currentState = State::SEARCH;
          }
          break;
        }

        // If detection changed from BOTH_SIDES to something else, transition appropriately
        if (currentDir != BOTH_SIDES) {
          if (currentDir == FRONT_LEFT || currentDir == FRONT_RIGHT ||
              currentDir == CENTER || currentDir == SIDE_LEFT || currentDir == SIDE_RIGHT) {
            currentState = State::ATTACK;
            stateEnterTime = currentTime;
          } else {
            currentState = State::SEARCH;
          }
          break;
        }

        // Limit power if attacking for less than 1 second
        int attackPower = (currentTime - attackStartTime < Config::Timing::ATTACK_POWER_LIMIT_TIME)
                         ? Config::Motor::Speeds::ATTACK_LIMITED
                         : Config::Motor::Speeds::BOTH_SIDES;

        Motors::setTargetSpeed(attackPower, attackPower);

        break;
      }
    }
  }

  State getCurrentState() { return currentState; }

  const char* getStateName() {
    switch (currentState) {
      case State::STARTUP: return "STARTUP";
      case State::OPENER: return "OPENER";
      case State::SEARCH: return "SEARCH";
      case State::TRACKING: return "TRACKING";
      case State::RETREAT_TURN: return "RETREAT";
      case State::ATTACK: return "ATTACK";
      case State::STOPPED: return "STOPPED";
      case State::BOTH_SIDES_ATTACK: return "BOTH_SIDES";
      default: return "UNKNOWN";
    }
  }

  const char* getStrategyName(int strategy) {
    switch (strategy) {
      case 0: return "NO_STRATEGY";
      case 1: return "HARD_PIVOT_CHARGE";
      case 2: return "POWER_SLIDE_ARC";
      case 3: return "DRAMATIC_BURST";
      case 4: return "AGGRESSIVE_BAIT";
      case 5: return "POWER_RAMP_CHARGE";
      case 6: return "WIDE_SWEEP_BURST";
      case 7: return "AGGRESSIVE_POWER_SLIDE";
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
      case FRONT_LEFT:  return "F_LEFT";
      case FRONT_RIGHT: return "F_RIGHT";
      case SIDE_LEFT:   return "S_LEFT";
      case SIDE_RIGHT:  return "S_RIGHT";
      case CENTER:      return "CENTER";
      case BOTH_SIDES:  return "BOTH_SIDES";
      default:          return "NONE";
    }
  }

  unsigned long getLastAttackTime() {
    return attackStartTime;
  }

  bool isAttackPowerLimited(unsigned long currentTime) {
    if (currentState == State::ATTACK || currentState == State::BOTH_SIDES_ATTACK) {
      return (currentTime - attackStartTime < Config::Timing::ATTACK_POWER_LIMIT_TIME);
    }
    return false;
  }

  // Opener Strategy Implementation
  static void executeOpenerStrategy(unsigned long currentTime) {
    unsigned long elapsed = currentTime - openerStartTime;
    
    // Early opponent detection handled within each strategy function
    // Full opener always executes to completion in competition mode
    
    // Execute strategy based on DIP switch pattern (0-7)
    switch (currentStrategy) {
      case 0: // Strategy 0: NO_STRATEGY - immediate transition to SEARCH
        openerCompleted = true; // Immediately mark as completed
        break;
      case 1: // Strategy 1: HARD pivot charge
        executeHardPivotCharge(elapsed);
        break;
      case 2: // Strategy 2: POWER slide arc
        executePowerSlideArc(elapsed);
        break;
      case 3: // Strategy 3: DRAMATIC burst
        executeDramaticBurst(elapsed);
        break;
      case 4: // Strategy 4: AGGRESSIVE bait
        executeAggressiveBait(elapsed);
        break;
      case 5: // Strategy 5: POWER ramp charge
        executePowerRampCharge(elapsed);
        break;
      case 6: // Strategy 6: WIDE sweep burst
        executeWideSweepBurst(elapsed);
        break;
      case 7: // Strategy 7: EXPERIMENTAL power slide
        executeAggressivePowerSlide(elapsed);
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
      
      // Always transition to SEARCH after opener completion
      // Opponent detection during opener is handled by early exit check
      currentState = State::SEARCH;
      stateEnterTime = currentTime;
    }
  }

  // Strategy 0: AGGRESSIVE forward scan with dramatic oscillations
  static void executeAggressiveScan(unsigned long elapsed) {
    if (elapsed < Config::Movement::Opener::SLOW_SCAN_PHASE) {
      // AGGRESSIVE forward advance with dramatic side-to-side scanning
      int scanSpeed = Config::Motor::Speeds::Opener::SLOW_SCAN_SPEED;
      
      // Check for opponent during scan - AGGRESSIVE RESPONSE
      int sensorPattern = Sensors::getCombinedSensorPattern();
      if ((sensorPattern & 0x1F) != 0) {
        // OPPONENT DETECTED - Boost aggression!
        scanSpeed += 30; // Massive power boost
      }
      
      // Dramatic oscillation pattern: 200ms cycles with hard turns
      int cycle = (elapsed / 200) % 4;
      switch (cycle) {
        case 0: // Hard left turn
          Motors::setTargetSpeed(scanSpeed / 2, scanSpeed);
          break;
        case 1: // Hard right turn  
          Motors::setTargetSpeed(scanSpeed, scanSpeed / 2);
          break;
        case 2: // Forward burst
          Motors::setTargetSpeed(scanSpeed + 20, scanSpeed + 20);
          break;
        case 3: // Wide oscillation
          Motors::setTargetSpeed(scanSpeed - 15, scanSpeed + 15);
          break;
      }
    } else if (elapsed < Config::Movement::Opener::SLOW_SCAN_TOTAL) {
      // Power forward alignment with aggressive straightening
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 25,
                            Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 25);
    } else {
      openerCompleted = true;
    }
  }

  // Strategy 1: AGGRESSIVE hard pivot scan with power charge
  static void executeHardPivotCharge(unsigned long elapsed) {
    if (elapsed < Config::Movement::Opener::PIVOT_PHASE) {
      // HARD pivot rotation with aggressive speed
      int pivotSpeed = Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED;
      
      // Check for opponent during pivot - AGGRESSIVE RESPONSE
      int sensorPattern = Sensors::getCombinedSensorPattern();
      if ((sensorPattern & 0x1F) != 0) {
        // OPPONENT DETECTED - Immediate charge with power boost!
        Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED + 20,
                              Config::Motor::Speeds::Opener::CHARGE_SPEED + 20);
        return;
      }
      
      Motors::setTargetSpeed(pivotSpeed, -pivotSpeed);
    } else if (elapsed < (Config::Movement::Opener::PIVOT_PHASE + Config::Movement::Opener::CHARGE_PHASE)) {
      // POWER charge forward at maximum speed
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED,
                            Config::Motor::Speeds::Opener::CHARGE_SPEED);
    } else if (elapsed < Config::Movement::Opener::PIVOT_TOTAL) {
      // Aggressive forward alignment with slight oscillation
      if ((elapsed / 50) % 2 == 0) {
        Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 30,
                              Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 25);
      } else {
        Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 25,
                              Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 30);
      }
    } else {
      openerCompleted = true;
    }
  }

  // Strategy 2: AGGRESSIVE wide arc with power slide
  static void executePowerSlideArc(unsigned long elapsed) {
    if (elapsed < Config::Movement::Opener::ARC_PHASE) {
      // DRAMATIC wide arc with power slide effect
      int outsideSpeed = Config::Motor::Speeds::Opener::POWER_SLIDE_SPEED;
      int insideSpeed = Config::Motor::Speeds::Opener::ARC_SPEED_INSIDE;
      
      // Check for opponent during arc - AGGRESSIVE RESPONSE
      int sensorPattern = Sensors::getCombinedSensorPattern();
      if ((sensorPattern & 0x1F) != 0) {
        // OPPONENT DETECTED - Sharpen turn toward opponent with power boost!
        if (sensorPattern & (1 << 2)) { // Side left detected
          outsideSpeed += 60; // Much sharper left turn
          insideSpeed += 20;  // Power boost
        } else if (sensorPattern & (1 << 3)) { // Side right detected
          insideSpeed += 60; // Much sharper right turn
          outsideSpeed += 20; // Power boost
        } else if (sensorPattern & (1 << 4)) { // Center detected
          // MAXIMUM CHARGE!
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED,
                                Config::Motor::Speeds::Opener::CHARGE_SPEED);
          return;
        }
      }
      
      Motors::setTargetSpeed(outsideSpeed, insideSpeed);
    } else if (elapsed < (Config::Movement::Opener::ARC_PHASE + Config::Movement::Opener::STRAIGHTEN_PHASE)) {
      // Aggressive straightening with power
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 40,
                            Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 40);
    } else if (elapsed < Config::Movement::Opener::ARC_TOTAL) {
      // Final power alignment with slight oscillation
      int oscillation = (elapsed / 75) % 2;
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 20 + (oscillation * 10),
                            Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 20 - (oscillation * 10));
    } else {
      openerCompleted = true;
    }
  }

  // Strategy 3: AGGRESSIVE delayed burst with dramatic pulses
  static void executeDramaticBurst(unsigned long elapsed) {
    // Check for opponent during ANY phase - AGGRESSIVE RESPONSE
    int sensorPattern = Sensors::getCombinedSensorPattern();
    if ((sensorPattern & 0x1F) != 0) {
      // OPPONENT DETECTED - Skip to maximum charge!
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED,
                          Config::Motor::Speeds::Opener::CHARGE_SPEED);
      return;
    }
      
    // Add opponent detection during power slide for aggressive response
    if ((sensorPattern & 0x1F) != 0) {
      // OPPONENT DETECTED - Immediate maximum charge!
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED,
                          Config::Motor::Speeds::Opener::CHARGE_SPEED);
      return;
    }
    
    if (elapsed < Config::Movement::Opener::HOLD_PHASE) {
      // TENSE hold with slight vibration
      int vibration = (elapsed / 50) % 2;
      Motors::setTargetSpeed(vibration * 5, -vibration * 5);
    } else if (elapsed < (Config::Movement::Opener::HOLD_PHASE + Config::Movement::Opener::PULSE_PHASE)) {
      // DRAMATIC left hard turn
      Motors::setTargetSpeed(-Config::Motor::Speeds::Opener::RAPID_OSCILLATION_SPEED,
                            Config::Motor::Speeds::Opener::RAPID_OSCILLATION_SPEED);
    } else if (elapsed < (Config::Movement::Opener::HOLD_PHASE + 2 * Config::Movement::Opener::PULSE_PHASE)) {
      // DRAMATIC right hard turn
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::RAPID_OSCILLATION_SPEED,
                            -Config::Motor::Speeds::Opener::RAPID_OSCILLATION_SPEED);
    } else if (elapsed < (Config::Movement::Opener::HOLD_PHASE + 2 * Config::Movement::Opener::PULSE_PHASE + Config::Movement::Opener::BURST_PHASE)) {
      // MAXIMUM POWER charge forward
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED,
                            Config::Motor::Speeds::Opener::CHARGE_SPEED);
    } else if (elapsed < Config::Movement::Opener::DELAYED_TOTAL) {
      // Aggressive alignment with power oscillation
      int powerOsc = (elapsed / 60) % 2;
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 30 + (powerOsc * 15),
                            Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 30 - (powerOsc * 15));
    } else {
      openerCompleted = true;
    }
  }


  // Strategy 4: AGGRESSIVE reverse bait with dramatic recovery
  static void executeAggressiveBait(unsigned long elapsed) {
    if (elapsed < Config::Movement::Opener::REVERSE_PHASE) {
      // AGGRESSIVE reverse with hard turn (power slide backwards)
      Motors::setTargetSpeed(-Config::Motor::Speeds::Opener::REVERSE_BAIT_SPEED,
                            -(Config::Motor::Speeds::Opener::REVERSE_BAIT_SPEED + 40));
      
      // Check for opponent during reverse - AGGRESSIVE COUNTER-ATTACK
      int sensorPattern = Sensors::getCombinedSensorPattern();
      if ((sensorPattern & 0x1F) != 0) {
        // OPPONENT DETECTED - Immediate forward attack!
        Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED,
                              Config::Motor::Speeds::Opener::CHARGE_SPEED);
        return;
      }
    } else if (elapsed < (Config::Movement::Opener::REVERSE_PHASE + Config::Movement::Opener::FORWARD_ARC_PHASE)) {
      // DRAMATIC forward power slide with aggressive turn
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED - 10,
                            Config::Motor::Speeds::Opener::CHARGE_SPEED - 30);
    } else if (elapsed < Config::Movement::Opener::BAIT_TOTAL - 200) {
      // Power straightening with oscillation
      int oscCycle = (elapsed / 80) % 3;
      switch (oscCycle) {
        case 0:
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 35,
                                Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 30);
          break;
        case 1:
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 30,
                                Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 35);
          break;
        case 2:
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 40,
                                Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 40);
          break;
      }
    } else if (elapsed < Config::Movement::Opener::BAIT_TOTAL) {
      // Final aggressive alignment
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 25,
                            Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 25);
    } else {
      openerCompleted = true;
    }
  }

  // Strategy 5: AGGRESSIVE ramped charge with dramatic power build
  static void executePowerRampCharge(unsigned long elapsed) {
    if (elapsed < Config::Movement::Opener::RAMP_PHASE) {
      // AGGRESSIVE exponential ramp from 50% to 90% power
      int rampProgress = elapsed * 100 / Config::Movement::Opener::RAMP_PHASE;
      int rampSpeed = Config::Motor::Speeds::Opener::RAMP_START_SPEED + 
                     (rampProgress * rampProgress * 40 / 10000); // Exponential curve
      
      // Add dramatic weaving during ramp
      int weave = (elapsed / 120) % 2;
      Motors::setTargetSpeed(rampSpeed + (weave * 15), rampSpeed - (weave * 15));
      
      // Check for opponent during ramp - AGGRESSIVE ACCELERATION
      int sensorPattern = Sensors::getCombinedSensorPattern();
      if ((sensorPattern & 0x1F) != 0) {
        // OPPONENT DETECTED - Skip to maximum power!
        Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED,
                              Config::Motor::Speeds::Opener::CHARGE_SPEED);
        return;
      }
    } else if (elapsed < (Config::Movement::Opener::RAMP_PHASE + Config::Movement::Opener::CORRECTION_PHASE)) {
      // AGGRESSIVE correction with hard pivot
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED / 2,
                            -Config::Motor::Speeds::Opener::HARD_PIVOT_SPEED / 2);
    } else if (elapsed < (Config::Movement::Opener::RAMP_PHASE + Config::Movement::Opener::CORRECTION_PHASE + Config::Movement::Opener::FULL_POWER_PHASE)) {
      // MAXIMUM POWER charge with weaving
      int powerWeave = (elapsed / 100) % 2;
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED - (powerWeave * 25),
                            Config::Motor::Speeds::Opener::CHARGE_SPEED + (powerWeave * 25));
    } else if (elapsed < Config::Movement::Opener::RAMP_TOTAL) {
      // Aggressive alignment with power oscillation
      int powerCycle = (elapsed / 80) % 3;
      switch (powerCycle) {
        case 0:
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 45,
                                Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 40);
          break;
        case 1:
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 40,
                                Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 45);
          break;
        case 2:
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 50,
                                Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 50);
          break;
      }
    } else {
      openerCompleted = true;
    }
  }

  // Strategy 7: AGGRESSIVE Power Slide (Experimental)
  static void executeAggressivePowerSlide(unsigned long elapsed) {
    if (elapsed < Config::Movement::Opener::POWER_SLIDE_PHASE) {
      // DRAMATIC power slide with hard turn
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::POWER_SLIDE_SPEED,
                            -Config::Motor::Speeds::Opener::POWER_SLIDE_SPEED / 2);
      
      // Check for opponent during power slide - AGGRESSIVE RESPONSE
      int sensorPattern = Sensors::getCombinedSensorPattern();
      if ((sensorPattern & 0x1F) != 0) {
        // OPPONENT DETECTED - Immediate maximum charge!
        Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED,
                              Config::Motor::Speeds::Opener::CHARGE_SPEED);
        return;
      }
    } else if (elapsed < (Config::Movement::Opener::POWER_SLIDE_PHASE + Config::Movement::Opener::RECOVERY_PHASE)) {
      // AGGRESSIVE recovery with opposite turn
      Motors::setTargetSpeed(-Config::Motor::Speeds::Opener::POWER_SLIDE_SPEED / 2,
                            Config::Motor::Speeds::Opener::POWER_SLIDE_SPEED);
    } else if (elapsed < (Config::Movement::Opener::POWER_SLIDE_PHASE + Config::Movement::Opener::RECOVERY_PHASE + Config::Movement::Opener::FINAL_CHARGE_PHASE)) {
      // MAXIMUM power forward charge
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED,
                            Config::Motor::Speeds::Opener::CHARGE_SPEED);
    } else if (elapsed < Config::Movement::Opener::POWER_SLIDE_TOTAL) {
      // Power alignment with aggressive oscillation
      int osc = (elapsed / 50) % 2;
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 40 + (osc * 25),
                            Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 40 - (osc * 25));
    } else {
      openerCompleted = true;
    }
  }

  // Strategy 6: AGGRESSIVE wide sweeps with power bursts
  static void executeWideSweepBurst(unsigned long elapsed) {
    if (elapsed < Config::Movement::Opener::MICRO_SWEEP_PHASE) {
      // WIDE aggressive left sweep
      Motors::setTargetSpeed(-Config::Motor::Speeds::Opener::MICRO_SWEEP_SPEED,
                            Config::Motor::Speeds::Opener::MICRO_SWEEP_SPEED + 20);
      
      // Check for opponent during sweep - AGGRESSIVE RESPONSE
      int sensorPattern = Sensors::getCombinedSensorPattern();
      if ((sensorPattern & 0x1F) != 0) {
        // OPPONENT DETECTED - Immediate charge in that direction!
        if (sensorPattern & (1 << 2)) { // Side left detected during left sweep
          Motors::setTargetSpeed(-Config::Motor::Speeds::Opener::CHARGE_SPEED / 2,
                                Config::Motor::Speeds::Opener::CHARGE_SPEED);
        } else {
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED,
                                Config::Motor::Speeds::Opener::CHARGE_SPEED);
        }
        return;
      }
    } else if (elapsed < 2 * Config::Movement::Opener::MICRO_SWEEP_PHASE) {
      // WIDE aggressive right sweep
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::MICRO_SWEEP_SPEED + 20,
                            -Config::Motor::Speeds::Opener::MICRO_SWEEP_SPEED);
      
      // Check for opponent during sweep - AGGRESSIVE RESPONSE
      int sensorPattern = Sensors::getCombinedSensorPattern();
      if ((sensorPattern & 0x1F) != 0) {
        // OPPONENT DETECTED - Immediate charge in that direction!
        if (sensorPattern & (1 << 3)) { // Side right detected during right sweep
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED,
                                -Config::Motor::Speeds::Opener::CHARGE_SPEED / 2);
        } else {
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED,
                                Config::Motor::Speeds::Opener::CHARGE_SPEED);
        }
        return;
      }
    } else if (elapsed < (2 * Config::Movement::Opener::MICRO_SWEEP_PHASE + Config::Movement::Opener::BURST_AFTER_SWEEP / 2)) {
      // POWER burst forward with weaving
      int weave = (elapsed / 80) % 2;
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED - (weave * 20),
                            Config::Motor::Speeds::Opener::CHARGE_SPEED + (weave * 20));
    } else if (elapsed < (2 * Config::Movement::Opener::MICRO_SWEEP_PHASE + Config::Movement::Opener::BURST_AFTER_SWEEP)) {
      // MAXIMUM straight charge
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED,
                            Config::Motor::Speeds::Opener::CHARGE_SPEED);
    } else if (elapsed < Config::Movement::Opener::MICRO_TOTAL) {
      // Aggressive alignment with power oscillation
      int powerCycle = (elapsed / 70) % 4;
      switch (powerCycle) {
        case 0:
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 40,
                                Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 35);
          break;
        case 1:
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 35,
                                Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 40);
          break;
        case 2:
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 45,
                                Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 45);
          break;
        case 3:
          Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 30,
                                Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 30);
          break;
      }
    } else {
      openerCompleted = true;
    }
  }
}
