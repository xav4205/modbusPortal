#include <Arduino.h>
#include "ModbusSlave.h"
#include "ModbusServerWiFi.h"
#include "config.h"
#include "Relay.h"

// #define SERVER_ID 1;
const uint8_t SERVER_ID(1);

ModbusSlave::ModbusSlave()
{
  _onChange = [](const int idx, const int value) {}; // Enregistrement d'une fonction vide
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
      Serial.println("Adresse non valide");
      response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
      return response;
    }
    if (MODBUS_READING_DEBUG)
    {
      Serial.print(millis());
      Serial.print(" : Demande de lecture de ");
      Serial.print(wrds);
      Serial.print(" mots à partir de l'adresse ");
      Serial.println(addr);
    }
    // Modbus address is 1..n, memory address 0..n-1
    addr--;
    // address valid?
    // Number of words valid?
    if (!wrds || (addr + wrds) > MODBUS_HOLDING_REGISTER_SIZE)
    {
      // No. Return error response
      Serial.println("Adresse non valide (depassement)");
      response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
      return response;
    }

    // Prepare response
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(wrds * 2));

    if (MODBUS_READING_DEBUG)
      Serial.println("---------");
      // Loop over all words to be sent
      for (uint16_t i = 0; i < wrds; i++)
      {
        uint16_t val = reg[addr + i];
        // Add word MSB-first to response buffer
         if (MODBUS_READING_DEBUG){

        Serial.print(addr + 400001 + i);
        Serial.print(" -> ");
        Serial.println(val);
         }

        response.add(val);
      }
      if (MODBUS_READING_DEBUG)
      Serial.println("---------");
    
    // Return the data response
    return response;
  };

  return f03;
};

MBSworker ModbusSlave::writeHoldingRegister(uint16_t(reg)[MODBUS_HOLDING_REGISTER_SIZE])
{

  std ::function<ModbusMessage(ModbusMessage)> f10 = [reg](ModbusMessage request) -> ModbusMessage
  {
    uint16_t addr = MODBUS_HOLDING_REGISTER_SIZE + 1; // Start address to write
    uint16_t wrds = 0;                                // Number of words to write
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
      Serial.println("Adresse non valide");
      response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
      return response;
    }

    if (MODBUS_WRITING_DEBUG)
    {

      Serial.print("Demande d'ecriture de ");
      Serial.print(wrds);
      Serial.print(" mots à partir de l'adresse ");
      Serial.print(addr);
      Serial.print(" (");
      Serial.print(bytesCount);
      Serial.println(" bytes)");
    }

    // Modbus address is 1..n, memory address 0..n-1
    addr--;
    // address valid?
    // Number of words valid?
    if (!wrds || (addr + wrds) > MODBUS_HOLDING_REGISTER_SIZE)
    {
      // No. Return error response
      Serial.println("Adresse non valide (depassement)");
      response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
      return response;
    }

    // Prepare response
    // response.add(request.getServerID(), request.getFunctionCode());
    if (MODBUS_WRITING_DEBUG)
    {
      Serial.println("++++++++++");
      // Loop over all words to be sent
      for (uint16_t i = 0; i < wrds; i++)
      {
        uint16_t val = reg[addr + i];
        // Add word MSB-first to response buffer
        Serial.print(addr + 1 + i);
        Serial.print(" <- ");
        Serial.print(values[i]);
        Serial.print(" (");
        Serial.print(val);
        Serial.println(")");

        reg[addr + i] = values[i];
      }

      Serial.println("++++++++");
    }
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

  bool changed = false;
  for (uint16_t i = 0; i < MODBUS_HOLDING_REGISTER_SIZE; ++i)
  {
    if (_previousHoldingRegister[i] != _holdingRegister[i])
    {

      _onChange(i + 1, _holdingRegister[i]);
      changed = true;
    }
    _previousHoldingRegister[i] = _holdingRegister[i];
  }
}

/**
 * @brief Affiche les statistiques du serveur Modbus sur le port serie
 *
 */
void ModbusSlave::printStats()
{

  Serial.print(_MBserver.activeClients());
  Serial.println(" clients running.");
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
    _previousHoldingRegister[i] = 0;
  }
}

/**
 * @brief Affiche les valeurs du registre sur le port série
 *
 */
void ModbusSlave::printHoldingRegisterInfo()
{
  Serial.println("==========");

  for (uint16_t i = 0; i < MODBUS_HOLDING_REGISTER_SIZE; i++)
  {
    // Add word MSB-first to response buffer
    Serial.print(400001 + i);
    Serial.print(" -> ");
    Serial.print(_holdingRegister[i]);
    Serial.print('\n');
  }
  Serial.println("==========");
}

/**
 * @brief Enregistre une fonction à executer lorsque qu'une valeur change dans le registre.
 *
 * Si cette fonction est appelée plusieurs fois, seule la dernière fonction est enregistrée.
 * @param callback Pointeur sur la fonction à exectuer
 * Celle-ci doit etre de type (int index, int valeur)
 *
 */
void ModbusSlave::onChange(ModbusCb callback)
{
  _onChange = callback;
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
  }
  else
    Serial.println("Depassement d'index de registre (Demande d'écriture)");
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
    Serial.println("Depassement d'index de registre (Demande de lecture)");

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
    Serial.print('[');
    Serial.print(firstByte, HEX);
    Serial.print('/');
    Serial.print(lastByte, HEX);
    Serial.println(']');
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