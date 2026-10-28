#include "Motors.h"
#include "Sensors.h"

namespace Motors {
  static int targetLeft = 0;
  static int targetRight = 0;
  static int actualLeft = 0;
  static int actualRight = 0;
  static unsigned long lastMotorUpdate = 0;

  // True until the robot has moved off a standstill once. Used to select the
  // gentler START_ACCEL_DELTA ramp for the very first spin-up after a match
  // start, then the normal MAX_ACCEL_DELTA ramp afterwards.
  static bool launchedFromStandstill = false;

  // Move `actual` one step toward `target` by at most `maxDelta`.
  // Returns the new value. Keeps the ramping logic in one place so every
  // caller uses identical, well-defined behaviour.
  static int rampToward(int actual, int target, int maxDelta) {
    int delta = target - actual;
    if (abs(delta) > maxDelta)
      return actual + ((delta > 0) ? maxDelta : -maxDelta);
    return target;
  }

  void init() {
    xmotion.StopMotors(1);
  }

  void update(unsigned long currentTime) {
    // Bypass update delay if line is detected to ensure instant reaction and prevent overshoot
    if (!Sensors::isCenterLineDetected() && (currentTime - lastMotorUpdate) < Config::Timing::MOTOR_UPDATE) return;
    lastMotorUpdate = currentTime;

    int clampedLeft = constrain(targetLeft, -100, 100);
    int clampedRight = constrain(targetRight, -100, 100);

    // --- Motor Ramping ---
    // Select the ramp rate for this update:
    //   * Line/edge detected  -> LINE_RETREAT_ACCEL_DELTA (gentle spool-up on retreat)
    //   * First move from rest -> START_ACCEL_DELTA (gentle match-start launch)
    //   * Otherwise            -> MAX_ACCEL_DELTA (normal response)
    bool lineActive = Sensors::isCenterLineDetected();
    int rampDelta = Config::Motor::MAX_ACCEL_DELTA;
    if (lineActive) {
      rampDelta = Config::Motor::LINE_RETREAT_ACCEL_DELTA;
    } else if (!launchedFromStandstill) {
      rampDelta = Config::Motor::START_ACCEL_DELTA;
    }

    // Direction reversals still bypass the ramp so the wheels can reverse
    // immediately for traction/safety; only magnitude changes are ramped.
    bool flipLeft  = (actualLeft  > 0 && clampedLeft  < 0) || (actualLeft  < 0 && clampedLeft  > 0);
    bool flipRight = (actualRight > 0 && clampedRight < 0) || (actualRight < 0 && clampedRight > 0);

    actualLeft  = flipLeft  ? clampedLeft  : rampToward(actualLeft,  clampedLeft,  rampDelta);
    actualRight = flipRight ? clampedRight : rampToward(actualRight, clampedRight, rampDelta);

    // Once either wheel has actually moved, the launch ramp is no longer needed.
    if (actualLeft != 0 || actualRight != 0) {
      launchedFromStandstill = true;
    }

    // --- Driver Output ---
    float multiplierLeft = 1.0;
    float multiplierRight = 1.0;

    // Safety: Apply speed limit if Debug DIP (D6) is active
    if (Sensors::isDebugModeEnabled()) {
        // Retreat and full attack need full power even in debug mode
        // Check target speeds (not actual) to ensure immediate full power response
        multiplierLeft = (targetLeft < 0 || targetLeft == 100) ? 1.0 : Config::Debug::DEBUG_SPEED_MULTIPLIER;
        multiplierRight = (targetRight < 0 || targetRight == 100) ? 1.0 : Config::Debug::DEBUG_SPEED_MULTIPLIER;
    }

    // Map percentage to PWM (0-255)
    int pwmLeft = actualLeft * 2.55 * multiplierLeft;
    int pwmRight = actualRight * 2.55 * multiplierRight;

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
      launchedFromStandstill = false; // Next move is a fresh start from rest
      xmotion.StopMotors(1);
  }
}
