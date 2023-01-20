#include <Arduino.h>

#include "Relay.h"
#include "config.h"
#include "SerialInterface.h"

Relay::Relay(unsigned int pin)
{

  _state = false;
  _pin = pin;
  _timer = millis();
  _relayWatchdogTimer = millis();

  pinMode(pin, OUTPUT);

  return;
}

/**
 * @brief Active le relay
 *
 */
void Relay::on()
{
  if (!_state)
    SerialInterface.println("Changement d'état du relais : ON");
  _state = true;
}

/**
 * @brief Eteint le relay
 *
 */
void Relay::off()
{
  if (_state)
    SerialInterface.println("Changement d'état du relais : OFF");
  _state = false;
}

/**
 * @brief Inverse le relay
 *
 */
bool Relay::toogle()
{
  SerialInterface.print("TOOGLE");
  _state = !_state;
  return _state;
}

/**
 * @brief Active temporairement le relais
 *
 * @param time Temps de la tempo en ms
 */

void Relay::pulse(unsigned long time)
{

  SerialInterface.print("PULSE");
  _timerIsActive = true;
  _setpointTimer = time;
  _timer = millis();

  return;
}

/**
 * @brief Routine d'actionnement du relais
 * Doit etre inseré dans la fonction loop()
 *
 */

void Relay::run()
{

  if (_timerIsActive)
  {
    if (millis() - _timer <= _setpointTimer)
      on();
    else
    {
      _timerIsActive = false;
      off();
    }
  }

  digitalWrite(_pin, !_state);
}

bool Relay::getState()
{
  return _state;
}
