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

  void init() {
    pinMode(Config::Pins::STATUS_LED, OUTPUT);
    digitalWrite(Config::Pins::STATUS_LED, LOW);
  }

  void update(unsigned long currentTime) {
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
    Serial.print(F("T:")); Serial.print(millis());
    Serial.print(F("|S:")); Serial.print(StateMachine::getStateName());

    // Line Status + Invert Flag
    Serial.print(F("|L:")); Serial.print(Sensors::isCenterLineDetected() ? F("HIT") : F("___"));
    Serial.print(F("[INV:")); Serial.print(Sensors::isLineInvertEnabled() ? F("1") : F("0"));
    Serial.print(F("]"));

    // Motors
    Serial.print(F("|M:")); Serial.print(Motors::getActualLeftSpeed());
    Serial.print(F(",")); Serial.print(Motors::getActualRightSpeed());

    // Opponent Tracking
    Serial.print(F("|D:")); Serial.print(StateMachine::getDirectionName(StateMachine::getLastOpponentDirection()));

    // ToF Sensor
    Serial.print(F("|VL:"));
    if (Sensors::isVL53L0XDisabled()) Serial.print(F("OFF"));
    else Serial.print(Sensors::getVL53L0XDistance());

    Serial.println();
  }
}
