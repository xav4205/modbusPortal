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

AsyncWebServer server(80);

Sim800 sim800;

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

  Serial.print('[' + (String)__FILE__ + "::" + __func__ + "/" + __LINE__ + "] \t");
  Serial.println("===UART initialized===");

 

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
                       Serial.println("Start updating " + type);
                     });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
                       Serial.printf("Error[%u]: ", error);
                       if (error == OTA_AUTH_ERROR)
                         Serial.println("Auth Failed");
                       else if (error == OTA_BEGIN_ERROR)
                         Serial.println("Begin Failed");
                       else if (error == OTA_CONNECT_ERROR)
                         Serial.println("Connect Failed");
                       else if (error == OTA_RECEIVE_ERROR)
                         Serial.println("Receive Failed");
                       else if (error == OTA_END_ERROR)
                         Serial.println("End Failed");
                     });
  ArduinoOTA.begin();

  //=========== Initialialisation du serveur WebSerial (port serie distant) ==========

  WebSerial.begin(&server);
  WebSerial.msgCallback(recvMsg);
  server.begin();

  //=========== Initialialisation du module SIM800 ==========
  sim800.begin(SIM800_UART_BAUDRATE, SIM800_TX_PIN, SIM800_RX_PIN);
}

void loop()
{

  // Runtime du serveur OTA
  ArduinoOTA.handle();

  // Fonction cyclique
  static unsigned long watchDog = millis();
  if (millis() - watchDog > WATCHDOG_TIMER)
  {
    watchDog = millis();
    Serial.print("Alive !");
    Serial.println(millis());
  }


  // Runtime du module SIM800
  sim800.run();
}