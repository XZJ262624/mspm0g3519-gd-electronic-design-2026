#ifndef IPS200_CONFIG_H_
#define IPS200_CONFIG_H_

#include "ti_msp_dl_config.h"

#ifndef IPS200_SPI_INST
#define IPS200_SPI_INST        SPI0
#endif

#define IPS200_GPIO_PORT       GPIOA

#define IPS200_PIN_RST         DL_GPIO_PIN_7
#define IPS200_PIN_CS          DL_GPIO_PIN_8
#define IPS200_PIN_BLK         DL_GPIO_PIN_13
#define IPS200_PIN_DC          DL_GPIO_PIN_15

#define IPS200_CONTROL_PINS    (IPS200_PIN_RST | IPS200_PIN_CS | \
                                IPS200_PIN_BLK | IPS200_PIN_DC)

#define IPS200_BLK_ACTIVE_HIGH 1U

#endif