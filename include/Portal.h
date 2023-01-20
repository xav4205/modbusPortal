#ifndef PORTAL_H
#define PORTAL_H

#include <Arduino.h>
#include "SerialInterface.h"
#include "config.h"
#include "Relay.h"

enum class PortalState
{
  CLOSED,
  OPEN_PROCESS,
  OPENED,
  CLOSE_PROCESS,
  INDEFINITE
};

class Portal
{

public:
  Portal(Relay *relay, int inputPin);

  void move();
  bool getSensorState();
  PortalState getState();
  void run();

private:
  PortalState _state;
  PortalState _previousState;
  unsigned long _timer;
  unsigned long _openTimer;
  unsigned long _closeTimer;
  int _inputPin;
  Relay *_relay;
};

#endif