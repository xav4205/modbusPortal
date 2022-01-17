#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <AsyncTCP.h>

#include <ESPAsyncWebServer.h>
#include <WebSerial.h>

#include "config.h" /*

  Software serial multple serial test

 Receives from the hardware serial, sends to software serial.
 Receives from software serial, sends to hardware serial.

 The circuit:
 * RX is digital pin 10 (connect to TX of other device)
 * TX is digital pin 11 (connect to RX of other device)

 Note:
 Not all pins on the Mega and Mega 2560 support change interrupts,
 so only the following can be used for RX:
 10, 11, 12, 13, 50, 51, 52, 53, 62, 63, 64, 65, 66, 67, 68, 69

 Not all pins on the Leonardo and Micro support change interrupts,
 so only the following can be used for RX:
 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI).

 created back in the mists of time
 modified 25 May 2012
 by Tom Igoe
 based on Mikal Hart's example

 This example code is in the public domain.

 */

AsyncWebServer server(80);

// Surveille les entrée du WebSerial
void recvMsg(uint8_t *data, size_t len)
{
  WebSerial.println("Send AT command...");
//  char t[] = { (char)72};
 // WebSerial.print(t);
 // WebSerial.print(String ((char)68));
 /* WebSerial.print((char 76);
  WebSerial.print((char*)76);
  WebSerial.println((char*)79);
*/
  bool sms = false;
  String d = "";
  for (int i = 0; i < len; i++)
  {
    if (char(data[i]) == '&')
    {
      WebSerial.println("Send SMS");
      sms = true;
      break;
    }

    d += char(data[i]);
  }

  if (sms)
  {
    Serial1.print(d);
    Serial1.write((char)26);
    Serial1.println();
  }
  else
    Serial1.println(d);
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

  //===Init sim800==
  // Open serial communications and wait for port to open:
  Serial1.begin(19200, SERIAL_8N1, 26, 25);

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
                       Serial.println("Start updating " + type); });
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
                         Serial.println("End Failed"); });
  ArduinoOTA.begin();

  //=========== Initialialisation du serveur WebSerial (port serie distant) ==========

  WebSerial.begin(&server);
  WebSerial.msgCallback(recvMsg);
  server.begin();
}

void loop()
{

  // Runtime du serveur OTA
  ArduinoOTA.handle();

  // Surveillance des entrée series

  if (Serial1.available())
  {
    char c = Serial1.read();
    Serial.write(c);
    WebSerial.print(String ((char)c));
  }

  if (Serial.available())
  {
    Serial1.write(Serial.read());
  }
}