#ifndef CONFIG_H
#define CONFIG_H

/*=======Pin config===========*/
#define SIM800_RX_PIN 25
#define SIM800_TX_PIN 26
#define SIM800_UART_BAUDRATE 19200

/*=======Main config===========*/
#define WEBSERIAL 0

/*===== Wifi Settings ======*/

#define WIFI_SSID "Wifi Metha"
#define WIFI_PASSWORD "biogaz85"
#define WIFI_LOCAL_IP \
  {                   \
    192, 168, 85, 222 \
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

/*===== Modbus Register Map ======*/

#define MODMAP_NEW_MESSAGE 1
#define MODMAP_NB_MESSAGE_QUEUE 2
#define MODMAP_IN_PROGRESS 3
#define MODMAP_WIFI_STATUS 4
#define MODMAP_WIFI_SIGNAL_LEVEL 5
#define MODMAP_GPRS_ATTACH 6
#define MODMAP_GPRS_SIGNAL_LEVEL 7
#define MODMAP_FIRST_PHONE_NUMBER_REGISTER 65
#define MODMAP_PHONE_NUMBER_SIZE_MESSAGE 10
#define MODMAP_FIRST_MESSAGE_REGISTER 70
#define MODMAP_MAX_SIZE_MESSAGE 118

/*===== Sim Module Settings ======*/
#define DEBUG_AT 1
#define DEBUG_RESPONSE 0
#define T_RESP 5000
#define TIME_OUT 1000
#define AT_TIME_OUT 5000
#define AT_DELAY 100
#define SIM_PIN_CODE "0000"
#define NB_OF_TRY_START 10
#define GPRS_WATCHDOG 1000000
#define MAX_NB_REPLY 10

#endif