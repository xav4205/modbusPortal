#ifndef SIM800_H
#define SIM800_H

#include <Arduino.h>
#include "config.h"


enum endAt
{
  endMark = false,
  returnCarriage = true
};

class Sim800
{

public:
  Sim800();

  void begin(int baudRate, int txPin, int RxPin);
  void sendSms(const String &sender, const String &text);
  bool atCommand(const String at, endAt end);
  void sendEndMark(void);
  int requestSignalQuality();
  bool requestNetworkRegistration();
  void setEchoMode(bool activate);
  void deleteSms();

  void run();
  void test(boolean reset = false);
  bool read(String at = "");

  void sendToLastSender(const String &text);
  String readSms(const String SmsStorePos);

  bool isReady();

//  String findSmsPosition(String message);
//  String getBodyOfSms();

  void process();

private:
  boolean _isReady;

  // String _message;
  boolean _isConnected;
  String _operator;
  bool _messageProcess;
  String _reply[MAX_NB_REPLY];
  String _bodySms;
  String _sender;

  void setTextModeSMS();
  void sendPinCode();

  String extractBetween(String s, String a, String b);

  unsigned long _sim800WatchdogTimer;
};

#endif