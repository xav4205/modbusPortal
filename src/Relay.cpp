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

  return;
}

/**
 * @brief Active le relay
 *
 */
void Relay::on()
{
  _state = true;
}

/**
 * @brief Eteint le relay
 *
 */
void Relay::off()
{
  _state = false;
}

/**
 * @brief Inverse le relay
 *
 */
bool Relay::toogle()
{
  _state = !_state;
  return _state;
}

/**
 * @brief Active temporairement le relais
 *
 * @param time Temps de la tempo en ms
 */

unsigned long Relay::pulse(unsigned long time)
{

  // TODO Creer une tempo

  return time;
}

/**
 * @brief Routine d'actionnement du relais
 * Doit etre inseré dans la fonction loop()
 *
 */

void Relay::run()
{
  // TODO state -> pin out
  static bool previousState = false;
  if (_state != previousState)
  {
    previousState = _state;
    SerialInterface.print("Changement d'état du relais :");
    if (_state)
      SerialInterface.println("ON");
    else
      SerialInterface.println("OFF");
  }
}
