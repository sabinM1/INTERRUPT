#include <Arduino.h>
#include "StateMachine.h"
#include "Sensors.h"
#include "Motors.h"

namespace StateMachine {

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
      currentState = State::STOPPED;
      Motors::setTargetSpeed(0, 0);
      return;
    }

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

    // Check if we should enter OPENER state (first time match starts)
    if (currentState == State::STARTUP && Sensors::hasMatchStarted()) {
      currentStrategy = Sensors::getStrategyPattern();
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
      lastActionTaken = (uint8_t)!lastActionTaken;
      currentState = State::RETREAT_TURN;
      stateEnterTime = currentTime;
      // Apply immediate reverse to stop before overshooting
      Motors::setTargetSpeed(-Config::Motor::Speeds::RETREAT_POWER, -Config::Motor::Speeds::RETREAT_POWER);
      return;
    }

    // Handle OPENER state transitions
    if (currentState == State::OPENER) {
      // Check for opponent detection during opener (immediate transition to attack)
      if (opponentDetected && currentOpponentDir != -1) {
        currentState = State::ATTACK;
        stateEnterTime = currentTime;
        attackStartTime = currentTime;
        return;
      }
      
      // Opener will complete via timer in executeOpenerStrategy()
      // If opener completes, it transitions to SEARCH
      return;
    }

    // Priority: Attack > Track/Search (edge already handled above)
    if (currentState != State::RETREAT_TURN) {
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
          currentState = State::SEARCH;
          lastSeekPatternChange = currentTime;
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
    
    // Execute strategy based on DIP switch pattern (0-7)
    switch (currentStrategy) {
      case 0: // Strategy 0: Slow forward scan-creep (default)
        executeSlowForwardScan(elapsed);
        break;
      case 1: // Strategy 1: Quick pivot scan then charge
        executeQuickPivotScan(elapsed);
        break;
      case 2: // Strategy 2: Forward arc acquire
        executeForwardArcAcquire(elapsed);
        break;
      case 3: // Strategy 3: Delayed burst with front-check
        executeDelayedBurst(elapsed);
        break;
      case 4: // Strategy 4: Reverse bait arc
        executeReverseBaitArc(elapsed);
        break;
      case 5: // Strategy 5: Ramped charge with mid-correction
        executeRampedCharge(elapsed);
        break;
      case 6: // Strategy 6: Double micro-sweep then burst
        executeDoubleMicroSweep(elapsed);
        break;
      case 7: // Strategy 7: Reserved/Experimental
        executeSlowForwardScan(elapsed); // Default fallback
        break;
    }
    
    // Check if opener should complete
    if (elapsed > Config::Timing::OPENER_MAX_DURATION || openerCompleted) {
      currentState = State::SEARCH;
      stateEnterTime = currentTime;
    }
  }

  // Strategy 0: Slow forward scan-creep
  static void executeSlowForwardScan(unsigned long elapsed) {
    if (elapsed < Config::Movement::Opener::SLOW_SCAN_PHASE) {
      // Advance at configured power while scanning
      int scanSpeed = Config::Motor::Speeds::Opener::SLOW_SCAN_SPEED;
      
      // Add slight wiggle for scanning
      if ((elapsed / 100) % 2 == 0) {
        Motors::setTargetSpeed(scanSpeed - Config::Motor::Speeds::Opener::WIGGLE_DIFFERENCE, 
                              scanSpeed + Config::Motor::Speeds::Opener::WIGGLE_DIFFERENCE);
      } else {
        Motors::setTargetSpeed(scanSpeed + Config::Motor::Speeds::Opener::WIGGLE_DIFFERENCE, 
                              scanSpeed - Config::Motor::Speeds::Opener::WIGGLE_DIFFERENCE);
      }
    } else if (elapsed < Config::Movement::Opener::SLOW_SCAN_TOTAL) {
      // Ensure forward-facing orientation
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 5, 
                            Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 5);
    } else {
      openerCompleted = true;
    }
  }

  // Strategy 1: Quick pivot scan then charge
  static void executeQuickPivotScan(unsigned long elapsed) {
    if (elapsed < Config::Movement::Opener::PIVOT_PHASE) {
      // Rotate in place at configured power
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::PIVOT_SCAN_SPEED, 
                            -Config::Motor::Speeds::Opener::PIVOT_SCAN_SPEED);
    } else if (elapsed < (Config::Movement::Opener::PIVOT_PHASE + Config::Movement::Opener::CHARGE_PHASE)) {
      // Straighten to forward and burst
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED, 
                            Config::Motor::Speeds::Opener::CHARGE_SPEED);
    } else if (elapsed < Config::Movement::Opener::PIVOT_TOTAL) {
      // Ensure forward alignment
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 5, 
                            Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 5);
    } else {
      openerCompleted = true;
    }
  }

  // Strategy 2: Forward arc acquire
  static void executeForwardArcAcquire(unsigned long elapsed) {
    if (elapsed < Config::Movement::Opener::ARC_PHASE) {
      // Drive shallow forward arc (outside wheel faster)
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ARC_SPEED_OUTSIDE, 
                            Config::Motor::Speeds::Opener::ARC_SPEED_INSIDE);
    } else if (elapsed < (Config::Movement::Opener::ARC_PHASE + Config::Movement::Opener::STRAIGHTEN_PHASE)) {
      // Straighten toward center
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 10, 
                            Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 10);
    } else if (elapsed < Config::Movement::Opener::ARC_TOTAL) {
      // Final alignment
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED, 
                            Config::Motor::Speeds::Opener::ALIGNMENT_SPEED);
    } else {
      openerCompleted = true;
    }
  }

  // Strategy 3: Delayed burst with front-check
  static void executeDelayedBurst(unsigned long elapsed) {
    if (elapsed < Config::Movement::Opener::HOLD_PHASE) {
      Motors::setTargetSpeed(0, 0);
    } else if (elapsed < (Config::Movement::Opener::HOLD_PHASE + Config::Movement::Opener::PULSE_PHASE)) {
      // Left pulse
      Motors::setTargetSpeed(-Config::Motor::Speeds::Opener::PIVOT_SCAN_SPEED / 2, 
                            Config::Motor::Speeds::Opener::PIVOT_SCAN_SPEED / 2);
    } else if (elapsed < (Config::Movement::Opener::HOLD_PHASE + 2 * Config::Movement::Opener::PULSE_PHASE)) {
      // Right pulse
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::PIVOT_SCAN_SPEED / 2, 
                            -Config::Motor::Speeds::Opener::PIVOT_SCAN_SPEED / 2);
    } else if (elapsed < (Config::Movement::Opener::HOLD_PHASE + 2 * Config::Movement::Opener::PULSE_PHASE + Config::Movement::Opener::BURST_PHASE)) {
      // Charge forward
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::CHARGE_SPEED, 
                            Config::Motor::Speeds::Opener::CHARGE_SPEED);
    } else if (elapsed < Config::Movement::Opener::DELAYED_TOTAL) {
      // Alignment
      Motors::setTargetSpeed(Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 5, 
                            Config::Motor::Speeds::Opener::ALIGNMENT_SPEED + 5);
    } else {
      openerCompleted = true;
    }
  }

  // Strategy 4: Reverse bait arc
  static void executeReverseBaitArc(unsigned long elapsed) {
    if (elapsed < 180) { // 180ms reverse diagonal
      Motors::setTargetSpeed(-40, -60); // Reverse with slight turn
    } else if (elapsed < 380) { // 200ms forward arc
      Motors::setTargetSpeed(80, 60); // Forward with 20-30° offset
    } else if (elapsed < 500) { // 120ms straighten
      Motors::setTargetSpeed(40, 40);
    } else if (elapsed < 600) { // 100ms final alignment
      Motors::setTargetSpeed(25, 25);
    } else {
      openerCompleted = true;
    }
  }

  // Strategy 5: Ramped charge with mid-correction
  static void executeRampedCharge(unsigned long elapsed) {
    if (elapsed < 300) { // First 300ms: ramp from 30% to 60%
      int rampSpeed = 30 + (elapsed * 30 / 300); // Linear ramp
      Motors::setTargetSpeed(rampSpeed, rampSpeed);
    } else if (elapsed < 350) { // 50ms mid-correction check
      // Brief pause to check sensors and apply small correction if needed
      Motors::setTargetSpeed(25, 25);
      // Could add sensor-based correction here
    } else if (elapsed < 500) { // 150ms full power
      Motors::setTargetSpeed(100, 100);
    } else if (elapsed < 600) { // 100ms alignment
      Motors::setTargetSpeed(30, 30);
    } else {
      openerCompleted = true;
    }
  }

  // Strategy 6: Double micro-sweep then burst
  static void executeDoubleMicroSweep(unsigned long elapsed) {
    if (elapsed < 100) { // First micro-sweep left
      Motors::setTargetSpeed(-25, 25);
    } else if (elapsed < 200) { // Second micro-sweep right
      Motors::setTargetSpeed(25, -25);
    } else if (elapsed < 400) { // 200ms burst forward
      Motors::setTargetSpeed(100, 100);
    } else if (elapsed < 500) { // 100ms alignment
      Motors::setTargetSpeed(30, 30);
    } else {
      openerCompleted = true;
    }
  }
}
