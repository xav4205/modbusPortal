#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>
#include "config.h"

class Relay
{

public:
  Relay();

  void on();
  void off();
  bool toogle();
  unsigned long pulse(unsigned long time);
  void process();

private:
  boolean _state;
  // String _message;
  unsigned long _timer;
  unsigned long _relayWatchdogTimer;
};

#endif