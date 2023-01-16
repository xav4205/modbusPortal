#ifndef SERIALINTERFACE_H
#define SERIALINTERFACE_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>

class SerialInterfaceClass
{

public:
  void enableWebSerial(bool activate = true);

  // Register callback function when data is received
  void msgCallback(RecvMsgHandler _recv);
  // Print

  void print(String m);
  void print(const char *m);
  void print(char *m);
  void print(int m);
  void print(uint8_t m);
  void print(uint16_t m);
  void print(uint32_t m);
  void print(double m);
  void print(float m);

  // Print with New Line

  void println(String m);
  void println(const char *m);
  void println(char *m);
  void println(int m);
  void println(uint8_t m);
  void println(uint16_t m);
  void println(uint32_t m);
  void println(float m);
  void println(double m);

  void begin();

private:
  void updateTag();

  AsyncWebServer *_server;
  bool _webOn;
  uint32_t _tag;
};

extern SerialInterfaceClass SerialInterface;
#endif