#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>

#include "SerialInterface.h"

void SerialInterfaceClass::begin()
{

  _webOn = false;
  _server = new AsyncWebServer(80);

  Serial.println("Init WebServer");
  WebSerial.begin(_server);
  _server->begin();
  updateTag();
}

void SerialInterfaceClass::updateTag()
{
  _tag = millis() / 1000;
}

void SerialInterfaceClass::enableWebSerial(bool activate)
{
  _webOn = activate;
}

void SerialInterfaceClass::msgCallback(RecvMsgHandler msg)
{
  WebSerial.msgCallback(msg);
}

// Print
void SerialInterfaceClass::print(String m)
{
  if (_webOn)
    WebSerial.print(m);
  else
    Serial.print(m);
}

void SerialInterfaceClass::print(const char *m)
{
  if (_webOn)
    WebSerial.print(m);
  else
    Serial.print(m);
}

void SerialInterfaceClass::print(char *m)
{
  if (_webOn)
    WebSerial.print(m);
  else
    Serial.print(m);
}

void SerialInterfaceClass::print(int m)
{
  if (_webOn)
    WebSerial.print(m);
  else
    Serial.print(m);
}

void SerialInterfaceClass::print(uint8_t m)
{
  if (_webOn)
    WebSerial.print(m);
  else
    Serial.print(m);
}

void SerialInterfaceClass::print(uint16_t m)
{
  if (_webOn)
    WebSerial.print(m);
  else
    Serial.print(m);
}

void SerialInterfaceClass::print(uint32_t m)
{
  if (_webOn)
    WebSerial.print(m);
  else
    Serial.print(m);
}

void SerialInterfaceClass::print(double m)
{
  if (_webOn)
    WebSerial.print(m);
  else
    Serial.print(m);
}

void SerialInterfaceClass::print(float m)
{
  if (_webOn)
    WebSerial.print(m);
  else
    Serial.print(m);
}

// Print with New Line

void SerialInterfaceClass::println(String m)
{
  updateTag();
  if (_webOn)
  {
    WebSerial.print(_tag);
    WebSerial.print(" : ");
    WebSerial.println(m);
  }
  else
    Serial.println(m);
}

void SerialInterfaceClass::println(const char *m)
{
  updateTag();
  if (_webOn)
  {
    WebSerial.print(_tag);
    WebSerial.print(" : ");
    WebSerial.println(m);
  }
  else
    Serial.println(m);
}

void SerialInterfaceClass::println(char *m)
{
  updateTag();
  if (_webOn)
  {
    WebSerial.print(_tag);
    WebSerial.print(" : ");
    WebSerial.println(m);
  }
  else
    Serial.println(m);
}

void SerialInterfaceClass::println(int m)
{
  updateTag();
  if (_webOn)
  {
    WebSerial.print(_tag);
    WebSerial.print(" : ");
    WebSerial.println(m);
  }
  else
    Serial.println(m);
}

void SerialInterfaceClass::println(uint8_t m)
{
  updateTag();
  if (_webOn)
  {
    WebSerial.print(_tag);
    WebSerial.print(" : ");
    WebSerial.println(m);
  }
  else
    Serial.println(m);
}

void SerialInterfaceClass::println(uint16_t m)
{
  updateTag();
  if (_webOn)
  {
    WebSerial.print(_tag);
    WebSerial.print(" : ");
    WebSerial.println(m);
  }
  else
    Serial.println(m);
}

void SerialInterfaceClass::println(uint32_t m)
{
  updateTag();
  if (_webOn)
  {
    WebSerial.print(_tag);
    WebSerial.print(" : ");
    WebSerial.println(m);
  }
  else
    Serial.println(m);
}

void SerialInterfaceClass::println(float m)
{
  updateTag();
  if (_webOn)
  {
    WebSerial.print(_tag);
    WebSerial.print(" : ");
    WebSerial.println(m);
  }
  else
    Serial.println(m);
}

void SerialInterfaceClass::println(double m)
{
  updateTag();
  if (_webOn)
  {
    WebSerial.print(_tag);
    WebSerial.print(" : ");
    WebSerial.println(m);
  }
  else
    Serial.println(m);
}

SerialInterfaceClass SerialInterface;