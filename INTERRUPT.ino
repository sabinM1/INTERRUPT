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

  // Visual start sequence (~3s delay)
  digitalWrite(Config::Pins::STATUS_LED, !digitalRead(Config::Pins::STATUS_LED));
  delay(2800);
  digitalWrite(Config::Pins::STATUS_LED, !digitalRead(Config::Pins::STATUS_LED));
  delay(100);
  digitalWrite(Config::Pins::STATUS_LED, !digitalRead(Config::Pins::STATUS_LED));
  delay(100);
  digitalWrite(Config::Pins::STATUS_LED, !digitalRead(Config::Pins::STATUS_LED));

  Serial.println("System ready.");
}

void loop() {
  unsigned long currentTime = millis();

  Sensors::update(currentTime);
  StateMachine::update(currentTime);
  Motors::update(currentTime);
  Utils::update(currentTime);
}
