#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>
#include "SerialInterface.h"
#include "config.h"

using RelayCb = std::function<void(bool state)>;

class Relay
{

public:
  Relay(unsigned int pin);

  void on();
  void off();
  bool toogle();
  void pulse(unsigned long time);
  void run();
  bool getState();

private:
  boolean _state;
  unsigned int _pin;
  unsigned long _timer;
  unsigned long _setpointTimer;
  bool _timerIsActive;
  unsigned long _relayWatchdogTimer;
};

#endif