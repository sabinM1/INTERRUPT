#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <Arduino.h>
#include "Config.h"

namespace StateMachine {
  enum class State : uint8_t {
    STARTUP = 0,
    OPENER = 1,
    SEARCH = 2,
    TRACKING = 3,
    RETREAT_TURN = 4,
    ATTACK = 5,
    STOPPED = 6,
    BOTH_SIDES_ATTACK = 7
  };

  void init();
  void update(unsigned long currentTime);

  State getCurrentState();
  const char* getStateName();

  // Debug helpers
  int getLastOpponentDirection();
  const char* getDirectionName(int direction);

  void updateOpponentMemory(int direction);

  // Attack timer access
  unsigned long getLastAttackTime();
  bool isAttackPowerLimited(unsigned long currentTime);
}

#endif
