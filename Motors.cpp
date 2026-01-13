#include "Motors.h"
#include "Sensors.h"

namespace Motors {
  static int targetLeft = 0;
  static int targetRight = 0;
  static int actualLeft = 0;
  static int actualRight = 0;
  static unsigned long lastMotorUpdate = 0;

  void init() {
    xmotion.StopMotors(1);
  }

  void update(unsigned long currentTime) {
    // Bypass update delay if line is detected to ensure instant reaction and prevent overshoot
    if (!Sensors::isCenterLineDetected() && (currentTime - lastMotorUpdate) < Config::Timing::MOTOR_UPDATE) return;
    lastMotorUpdate = currentTime;

    int clampedLeft = constrain(targetLeft, -100, 100);
    int clampedRight = constrain(targetRight, -100, 100);

    // --- Left Motor Ramp ---
    // Instant reverse if direction flips, otherwise smooth ramp
    if ((actualLeft > 0 && clampedLeft < 0) || (actualLeft < 0 && clampedLeft > 0)) {
        actualLeft = clampedLeft;
    } else {
        int delta = clampedLeft - actualLeft;
        if (abs(delta) > Config::Motor::MAX_ACCEL_DELTA)
            actualLeft += (delta > 0) ? Config::Motor::MAX_ACCEL_DELTA : -Config::Motor::MAX_ACCEL_DELTA;
        else
            actualLeft = clampedLeft;
    }

    // --- Right Motor Ramp ---
    if ((actualRight > 0 && clampedRight < 0) || (actualRight < 0 && clampedRight > 0)) {
        actualRight = clampedRight;
    } else {
        int delta = clampedRight - actualRight;
        if (abs(delta) > Config::Motor::MAX_ACCEL_DELTA)
            actualRight += (delta > 0) ? Config::Motor::MAX_ACCEL_DELTA : -Config::Motor::MAX_ACCEL_DELTA;
        else
            actualRight = clampedRight;
    }

    // --- Driver Output ---
    float multiplier = 1.0;

    // Safety: Apply speed limit if Debug DIP (D6) is active
    if (Sensors::isDebugModeEnabled()) {
        multiplier = Config::Debug::DEBUG_SPEED_MULTIPLIER;
    }

    // Map percentage to PWM (0-255)
    int pwmLeft = actualLeft * 2.55 * multiplier;
    int pwmRight = actualRight * 2.55 * multiplier;

    xmotion.MotorControl(pwmRight, -pwmLeft);
  }

  void setTargetSpeed(int left, int right) {
      targetLeft = left;
      targetRight = right;
  }

  int getActualLeftSpeed() { return actualLeft; }
  int getActualRightSpeed() { return actualRight; }

  void stop() {
      targetLeft = 0;
      targetRight = 0;
      actualLeft = 0;
      actualRight = 0;
      xmotion.StopMotors(1);
  }
}
