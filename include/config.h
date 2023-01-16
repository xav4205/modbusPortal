#ifndef CONFIG_H
#define CONFIG_H

//=======Pin config===========

#define OUTPUT_1_PIN 1
#define OUTPUT_2_PIN 2
#define INPUT_1_PIN 3
#define INPUT_2_PIN 4

/*=======Main config===========*/
#define WATCHDOG_TIMER 10000
#define MODBUS_LINK_TIMER 5000
#define WEB_SERIAL 1
#define MODBUS_DEBUG 0

/*===== Wifi Settings ======*/

#define HOME_NETWORK

#ifdef HOME_NETWORK

#define WIFI_SSID "Reseau Xavier"
#define WIFI_PASSWORD "xmzbr85310"

#define WIFI_LOCAL_IP \
  {                   \
    192, 168, 14, 223 \
  }
#define WIFI_SUBNET  \
  {                  \
    255, 255, 255, 0 \
  }
#define WIFI_DNS \
  {              \
    1, 1, 1, 1   \
  }
#define WIFI_GATEWAY \
  {                  \
    192, 168, 14, 1  \
  }

#else

#define WIFI_SSID "Wifi Metha"
#define WIFI_PASSWORD "biogaz85"

#define WIFI_LOCAL_IP \
  {                   \
    192, 168, 85, 223 \
  }
#define WIFI_SUBNET  \
  {                  \
    255, 255, 255, 0 \
  }
#define WIFI_DNS \
  {              \
    1, 1, 1, 1   \
  }
#define WIFI_GATEWAY \
  {                  \
    192, 168, 85, 1  \
  }

#endif

/*===== Modbus Server Settings ======*/

#define MODBUS_SERVER_WATCHDOG 60000
#define MODBUS_HOLDING_REGISTER_SIZE 8

/*===== Modbus Register Map ======*/

#define MODMAP_LIVE_WORD 1 // Mot de vie
#define MODMAP_OUTPUT_1 2  // Etat de la sortie 1
#define MODMAP_OUTPUT_2 3  // Etat de la sortie 2
#define MODMAP_INPUT_1 4   // Etat de l'entree 1
#define MODMAP_INPUT_2 5   // Etat de l'entree 2
#define MODMAP_WIFI_STATUS 6
#define MODMAP_WIFI_SIGNAL_LEVEL 7
#define MODMAP_LIVE_WORD_ECHO 8

#endif