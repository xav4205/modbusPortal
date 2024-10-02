#ifndef CONFIG_H
#define CONFIG_H

//=======Pin config===========

#define OUTPUT_1_PIN 16
#define OUTPUT_2_PIN 26
#define INPUT_1_PIN 33
#define INPUT_2_PIN 32

/*=======Main config===========*/
#define WATCHDOG_TIMER 10000
#define WIFI_RESTART 10
#define MODBUS_LINK_TIMER 5000
#define WEB_SERIAL 1
#define MODBUS_READING_DEBUG 1
#define MODBUS_WRITING_DEBUG 1
#define RELAY_PULSE_TIME 1000
#define PORTAL_SENSOR_DEBOUNCE 4000
#define PORTAL_OPEN_TIME 50000

/*===== Wifi Settings ======*/
// NE RIEN MODIFIER ICI MAIS DANS platformio.ini

#define ST(A) #A
#define STR(A) ST(A)

#define WIFI_SSID STR(WIFI_NETWORK_SSID)
#define WIFI_PASSWORD STR(WIFI_NETWORK_PASSWORD)

#define ESP_PASSWORD STR(OTA_PASSWORD)

#define WIFI_LOCAL_IP                                      \
  {                                                        \
    IP_ADDRESS_1, IP_ADDRESS_2, IP_ADDRESS_3, IP_ADDRESS_4 \
  }

#define WIFI_SUBNET  \
  {                  \
    255, 255, 255, 0 \
  }
#define WIFI_DNS \
  {              \
    1, 1, 1, 1   \
  }
#define WIFI_GATEWAY                            \
  {                                             \
    IP_ADDRESS_1, IP_ADDRESS_2, IP_ADDRESS_3, 1 \
  }

/*===== Modbus Server Settings ======*/

#define MODBUS_SERVER_WATCHDOG 60000
#define MODBUS_HOLDING_REGISTER_SIZE 16

/*===== Modbus Register Map ======*/
/*Lecture*/
#define MODMAP_LIVE_WORD 1 // Mot de vie
#define MODMAP_WIFI_STATUS 2
#define MODMAP_WIFI_SIGNAL_LEVEL 3
#define MODMAP_MAGNET_SENSOR 4
#define MODMAP_OUTPUT_1 5 // Etat de la sortie 1
#define MODMAP_OUTPUT_2 6 // Etat de la sortie 2
#define MODMAP_INPUT_1 7  // Etat de l'entree 1
#define MODMAP_INPUT_2 8  // Etat de l'entr4ee 2
#define MODMAP_PORTAL_STATE 9
#define MODMAP_REG_10 10
/*Ecriture*/
#define MODMAP_PORTAL_REQUEST 11
#define MODMAP_LIVE_WORD_ECHO 12
#define MODMAP_REG_13 13
#define MODMAP_REG_14 14
#define MODMAP_REG_15 15
#define MODMAP_REG_16 16

#endif