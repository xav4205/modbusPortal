#include <Arduino.h>
#include <WebSerial.h>

#include "Relay.h"
#include "config.h"

Relay::Relay()
{

  _state = false;
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

void Relay::process()
{
  // TODO state -> pin out
}
