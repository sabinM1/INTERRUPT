#include "Utils.h"
#include "Sensors.h"
#include "Motors.h"
#include "StateMachine.h"

namespace Utils {
  static bool ledState = false;
  static unsigned long lastLedToggle = 0;
  static unsigned long lastDebugPrint = 0;
  static unsigned long blinkInterval = 0;
  static bool blinkingEnabled = false;
  static unsigned long lastLoopTime = 0;
  static unsigned long loopDuration = 0;
  static unsigned long maxLoopDuration = 0;

  void init() {
    pinMode(Config::Pins::STATUS_LED, OUTPUT);
    digitalWrite(Config::Pins::STATUS_LED, LOW);
  }

  void update(unsigned long currentTime) {
    // --- Loop Timing ---
    if (lastLoopTime != 0) {
      loopDuration = currentTime - lastLoopTime;
      if (loopDuration > maxLoopDuration) {
        maxLoopDuration = loopDuration;
      }
    }
    lastLoopTime = currentTime;

    // --- LED Management ---
    if (blinkingEnabled && blinkInterval > 0) {
      if ((currentTime - lastLedToggle) >= blinkInterval) {
        lastLedToggle = currentTime;
        ledState = !ledState;
        digitalWrite(Config::Pins::STATUS_LED, ledState);
      }
    } else {
      // Default behavior: Solid ON during match, Blink when waiting
      if (Sensors::hasMatchStarted()) {
          digitalWrite(Config::Pins::STATUS_LED, HIGH);
      }
      else {
        if ((currentTime - lastLedToggle) >= Config::Timing::LED_BLINK) {
          lastLedToggle = currentTime;
          ledState = !ledState;
          digitalWrite(Config::Pins::STATUS_LED, ledState);
        }
      }
    }

    // --- Serial Telemetry (if DIP enabled) ---
    if (Sensors::isDebugModeEnabled()) {
        if ((currentTime - lastDebugPrint) >= Config::Timing::DEBUG_PRINT) {
          lastDebugPrint = currentTime;
          printStatus();
        }
    }
  }

  void setStatusLED(bool state) {
      blinkingEnabled = false;
      ledState = state;
      digitalWrite(Config::Pins::STATUS_LED, state);
  }

  void blinkStatusLED(unsigned long interval) {
      blinkingEnabled = true;
      blinkInterval = interval;
  }

  void printStatus() {
    unsigned long currentTime = millis();
    Serial.print(F("T:")); Serial.print(currentTime);
    Serial.print(F("|Lt:")); Serial.print(loopDuration);
    Serial.print(F("|Lmax:")); Serial.print(maxLoopDuration);
    Serial.print(F("|S:")); Serial.print(StateMachine::getStateName());

    // Line Status
    Serial.print(F("|L:")); Serial.print(Sensors::isCenterLineDetected() ? F("HIT") : F("___"));

    // Motors
    Serial.print(F("|M:")); Serial.print(Motors::getActualLeftSpeed());
    Serial.print(F(",")); Serial.print(Motors::getActualRightSpeed());

    // Opponent Tracking
    Serial.print(F("|D:")); Serial.print(StateMachine::getDirectionName(StateMachine::getLastOpponentDirection()));
    
    // Sensor Pattern Debug
    int sensorPattern = Sensors::getCombinedSensorPattern();
    Serial.print(F("|P:"));
    Serial.print((sensorPattern & (1 << 0)) ? "FL" : "__");
    Serial.print((sensorPattern & (1 << 1)) ? "FR" : "__");
    Serial.print((sensorPattern & (1 << 2)) ? "SL" : "__");
    Serial.print((sensorPattern & (1 << 3)) ? "SR" : "__");
    Serial.print((sensorPattern & (1 << 4)) ? "C" : "_");
    
    // Combined Pattern Indicator
    bool sideLeftDetected = (sensorPattern & (1 << 2)) != 0;
    bool sideRightDetected = (sensorPattern & (1 << 3)) != 0;
    bool centerDetected = (sensorPattern & (1 << 4)) != 0;
    if (sideLeftDetected && centerDetected) {
        Serial.print(F("[SL+C]"));
    } else if (sideRightDetected && centerDetected) {
        Serial.print(F("[SR+C]"));
    } else {
        Serial.print(F("[---]"));
    }

    // Attack Timer Status
    unsigned long attackStartTime = StateMachine::getLastAttackTime();
    bool isLimited = StateMachine::isAttackPowerLimited(currentTime);
    
    Serial.print(F("|ATK:"));
    if (attackStartTime == 0) {
      Serial.print(F("NEVER"));
    } else {
      unsigned long attackDuration = currentTime - attackStartTime;
      Serial.print(attackDuration);
      Serial.print(F("ms"));
    }
    Serial.print(F("["));
    Serial.print(isLimited ? F("LIM") : F("FULL"));
    Serial.print(F("]"));

    // ToF Sensor
    Serial.print(F("|VL:"));
    if (Sensors::isVL53L0XDisabled()) Serial.print(F("OFF"));
    else Serial.print(Sensors::getVL53L0XDistance());

    // Strategy Number and Name (0-7: 3-bit pattern from DIP switches 5,6,7)
    Serial.print(F("|STRAT:"));
    int strategy = Sensors::getStrategyPattern();
    Serial.print(strategy);
    Serial.print(F("["));
    Serial.print(StateMachine::getStrategyName(strategy));
    Serial.print(F("]"));

    Serial.println();
    maxLoopDuration = 0;
  }

  unsigned long getLoopDuration() {
    return loopDuration;
  }

  unsigned long getMaxLoopDuration() {
    return maxLoopDuration;
  }
}
