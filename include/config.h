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



#endif