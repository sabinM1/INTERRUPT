#include <Arduino.h>
#include <SoftwareWire.h>
#include "Sensors.h"
#include "StateMachine.h" // Include StateMachine header

namespace Sensors {

const int VL_SDA = Config::Pins::VL53L0X_SDA;
const int VL_SCL = Config::Pins::VL53L0X_SCL;
const uint8_t VL_ADDR = 0x29;
SoftwareWire myWire(VL_SDA, VL_SCL);

// Sensor State
bool js40fStates[4] = {false};
static unsigned long js40LastDebounce[4] = {0};
static uint16_t vl53loxDistance = 8190;
static bool vl53loxReady = false;
static bool centerLineDetected = false;
static int lineTriggerCount = 0;

// Config State (from #defines)
static bool vlDisableSwitchState = !VL_ENABLED;
static bool debugModeEnabled = DEBUG_MODE_ENABLED;


// Strategy Selection
static bool strategy1State = false;
static bool strategy2State = false;
static bool strategy3State = false;

// Robot Logic State
static bool robotReadyState = false;
static bool matchStarted = false;
static bool stopSequenceRequested = false;
static unsigned long lastSensorRead = 0;
static unsigned long lastVL53Read = 0;
static unsigned long lastBlinkTime = 0;
static int readyBlinkCount = 0;

enum RobotState { STARTUP_ERROR, STARTUP_READY_SIGNAL, OPERATIONAL, MATCH_STOPPED };
static RobotState currentState;

// I2C Helpers
void writeReg(uint8_t reg, uint8_t value) {
  myWire.beginTransmission(VL_ADDR); myWire.write(reg); myWire.write(value); myWire.endTransmission();
}
uint8_t readReg(uint8_t reg) {
  myWire.beginTransmission(VL_ADDR); myWire.write(reg); myWire.endTransmission(); myWire.requestFrom(VL_ADDR, (uint8_t)1); return myWire.read();
}

// Magic sequence to kickstart the VL53L0X without full API
void kickstartVL53L0X() {
  writeReg(0x88, 0x00); writeReg(0x80, 0x01); writeReg(0xFF, 0x01); writeReg(0x00, 0x00);
  writeReg(0x91, 0x3C); writeReg(0x00, 0x01); writeReg(0xFF, 0x00); writeReg(0x80, 0x00);
  writeReg(0x80, 0x01); writeReg(0xFF, 0x01); writeReg(0x00, 0x00); writeReg(0x91, 0x3C);
  writeReg(0x00, 0x01); writeReg(0xFF, 0x00); writeReg(0x80, 0x00); writeReg(0x00, 0x02);
}

void init() {
  lastSensorRead = 0;
  lineTriggerCount = 0;

  // Pin Setup
  pinMode(Config::Pins::CENTER_LINE, INPUT);
  pinMode(Config::Pins::IR_START_MODULE, INPUT);

  pinMode(Config::Pins::JS40F_FRONT_LEFT, INPUT);
  pinMode(Config::Pins::JS40F_FRONT_RIGHT, INPUT);
  pinMode(Config::Pins::JS40F_SIDE_LEFT, INPUT);
  pinMode(Config::Pins::JS40F_SIDE_RIGHT, INPUT);

  pinMode(Config::Pins::STRATEGY_1, INPUT_PULLUP);
  pinMode(Config::Pins::STRATEGY_2, INPUT_PULLUP);
  pinMode(Config::Pins::STRATEGY_3, INPUT_PULLUP);

  // I2C Init
  myWire.begin();
  delay(10);
  myWire.setClock(50000); // Low speed for software I2C stability
  delay(10);
  kickstartVL53L0X();

  // Safety check: Is the start module active on boot?
  int initialIRState = digitalRead(Config::Pins::IR_START_MODULE);
  if (initialIRState == 1) {
      currentState = STARTUP_ERROR;
  } else {
    robotReadyState = true;
    currentState = OPERATIONAL;
    digitalWrite(Config::Pins::STATUS_LED, LOW);
  }
}

void update(unsigned long currentTime) {
  // --- Status LED Logic ---
  if (currentState == STARTUP_ERROR || currentState == MATCH_STOPPED) {
      // Continuous slow blink for errors/stop
      if (currentTime - lastBlinkTime > 250) {
        lastBlinkTime = currentTime;
        digitalWrite(Config::Pins::STATUS_LED, !digitalRead(Config::Pins::STATUS_LED));
      }
      return; // Halt sensor processing
  }

  // --- Fast Polling (5ms) ---
  if ((currentTime - lastSensorRead) >= 5) {
    lastSensorRead = currentTime;

    // Strategy Selection
    strategy1State = (digitalRead(Config::Pins::STRATEGY_1) == LOW);
    strategy2State = (digitalRead(Config::Pins::STRATEGY_2) == LOW);
    strategy3State = (digitalRead(Config::Pins::STRATEGY_3) == LOW);

    // JS40F: Sequential read to avoid crosstalk/spikes
    static int phase = 0;
    uint8_t pins[] = { Config::Pins::JS40F_FRONT_LEFT, Config::Pins::JS40F_FRONT_RIGHT, Config::Pins::JS40F_SIDE_LEFT, Config::Pins::JS40F_SIDE_RIGHT };

    bool val = digitalRead(pins[phase]) == HIGH;
    if (val != js40fStates[phase]) {
       // Debounce 50ms
       if (currentTime - js40LastDebounce[phase] > 50) {
         js40fStates[phase] = val;
         js40LastDebounce[phase] = currentTime;
       }
    }
    phase = (phase + 1) % 4;


  }

  // --- Ultra-Fast Line Polling (Double Check) ---
  // Polled every loop for maximum responsiveness to prevent overshoot.
  // A 2-read confirmation filters out high-frequency false positives (EMI/Noise)
  // while maintaining sub-millisecond reaction times.
  bool currentLineState;
  #ifdef LINE_INVERT
    currentLineState = (digitalRead(Config::Pins::CENTER_LINE) == (LINE_INVERT ? LOW : HIGH));
  #else
    currentLineState = (digitalRead(Config::Pins::CENTER_LINE) == HIGH);
  #endif
  
  // Debounce with 2 consecutive readings required (with hysteresis)
  if (currentLineState) {
    if (lineTriggerCount < 2) {
      lineTriggerCount++;
    }
  } else {
    if (lineTriggerCount > 0) {
      lineTriggerCount--;
    }
  }
  
  // Confirm line detection only after 2 consecutive positive readings
  centerLineDetected = (lineTriggerCount >= 2);

  // --- Distance Sensor (30ms) ---
  if ((currentTime - lastVL53Read) >= 30) {
    lastVL53Read = currentTime;

    readReg(0x13);
    myWire.beginTransmission(VL_ADDR); myWire.write(0x1E); myWire.endTransmission();
    myWire.requestFrom(VL_ADDR, (uint8_t)2);

    if (myWire.available() >= 2) {
      uint8_t hi = myWire.read();
      uint8_t lo = myWire.read();
      uint16_t r = (hi << 8) | lo;

      // Filter noise (valid range 20-8000)
      if (r > 20 && r < 8000) {
          vl53loxDistance = r + 20;
          vl53loxReady = (vl53loxDistance < 800);
      } else {
          vl53loxReady = false;
          vl53loxDistance = 8190;
      }
    }
    writeReg(0x0B, 0x01); // Trigger next read
  }

  // Start Module Logic
  if (matchStarted && !stopSequenceRequested) handleIRStopSequence();
  if (stopSequenceRequested) { currentState = MATCH_STOPPED; return; }
  if (robotReadyState && !matchStarted) handleIRStartSequence();
}

// --- Getters & Helpers ---

void handleIRStartSequence() { if (digitalRead(Config::Pins::IR_START_MODULE) == 1) matchStarted = true; }
void handleIRStopSequence() { if (digitalRead(Config::Pins::IR_START_MODULE) == 0) stopSequenceRequested = true; }

// Modifiy this function to ignore the line sensor during RETREAT_TURN state
bool isCenterLineDetected() {
  if (StateMachine::getCurrentState() == StateMachine::State::RETREAT_TURN) {
    return false; // Ignore line sensor during retreat
  }
  return centerLineDetected;
}

bool isCenterLineDetectedRaw() {
  return centerLineDetected;
}

bool isIRModuleConnected() { return robotReadyState; }
bool hasMatchStarted() { 
  return matchStarted; 
}
bool shouldStopRobot() { return stopSequenceRequested; }
void resetStartStopState() { robotReadyState = false; matchStarted = false; stopSequenceRequested = false; }
bool isStartButtonPressed() { return matchStarted; }
bool isStopButtonPressed() { return stopSequenceRequested; }

bool isJS40FDetected(int idx) { return (idx >= 0 && idx < 4) ? js40fStates[idx] : false; }
bool isAnyJS40FDetected() { for(int i=0; i<4; i++) if(js40fStates[i]) return true; return false; }
int getJS40FDetectionPattern() { int p = 0; for(int i=0; i<4; i++) if(js40fStates[i]) p |= (1<<i); return p; }

bool isVL53L0XReady() { return vl53loxReady; }
uint16_t getVL53L0XDistance() { return vl53loxDistance; }
bool isVL53L0XDisabled() { return vlDisableSwitchState; }
bool isDebugModeEnabled() { return debugModeEnabled; }


// Strategy getters
bool getStrategy1() { return strategy1State; }
bool getStrategy2() { return strategy2State; }
bool getStrategy3() { return strategy3State; }
int getStrategyPattern() {
    int pattern = 0;
    if (strategy1State) pattern |= 1;
    if (strategy2State) pattern |= 2;
    if (strategy3State) pattern |= 4;
    return pattern;
}

bool isOpponentInFrontCenter() {
    if (vlDisableSwitchState) return false;
    if (vl53loxReady && vl53loxDistance < 300) return true;
    return false;
}
bool isOpponentDetected() { return isAnyJS40FDetected() || isOpponentInFrontCenter(); }

int getOpponentDirection() {
  // CRITICAL: Dual Front detection takes priority over single sensors
  if (js40fStates[0] && js40fStates[1]) return CENTER;
  if (isOpponentInFrontCenter()) return CENTER;

  // Check for both side sensors
  if (js40fStates[2] && js40fStates[3]) return BOTH_SIDES;

  if (js40fStates[0]) return FRONT_LEFT;
  if (js40fStates[1]) return FRONT_RIGHT;
  if (js40fStates[2]) return SIDE_LEFT;
  if (js40fStates[3]) return SIDE_RIGHT;
  return -1;
}

int getCombinedSensorPattern() {
  // Returns a bitmask of detected sensors for combined pattern detection
  int pattern = 0;
  if (js40fStates[0]) pattern |= (1 << FRONT_LEFT);   // Bit 0
  if (js40fStates[1]) pattern |= (1 << FRONT_RIGHT);  // Bit 1
  if (js40fStates[2]) pattern |= (1 << SIDE_LEFT);    // Bit 2
  if (js40fStates[3]) pattern |= (1 << SIDE_RIGHT);   // Bit 3

  // Check for center detection (either dual front sensors or VL53L0X)
  if (js40fStates[0] && js40fStates[1]) pattern |= (1 << CENTER); // Dual front sensors
  if (isOpponentInFrontCenter()) pattern |= (1 << CENTER); // VL53L0X center detection

  return pattern;
}

int getCenterLineValue() { return 0; }
int getOpponentPosition() { for(int i=0; i<4; i++) if(js40fStates[i]) return i; return -1; }

} // namespace
