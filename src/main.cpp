#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <AsyncTCP.h>

#include "config.h"
#include "Relay.h"
#include "ModbusSlave.h"
#include "ModbusServerWiFi.h"
#include "SerialInterface.h"

// AsyncWebServer server(80);

Relay relay(0);

ModbusSlave modbus;

// Surveille les entrée du WebSerial
void recvMsg(uint8_t *data, size_t len)
{
  SerialInterface.println("Received Data...");

  String d = "";
  for (int i = 0; i < len; i++)
  {
    d += char(data[i]);
  }
  SerialInterface.println(d);
  if (d == "ON")
  {
    relay.on();
  }
  if (d == "OFF")
  {
    relay.off();
  }

  modbus.printHoldingRegisterInfo();
}

void setup()
{
  //=========== Initialialisation du port Serie ==========
  Serial.begin(115200);

  while (!Serial)
  {
    ;
  }

  //=========== Initialialisation de la station Wifi ==========

  // Initialise les IP Configurer l'ip dans le fichier platform.ini
  IPAddress ip = IPAddress((int[])WIFI_LOCAL_IP[0], (int[])WIFI_LOCAL_IP[1], (int[])WIFI_LOCAL_IP[2], (int[])WIFI_LOCAL_IP[3]);
  IPAddress dns = IPAddress((int[])WIFI_DNS[0], (int[])WIFI_DNS[1], (int[])WIFI_DNS[2], (int[])WIFI_DNS[3]);
  IPAddress gateway = IPAddress((int[])WIFI_GATEWAY[0], (int[])WIFI_GATEWAY[1], (int[])WIFI_GATEWAY[2], (int[])WIFI_GATEWAY[3]);
  IPAddress subnet = IPAddress((int[])WIFI_SUBNET[0], (int[])WIFI_SUBNET[1], (int[])WIFI_SUBNET[2], (int[])WIFI_SUBNET[3]);

  // Configure et demarre la station Wifi
  Serial.println("Booting");

  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet, dns);
  delay(200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  delay(200);
  Serial.print("Connection au reseau WIFI");
  uint restartTimer = 60;
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
    if (restartTimer <= 0)
      ESP.restart();
    restartTimer--;
  }
  Serial.println();

  // print local WIFI_LOCAL_IP address:
  IPAddress localIP = WiFi.localIP();
  Serial.printf("Adresse IP locale: %u.%u.%u.%u\n", localIP[0], localIP[1], localIP[2], localIP[3]);
  Serial.println();

  //=========== Initialialisation du serveur WebSerial (port serie distant) ==========

  SerialInterface.begin();

  if (WEB_SERIAL)
  {
    SerialInterface.enableWebSerial();
    SerialInterface.msgCallback(recvMsg);
  }

  //=========== Initialialisation du serveur OTA (upload firmware par le Wifi) ==========

  // Port defaults to 3232
  ArduinoOTA.setHostname("Modbusportal ESP32");
  ArduinoOTA.setPassword("portal");
  ArduinoOTA.onStart([]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                         type = "sketch";
                       else // U_SPIFFS
                         type = "filesystem";

                       // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()

                       SerialInterface.println("Start updating " + type); });
  ArduinoOTA.onEnd([]()
                   { SerialInterface.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        {
                          SerialInterface.print(".");
                          SerialInterface.println((progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
                       SerialInterface.print("Error[");
                       SerialInterface.print(error);
                       SerialInterface.print("]: ");
                       if (error == OTA_AUTH_ERROR)
                         SerialInterface.println("Auth Failed");
                       else if (error == OTA_BEGIN_ERROR)
                         SerialInterface.println("Begin Failed");
                       else if (error == OTA_CONNECT_ERROR)
                         SerialInterface.println("Connect Failed");
                       else if (error == OTA_RECEIVE_ERROR)
                         SerialInterface.println("Receive Failed");
                       else if (error == OTA_END_ERROR)
                         SerialInterface.println("End Failed"); });
  ArduinoOTA.begin();

  //=========== Initialialisation du serveur Modbus ==========
  modbus.init();
  // Enregistrement de la fonction d'envoi
  modbus.registerMessageWorker(
      [](const String &recipient, const String &text)
      {
        SerialInterface.println("Envoi d'un message");
        SerialInterface.print("Destinataire => ");
        SerialInterface.println(recipient);
        SerialInterface.print("Message => ");
        SerialInterface.println(text);
        /*
                if (sim800.sendSms(recipient, text))
                  modbus.messageSent();
                  */
      });

  Serial.printf("Initialisation terminée");
}

void loop()
{

  // Routine du serveur OTA
  ArduinoOTA.handle();

  // Fonction cyclique
  // Lie les stats du module SIM800 et les integrent au registre Modbus correspondant
  // Préleve les infos du Wifi et les stocke dans le registre d'éxecution
  static unsigned long watchDog = millis();
  if (millis() - watchDog >= WATCHDOG_TIMER)
  {
    watchDog = millis();

    // == WIFI ==
    if (WiFi.status() == WL_CONNECTED)
    {
      modbus.setHoldingRegister(MODMAP_WIFI_SIGNAL_LEVEL, -WiFi.RSSI());
    }
    modbus.setHoldingRegister(MODMAP_WIFI_STATUS, WiFi.status());

    // == MODBUS SerialInterface ==

    if ((modbus.getHoldingRegister(MODMAP_LIVE_WORD) - modbus.getHoldingRegister(MODMAP_LIVE_WORD_ECHO)) > 5)
    {
      SerialInterface.println("Communication avec l'automate perdue");
    }
    else
      SerialInterface.println("Communication avec l'automate etablie");
  }

  static unsigned long liveWordTimer = millis();

  if (millis() - liveWordTimer >= 1000)
  {
    liveWordTimer = millis();
    unsigned int liveWord = liveWordTimer / 1000;
    modbus.setHoldingRegister(MODMAP_LIVE_WORD, liveWord);
  }

  // Routine du serveur Modbus
  modbus.run();

  // Routine du module SIM800
  relay.run();
}