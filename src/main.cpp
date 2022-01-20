#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <AsyncTCP.h>

#include <ESPAsyncWebServer.h>
#include <WebSerial.h>

#include "config.h"
#include "SIM800.h"
#include "ModbusSlave.h"
#include "ModbusServerWiFi.h"

AsyncWebServer server(80);

Sim800 sim800;

ModbusSlave modbus;

// Surveille les entrée du WebSerial
void recvMsg(uint8_t *data, size_t len)
{
  WebSerial.println("Send AT command...");

  bool sms = false;
  String str = "";
  for (int i = 0; i < len; i++)
  {
    if (char(data[i]) == '&')
    {
      WebSerial.println("Detect Ctrl+Z");
      sms = true;
      break;
    }

    str += char(data[i]);
  }

  if (sms)
    sim800.atCommand(str, endAt::endMark);
  else
    sim800.atCommand(str, endAt::returnCarriage);
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

  // Initialise les IP selon le fichier config.h
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

  // print local WIFI_LOCAL_IP address:
  IPAddress localIP = WiFi.localIP();
  Serial.printf("Addresse IP locale: %u.%u.%u.%u\n", localIP[0], localIP[1], localIP[2], localIP[3]);

  //=========== Initialialisation du serveur WebSerial (port serie distant) ==========

  WebSerial.begin(&server);
  WebSerial.msgCallback(recvMsg);
  server.begin();

  //=========== Initialialisation du serveur OTA (upload firmware par le Wifi) ==========

  // Port defaults to 3232
  ArduinoOTA.setHostname("ModbusSms ESP32");
  ArduinoOTA.setPassword("biogaz");
  ArduinoOTA.onStart([]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                         type = "sketch";
                       else // U_SPIFFS
                         type = "filesystem";

                       // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                       WebSerial.println("Start updating " + type);
                     });
  ArduinoOTA.onEnd([]()
                   { WebSerial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        {
                          WebSerial.print("Progress: ");
                          WebSerial.println((progress / (total / 100)));
                        });
  ArduinoOTA.onError([](ota_error_t error)
                     {
                       WebSerial.print("Error[");
                       WebSerial.print(error);
                       WebSerial.print("]: ");
                       if (error == OTA_AUTH_ERROR)
                         WebSerial.println("Auth Failed");
                       else if (error == OTA_BEGIN_ERROR)
                         WebSerial.println("Begin Failed");
                       else if (error == OTA_CONNECT_ERROR)
                         WebSerial.println("Connect Failed");
                       else if (error == OTA_RECEIVE_ERROR)
                         WebSerial.println("Receive Failed");
                       else if (error == OTA_END_ERROR)
                         WebSerial.println("End Failed");
                     });
  ArduinoOTA.begin();

  //=========== Initialialisation du module SIM800 ==========
  sim800.begin(SIM800_UART_BAUDRATE, SIM800_TX_PIN, SIM800_RX_PIN);

  //=========== Initialialisation du serveur Modbus ==========
  modbus.init();
  // Enregistrement de la fonction d'envoi
  modbus.registerMessageWorker(
      [](const String &recipient, const String &text)
      {
        WebSerial.println("Envoi d'un message");
        WebSerial.print("Destinataire => ");
        WebSerial.println(recipient);
        WebSerial.print("Message => ");
        WebSerial.println(recipient);
        //sim800.sendSms(recipient, text);
      });
}

void loop()
{

  // Routine du serveur OTA
  ArduinoOTA.handle();

  // Fonction cyclique
  // Lie les stats du module SIM800 et les integrent au registre Modbus correspondant
  // Préleve les infos du Wifi et les stocke dans le registre d'éxecution
  static unsigned long watchDog = millis();
  if (millis() - watchDog > WATCHDOG_TIMER)
  {
    watchDog = millis();

    // == WIFI ==
    if (WiFi.status() == WL_CONNECTED)
    {
      modbus.setHoldingRegister(MODMAP_WIFI_SIGNAL_LEVEL, -WiFi.RSSI());
    }
    modbus.setHoldingRegister(MODMAP_WIFI_STATUS, WiFi.status());

    // == GSM ==
    modbus.setHoldingRegister(MODMAP_GPRS_SIGNAL_LEVEL, sim800.requestSignalQuality());
    modbus.setHoldingRegister(MODMAP_GPRS_ATTACH, sim800.requestNetworkRegistration());
  }

  static unsigned long liveWordTimer = millis();
  static unsigned long liveWordTimerOffset = millis();
  if (millis() - liveWordTimer > 1000)
  {
    liveWordTimer = millis();

    unsigned int liveWord = (liveWordTimer - liveWordTimerOffset)/1000;

    if (liveWord >= 65535)
      liveWordTimerOffset = millis();

    modbus.setHoldingRegister(MODMAP_LIVE_WORD, liveWord);
  }

  if (Serial.available())
  {
    delay(500);

    while (Serial.available()) // Vide le tampon du port serie
    {
      Serial.read();
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.print("[*] Network information for ");
      Serial.println(WIFI_SSID);

      Serial.println("[+] BSSID : " + WiFi.BSSIDstr());
      Serial.print("[+] Gateway IP : ");
      Serial.println(WiFi.gatewayIP());
      Serial.print("[+] Subnet Mask : ");
      Serial.println(WiFi.subnetMask());
      Serial.println((String) "[+] RSSI : " + WiFi.RSSI() + " dB");
      Serial.print("[+] ESP32 IP : ");
      Serial.println(WiFi.localIP());
    }
    else
      Serial.println("Wifi non connecté");

    modbus.printStats();
    modbus.printHoldingRegisterInfo();
  }

  // Routine du serveur Modbus
  modbus.run();

  // Routine du module SIM800
  sim800.run();
}