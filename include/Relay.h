#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>
#include "SerialInterface.h"
#include "config.h"

class Relay
{

public:
  Relay(unsigned int pin);

  void on();
  void off();
  bool toogle();
  unsigned long pulse(unsigned long time);
  void run();

private:
  boolean _state;
  unsigned int _pin;
  unsigned long _timer;
  unsigned long _relayWatchdogTimer;
};

#endif