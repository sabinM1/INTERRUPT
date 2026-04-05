/* INTERRUPT - Main Entry */

#include "Config.h"
#include "Sensors.h"
#include "Motors.h"
#include "StateMachine.h"
#include "Utils.h"

void setup() {
  Serial.begin(9600);
  Serial.println("=== INTERRUPT ===");

  Motors::init();
  Sensors::init();
  StateMachine::init();
  Utils::init();



  Serial.println("System ready.");
}

void loop() {
  unsigned long currentTime = millis();

  Sensors::update(currentTime);
  StateMachine::update(currentTime);
  Motors::update(currentTime);
  Utils::update(currentTime);
}
