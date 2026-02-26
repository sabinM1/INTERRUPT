#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <Arduino.h>
#include "Config.h"

namespace StateMachine {
  enum class State : uint8_t {
    STARTUP = 0,
    SEARCH = 1,
    TRACKING = 2,
    RETREAT_TURN = 3,
    ATTACK = 4,
    STOPPED = 5,
    BOTH_SIDES_ATTACK = 6
  };

  void init();
  void update(unsigned long currentTime);

  State getCurrentState();
  const char* getStateName();

  // Debug helpers
  int getLastOpponentDirection();
  const char* getDirectionName(int direction);

  void updateOpponentMemory(int direction);
}

#endif
