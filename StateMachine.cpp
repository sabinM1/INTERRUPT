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

  // Search pattern vars
  static bool seekingRightward = true;
  static unsigned long lastSeekPatternChange = 0;

  // Anti-Stall
  static unsigned long attackDirectionStartTime = 0;
  static int lastAttackDirection = -1;

  // Tuning - now configured in Config.h

  void init() {
    currentState = State::STARTUP;
    stateEnterTime = 0;
    lastOpponentDirection = -1;
    lastOpponentSeen = 0;
    lastActionTaken = 0;
    attackDirectionStartTime = 0;
    lastAttackDirection = -1;
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

    // 4. State Decision Logic
    bool centerEdge = Sensors::isCenterLineDetected();

    // Priority: Edge > Attack > Track/Search
    if (centerEdge) {
      if (currentState != State::RETREAT_TURN) {
          lastActionTaken = (uint8_t)!lastActionTaken;
          currentState = State::RETREAT_TURN;
          stateEnterTime = currentTime;
          // Apply immediate reverse to stop before overshooting
          Motors::setTargetSpeed(-Config::Motor::Speeds::RETREAT_POWER, -Config::Motor::Speeds::RETREAT_POWER);
      }
    }
    else if (currentState != State::RETREAT_TURN) {
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
    }

    // 5. Motor Actuation per State
    switch (currentState) {
      case State::STARTUP: {
        Motors::setTargetSpeed(0, 0);
        if (Sensors::hasMatchStarted()) {
          currentState = State::SEARCH;
          stateEnterTime = currentTime;
        }
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

        leftSpeed = max(-100, min(100, leftSpeed));
        rightSpeed = max(-100, min(100, rightSpeed));
        Motors::setTargetSpeed(leftSpeed, rightSpeed);
        break;
      }

      case State::TRACKING: {
        // Gentle realignment
        int leftTrackSpeed = Config::Motor::Speeds::TRACK_BASE;  // 20
        int rightTrackSpeed = Config::Motor::Speeds::TRACK_BASE; // 20

        if (lastOpponentDirection != -1) {
          if (lastOpponentDirection == FRONT_LEFT || lastOpponentDirection == SIDE_LEFT) {
             leftTrackSpeed = Config::Motor::Speeds::TRACK_BASE - Config::Motor::Speeds::TRACK_TURN_DIFF; // 10
             rightTrackSpeed = Config::Motor::Speeds::TRACK_BASE + Config::Motor::Speeds::TRACK_TURN_DIFF; // 30
          }
          else if (lastOpponentDirection == FRONT_RIGHT || lastOpponentDirection == SIDE_RIGHT) {
             leftTrackSpeed = Config::Motor::Speeds::TRACK_BASE + Config::Motor::Speeds::TRACK_TURN_DIFF; // 30
             rightTrackSpeed = Config::Motor::Speeds::TRACK_BASE - Config::Motor::Speeds::TRACK_TURN_DIFF; // 10
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
        int leftSpeed = Config::Motor::Speeds::ATTACK_FULL;
        int rightSpeed = Config::Motor::Speeds::ATTACK_FULL;

        // Anti-Stall Detection
        if (direction != lastAttackDirection) {
            lastAttackDirection = direction;
            attackDirectionStartTime = currentTime;
        }

        // If pushing same direction > 1s, force straight to break deadlock
        bool stallDetected = (currentTime - attackDirectionStartTime > Config::Timing::STALL_DETECTION);
        if (stallDetected && (direction == FRONT_LEFT || direction == FRONT_RIGHT)) {
             leftSpeed = Config::Motor::Speeds::ATTACK_FULL; rightSpeed = Config::Motor::Speeds::ATTACK_FULL;
        }
        else {
            if (direction == SIDE_LEFT) {
                leftSpeed = 0; rightSpeed = Config::Motor::Speeds::ATTACK_SIDE;
            }
            else if (direction == SIDE_RIGHT) {
                leftSpeed = Config::Motor::Speeds::ATTACK_SIDE; rightSpeed = 0;
            }
            else if (direction == FRONT_LEFT) {
                leftSpeed = 0; rightSpeed = Config::Motor::Speeds::ATTACK_FRONT_TURN;
            }
            else if (direction == FRONT_RIGHT) {
                leftSpeed = Config::Motor::Speeds::ATTACK_FRONT_TURN; rightSpeed = 0;
            }
        }

        // Turbo boost if centered
        if ((currentTime - lastOpponentSeen) < Config::Timing::CENTER_BOOST_WINDOW && direction == CENTER) {
           leftSpeed += Config::Motor::Speeds::MEMORY_ATTACK_BONUS;
           rightSpeed += Config::Motor::Speeds::MEMORY_ATTACK_BONUS;
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

        Motors::setTargetSpeed(Config::Motor::Speeds::BOTH_SIDES, Config::Motor::Speeds::BOTH_SIDES);

        break;
      }
    }
  }

  State getCurrentState() { return currentState; }

  const char* getStateName() {
    switch (currentState) {
      case State::STARTUP: return "STARTUP";
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
}
