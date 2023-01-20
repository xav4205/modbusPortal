#ifndef MODBUS_SLAVE_H
#define MODBUS_SLAVE_H
#include <Arduino.h>
#include "ModbusServerWiFi.h"
#include "config.h"
#include "Relay.h"

using ModbusCb = std::function<void(const int idx, const int value)>;

class ModbusSlave
{
public:
  ModbusSlave();

  void run();
  void init();

  void onChange(ModbusCb callback);
  void setHoldingRegister(uint16_t idx, uint16_t value);
  uint16_t getHoldingRegister(uint16_t idx);
  void printHoldingRegisterInfo();
  void printStats();

private:
  // Set up a Modbus server
  ModbusServerWiFi _MBserver;
  uint16_t _holdingRegister[MODBUS_HOLDING_REGISTER_SIZE];         // Test server memory
  uint16_t _previousHoldingRegister[MODBUS_HOLDING_REGISTER_SIZE]; // Test server memory
  ModbusCb _onChange;

  void clearHoldingRegister();

  static MBSworker readHoldingRegister(uint16_t(reg)[MODBUS_HOLDING_REGISTER_SIZE]);
  static MBSworker writeHoldingRegister(uint16_t(reg)[MODBUS_HOLDING_REGISTER_SIZE]);

  String toString(int firstRegister, int size);
};

#endif