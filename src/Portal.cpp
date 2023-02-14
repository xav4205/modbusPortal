#include <Arduino.h>

#include "Portal.h"
#include "config.h"
#include "SerialInterface.h"

Portal::Portal(Relay *relay, int inputPin)
{

  _relay = relay;
  _inputPin = inputPin;
  _state = PortalState::CLOSED;

  _closeTimer = millis();
  _timer = millis();
  _openTimer = millis();

  return;
}

/**
 * @brief Eteint le relais
 *
 */
void Portal::move()
{
  if (_state == PortalState::OPENED)
    _state = PortalState::CLOSE_PROCESS;

  if (_state == PortalState::CLOSED)
    _state = PortalState::OPEN_PROCESS;

  _relay->pulse(RELAY_PULSE_TIME);
  _openTimer = millis();
  _closeTimer = millis();
}

bool Portal::getSensorState()
{

  return digitalRead(_inputPin);
}

PortalState Portal::getState()
{
  return _state;
}

/**
 * @brief Routine
 * Doit etre inseré dans la fonction loop()
 *
 */

void Portal::run()
{

  if (digitalRead(_inputPin))
    _closeTimer = millis();
  else
    _openTimer = millis();

  if (millis() - _closeTimer >= PORTAL_SENSOR_DEBOUNCE)
  {
    _state = PortalState::CLOSED;
  }
  else if (_state == PortalState::CLOSED)
    _state = PortalState::OPEN_PROCESS;
  
  if (millis() - _openTimer >= PORTAL_OPEN_TIME && _state == PortalState::OPEN_PROCESS)
    _state = PortalState::OPENED;

  _previousState = _state;
}

