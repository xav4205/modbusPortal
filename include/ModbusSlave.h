#ifndef MODBUS_SLAVE_H
#define MODBUS_SLAVE_H
#include <Arduino.h>
#include "ModbusServerWiFi.h"
#include "config.h"


using MessageCb = std::function<void(const String &recipient, const String &text)>;

class ModbusSlave
{
public:
  ModbusSlave();

  void run();
  void init();

  void registerMessageWorker(MessageCb callback);
  void setHoldingRegister(uint16_t idx, uint16_t value);
  uint16_t getHoldingRegister(uint16_t idx);
  void printHoldingRegisterInfo();
  void printStats();


private:
  // Set up a Modbus server
  ModbusServerWiFi _MBserver;
  uint16_t _holdingRegister[MODBUS_HOLDING_REGISTER_SIZE]; // Test server memory

  MessageCb _messageWorker;

  void clearHoldingRegister();
  void clearMessage();
  void newMessageReceived();
  static MBSworker readHoldingRegister(uint16_t(reg)[MODBUS_HOLDING_REGISTER_SIZE]);
  static MBSworker writeHoldingRegister(uint16_t(reg)[MODBUS_HOLDING_REGISTER_SIZE]);

  String toString(int firstRegister, int size);
};

#endif