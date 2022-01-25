#include <Arduino.h>
#include "ModbusSlave.h"
#include "ModbusServerWiFi.h"
#include "config.h"
#include <WebSerial.h>

//#define SERVER_ID 1;
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
      Serial.println("Adresse non valide");
      response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
      return response;
    }

    Serial.print("Demande de lecture de ");
    Serial.print(wrds);
    Serial.print(" mots à partir de l'adresse ");
    Serial.println(addr);

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

    Serial.println("---------");
    // Loop over all words to be sent
    for (uint16_t i = 0; i < wrds; i++)
    {
      uint16_t val = reg[addr + i];
      // Add word MSB-first to response buffer
      Serial.print(addr + 400001 + i);
      Serial.print(" -> ");
      Serial.println(val);

      response.add(val);
    }
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
      Serial.println("Adresse non valide");
      response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
      return response;
    }

    Serial.print("Demande d'ecriture de ");
    Serial.print(wrds);
    Serial.print(" mots à partir de l'adresse ");
    Serial.print(addr);
    Serial.print(" (");
    Serial.print(bytesCount);
    Serial.println(" bytes)");

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
  unsigned int messageStatus = getHoldingRegister(MODMAP_SEND_STATE);

  if (millis() - watchDog > MODBUS_SERVER_WATCHDOG)
  {

    watchDog = millis();
  }

  if (getHoldingRegister(MODMAP_SEND_MESSAGE) && messageStatus == MESSAGE_READY_TO_SEND)
  {
    setHoldingRegister(MODMAP_SEND_STATE, MESSAGE_IN_PROGRESS);
    sendMessageRequest();
  }

  // Empeche l'envoi de message multiple
  static unsigned long messageTimer = millis();

  switch (getHoldingRegister(MODMAP_SEND_STATE))
  {
  case MESSAGE_READY_TO_SEND:
    messageTimer = millis();
    break;
  case MESSAGE_IN_PROGRESS:

    if (millis() - messageTimer > MESSAGE_TIME_BETWEEN)
    {
      messageTimer = millis();
      setHoldingRegister(MODMAP_SEND_STATE, MESSAGE_ERROR);
    }

    break;
  case MESSAGE_SENT:
    if (getHoldingRegister(MODMAP_MESSAGE_ACK))
    {
      setHoldingRegister(MODMAP_SEND_STATE, MESSAGE_IS_ACK);
    }

    if (millis() - messageTimer > MESSAGE_TIME_BETWEEN)
    {
      WebSerial.println("Pas d'acquitement suite à l'envoi");
      messageTimer = millis();
      setHoldingRegister(MODMAP_SEND_STATE, MESSAGE_ERROR);
    }

    /* if (millis() - messageTimer[MESSAGE_IN_PROGRESS] > MESSAGE_TIME_BETWEEN)
      setHoldingRegister(MODMAP_SEND_STATE, MESSAGE_ERROR);
    messageTimer[MESSAGE_SENT] = millis();
    messageTimer[3] = millis();*/
    break;
  case MESSAGE_IS_ACK:

    setHoldingRegister(MODMAP_SEND_STATE, MESSAGE_READY_TO_SEND);
    break;
  case MESSAGE_ERROR:

    if (getHoldingRegister(MODMAP_MESSAGE_ACK))
    {
      setHoldingRegister(MODMAP_SEND_STATE, MESSAGE_IS_ACK);
    }

    if (millis() - messageTimer > MESSAGE_TIME_BETWEEN)
    {
      WebSerial.println("Pas d'acquitement suite à l'erreur");
      setHoldingRegister(MODMAP_SEND_STATE, MESSAGE_READY_TO_SEND);
    }

    break;

  default:
    break;
  }
}

/**
 * @brief Affidche les statistiques du serveur Modbus sur le port serie
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
  }
}

/**
 * @brief Affiche les valeurs du registre sur le port série
 * 
 */
void ModbusSlave::printHoldingRegisterInfo()
{
  WebSerial.println("==========");

  for (uint16_t i = 0; i < MODBUS_HOLDING_REGISTER_SIZE; i++)
  {
    // Add word MSB-first to response buffer
    WebSerial.print(400001 + i);
    WebSerial.print(" -> ");
    WebSerial.println(_holdingRegister[i]);
  }
  WebSerial.println("==========");
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
 * @brief Execute la fonction enregistrée grace à registerMessageWorker
 * en prélevant le numero et le texte dans le registre d'éxecution
 * 
 */

void ModbusSlave::messageSent()
{
  setHoldingRegister(MODMAP_SEND_STATE, MESSAGE_SENT);
  WebSerial.println("Message envoyé");
}

/**
 * @brief Execute la fonction enregistrée grace à registerMessageWorker
 * en prélevant le numero et le texte dans le registre d'éxecution
 * 
 */

void ModbusSlave::sendMessageRequest()
{

  _messageWorker(
      toString(MODMAP_FIRST_PHONE_NUMBER_REGISTER, MODMAP_PHONE_NUMBER_SIZE_MESSAGE),
      toString(MODMAP_FIRST_MESSAGE_REGISTER, MODMAP_MAX_SIZE_MESSAGE));

  clearMessage();
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
    _holdingRegister[idx - 1] = value;

  return;
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

  return 0;
}

/**
 * @brief Efface le message du registre correspondant
 * 
 */
void ModbusSlave::clearMessage()
{
  for (uint8_t i = MODMAP_FIRST_MESSAGE_REGISTER; i < MODMAP_FIRST_MESSAGE_REGISTER + MODMAP_MAX_SIZE_MESSAGE; i++)
  {
    setHoldingRegister(i, 0);
  }

  return;
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