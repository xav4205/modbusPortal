#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <AsyncTCP.h>

#include "config.h"
#include "ModbusSlave.h"
#include "ModbusServerWiFi.h"
#include "SerialInterface.h"
#include "Relay.h"
#include "Portal.h"

// AsyncWebServer server(80);

Relay relay1(OUTPUT_1_PIN);
Relay relay2(OUTPUT_2_PIN);

ModbusSlave modbus;

Portal portal(&relay1, INPUT_1_PIN);

// Surveille les entrées du WebSerial
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
    modbus.setHoldingRegister(MODMAP_PORTAL_REQUEST, 1);

  if (d == "OFF")
    modbus.setHoldingRegister(MODMAP_PORTAL_REQUEST, 0);
}

void setup()
{

  pinMode(INPUT_1_PIN, PULLUP);
  pinMode(INPUT_2_PIN, PULLUP);

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
  ArduinoOTA.setPassword(ESP_PASSWORD);
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
  modbus.onChange(
      [](const int &idx, const int &value)
      {
        if (idx == MODMAP_PORTAL_REQUEST)
        {
          if (value)
          {
            SerialInterface.println("MODBUS : Debut demande");
            portal.move();
          }
          else
            SerialInterface.println("MODBUS : Fin demande");
        }
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
  static unsigned int wifiCounter = 0;

  if (millis() - watchDog >= WATCHDOG_TIMER)
  {
    watchDog = millis();

    // == WIFI ==
    if (WiFi.status() == WL_CONNECTED)
    {
      modbus.setHoldingRegister(MODMAP_WIFI_SIGNAL_LEVEL, -WiFi.RSSI());
      wifiCounter = 0;
    }
    else
    {
      wifiCounter++;

      Wifi.disconnect();
      delay(1000);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      delay(200);

      uint restartTimer = 60;
      while (WiFi.status() != WL_CONNECTED)
      {
        Serial.print('.');
        delay(1000);
        if (restartTimer <= 0)
          ESP.restart();
        restartTimer--;
      }
    }

    if (wifiCounter > WIFI_RESTART)
    {
      ESP.restart()
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

  // Copie l'état des relais dans le registre modbus
  modbus.setHoldingRegister(MODMAP_OUTPUT_1, relay1.getState());
  modbus.setHoldingRegister(MODMAP_OUTPUT_2, relay2.getState());
  modbus.setHoldingRegister(MODMAP_PORTAL_STATE, (int)portal.getState());
  modbus.setHoldingRegister(MODMAP_MAGNET_SENSOR, portal.getSensorState());

  // Routine du serveur Modbus
  modbus.run();
  portal.run();
  relay1.run();
  relay2.run();

  /*
    pinMode(OUTPUT_1_PIN, OUTPUT);
    pinMode(OUTPUT_2_PIN, OUTPUT);

    digitalWrite(OUTPUT_1_PIN, true);
    digitalWrite(OUTPUT_2_PIN, false);

    delay(1000);
        digitalWrite(OUTPUT_1_PIN, false);
    digitalWrite(OUTPUT_2_PIN, true);
    delay(2000);

    */
}