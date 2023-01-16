#include <Arduino.h>
#include "ModbusSlave.h"
#include "ModbusServerWiFi.h"
#include "config.h"
#include <SerialInterface.h>

// #define SERVER_ID 1;
const uint8_t SERVER_ID(1);

ModbusSlave::ModbusSlave()
{
  _messageWorker = [](const String &recipient, const String &text) {}; // Enregistrement d'une fonction vide
}

MBSworker ModbusSlave::readHoldingRegister(uint16_t(reg)[MODBUS_HOLDING_REGISTER_SIZE])
{

  std ::function<ModbusMessage(ModbusMessage)> f03 = [reg](ModbusMessage request) -> ModbusMessage
  {
    uint16_t addr = MODBUS_HOLDING_REGISTER_SIZE; // Start address to read
    uint16_t wrds = 0;                            // Number of words to read
    ModbusMessage response;

    // Get addr and words from data array. Values are MSB-first, get() will convert to binary
    request.get(2, addr);
    request.get(4, wrds);

    addr++;

    if (addr > MODBUS_HOLDING_REGISTER_SIZE)
    {
      // No. Return error response
      SerialInterface.println("Adresse non valide");
      response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
      return response;
    }

    SerialInterface.print("Demande de lecture de ");
    SerialInterface.print(wrds);
    SerialInterface.print(" mots à partir de l'adresse ");
    SerialInterface.println(addr);

    // Modbus address is 1..n, memory address 0..n-1
    addr--;
    // address valid?
    // Number of words valid?
    if (!wrds || (addr + wrds) > MODBUS_HOLDING_REGISTER_SIZE)
    {
      // No. Return error response
      SerialInterface.println("Adresse non valide (depassement)");
      response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
      return response;
    }

    // Prepare response
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(wrds * 2));

    SerialInterface.println("---------");
    // Loop over all words to be sent
    for (uint16_t i = 0; i < wrds; i++)
    {
      uint16_t val = reg[addr + i];
      // Add word MSB-first to response buffer
      SerialInterface.print(addr + 400001 + i);
      SerialInterface.print(" -> ");
      SerialInterface.println(val);

      response.add(val);
    }
    SerialInterface.println("---------");

    // Return the data response
    return response;
  };

  return f03;
};

MBSworker ModbusSlave::writeHoldingRegister(uint16_t(reg)[MODBUS_HOLDING_REGISTER_SIZE])
{

  std ::function<ModbusMessage(ModbusMessage)> f10 = [reg](ModbusMessage request) -> ModbusMessage
  {
    uint16_t addr = MODBUS_HOLDING_REGISTER_SIZE + 1; // Start address to read
    uint16_t wrds = 0;                                // Number of words to read
    uint8_t bytesCount = 0;
    std::vector<uint16_t> values;
    ModbusMessage response;

    // Get addr and words from data array. Values are MSB-first, get() will convert to binary
    request.get(2, addr);
    request.get(4, wrds);
    request.get(6, bytesCount);

    for (uint8_t i = 0; i < bytesCount; i += 2)
    {
      uint16_t val;
      request.get(7 + i, val);
      values.push_back(val);
    }

    // request.get(8, values, bytesCount);

    addr++;

    if (addr > MODBUS_HOLDING_REGISTER_SIZE)
    {
      // No. Return error response
      SerialInterface.println("Adresse non valide");
      response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
      return response;
    }

    SerialInterface.print("Demande d'ecriture de ");
    SerialInterface.print(wrds);
    SerialInterface.print(" mots à partir de l'adresse ");
    SerialInterface.print(addr);
    SerialInterface.print(" (");
    SerialInterface.print(bytesCount);
    SerialInterface.println(" bytes)");

    // Modbus address is 1..n, memory address 0..n-1
    addr--;
    // address valid?
    // Number of words valid?
    if (!wrds || (addr + wrds) > MODBUS_HOLDING_REGISTER_SIZE)
    {
      // No. Return error response
      SerialInterface.println("Adresse non valide (depassement)");
      response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
      return response;
    }

    // Prepare response
    // response.add(request.getServerID(), request.getFunctionCode());

    SerialInterface.println("++++++++++");
    // Loop over all words to be sent
    for (uint16_t i = 0; i < wrds; i++)
    {
      uint16_t val = reg[addr + i];
      // Add word MSB-first to response buffer
      SerialInterface.print(addr + 1 + i);
      SerialInterface.print(" <- ");
      SerialInterface.print(values[i]);
      SerialInterface.print(" (");
      SerialInterface.print(val);
      SerialInterface.println(")");

      reg[addr + i] = values[i];
    }

    SerialInterface.println("++++++++");

    response = ECHO_RESPONSE;
    //  response.add(request.getServerID(), request.getFunctionCode(), addr+1, wrds);
    // Return the data response
    return response;
  };

  return f10;
};

void ModbusSlave::init()
{

  // Enregistre la fonction qui sera executée en cas de demande de lecture du registre d'exploitation (fonction 03)
  _MBserver.registerWorker(SERVER_ID, READ_HOLD_REGISTER, readHoldingRegister(_holdingRegister));

  // Enregistre la fonction qui sera executée en cas de demande de lecture du registre d'exploitation (fonction 16)
  _MBserver.registerWorker(SERVER_ID, WRITE_MULT_REGISTERS, writeHoldingRegister(_holdingRegister));

  clearHoldingRegister();

  // Demarre le serveur Modbus TCP/IP:
  // Port 502, maximum de 4 clients en parallèle, 10 seconds de timeout
  _MBserver.start(502, 4, 10000);
  return;
}

/**
 * @brief Routine
 * - Va chercher les info du wifi
 * - Déclenche l'execution de la fonction d'envoi quand le jeton "new message" passe à 1
 *
 */
void ModbusSlave::run()
{
  static unsigned long watchDog = millis();

  if (millis() - watchDog > MODBUS_SERVER_WATCHDOG)
  {

    watchDog = millis();
  }
}

/**
 * @brief Affiche les statistiques du serveur Modbus sur le port serie
 *
 */
void ModbusSlave::printStats()
{

  SerialInterface.print(_MBserver.activeClients());
  SerialInterface.println(" clients running.");
}

/**
 * @brief Vide le registre d'execution
 *
 */
void ModbusSlave::clearHoldingRegister()
{
  // Initialize server memory with consecutive values
  for (uint16_t i = 0; i < MODBUS_HOLDING_REGISTER_SIZE; ++i)
  {
    _holdingRegister[i] = 0;
  }
}

/**
 * @brief Affiche les valeurs du registre sur le port série
 *
 */
void ModbusSlave::printHoldingRegisterInfo()
{
  SerialInterface.println("==========");

  for (uint16_t i = 0; i < MODBUS_HOLDING_REGISTER_SIZE; i++)
  {
    // Add word MSB-first to response buffer
    SerialInterface.print(400001 + i);
    SerialInterface.print(" -> ");
    SerialInterface.println(_holdingRegister[i]);
  }
  SerialInterface.println("==========");
}

/**
 * @brief Enregistre une fonction à executer lorsque qu'un message est reçu.
 *
 * Si cette fonction est appelée plusieurs fois, seule la dernière fonction est enregistrée.
 * @param callback Pointeur sur la fonction à exectuer
 * Celle-ci doit etre de type (String destinataire, String text)
 *
 */
void ModbusSlave::registerMessageWorker(MessageCb callback)
{
  _messageWorker = callback;
}

/**
 * @brief Ecrit une valeur dans un registre
 *
 * @param idx Index du registre à ecrire
 */
void ModbusSlave::setHoldingRegister(uint16_t idx, uint16_t value)
{
  // DebugSerial.printf("Set HRegister [%d] <- %d\n", idx, value);
  if (idx > 0 && idx <= MODBUS_HOLDING_REGISTER_SIZE)
  {
    _holdingRegister[idx - 1] = value;
    if (MODBUS_DEBUG)
    {
      SerialInterface.println("Demande d'écriture");
      printHoldingRegisterInfo();
    }
  }
  else
    SerialInterface.println("Depassement d'index de registre (Demande d'écriture)");

}

/**
 * @brief Lie la valeur stockée dans un registre
 *
 * @param idx Index du registre à lire
 * @return Valeur extraite du registre
 */
uint16_t ModbusSlave::getHoldingRegister(uint16_t idx)
{
  if (idx > 0 && idx <= MODBUS_HOLDING_REGISTER_SIZE)
    return _holdingRegister[idx - 1];
  else
    SerialInterface.println("Depassement d'index de registre (Demande de lecture)");

  return 0;
}

/**
 * @brief Convertit les octets stockés en modbus en texte
 *
 * @param firstRegister Index du premier registre à lire
 * @param size Nombre de registre à lire
 * @return Texte extrait du registre
 */
String ModbusSlave::toString(int firstRegister, int size)
{
  String msg = "";

  for (int i = 0; i < size; i++)
  {
    const int16_t value = getHoldingRegister(i + firstRegister);

    uint8_t lastByte = (value & 0xFF);         // extract first byte
    uint8_t firstByte = ((value >> 8) & 0xFF); // extract second byte
                                               /*
    SerialInterface.print('[');
    SerialInterface.print(firstByte, HEX);
    SerialInterface.print('/');
    SerialInterface.print(lastByte, HEX);
    SerialInterface.println(']');
    */
    if (firstByte)
      msg += char(firstByte);
    else
      break;

    if (lastByte)
      msg += char(lastByte);
    else
      break;
  }
  /*
    for (int i = 0; i < size; i++)
    {
      if (msg[i])
      {
        DebugSerial.write(msg[i]);
        DebugSerial.print('(');
        DebugSerial.print(msg[i]);
        DebugSerial.print(')');
      }
      else
        break;
    }
  */
  return msg;
}