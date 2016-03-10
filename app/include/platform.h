#ifndef __PLATFORM_H_
#define __PLATFORM_H_


//#define sockaddr_t sockaddr_in
#define _POSIX_C_SOURCE 200809L

//#include "hal_uart.h"
#include "c_types.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "ip_addr.h"
#include "smartconfig.h"
#include "upgrade.h"
#include "osapi.h"
#include "spi_flash.h"

#include "gagent_typedef.h"

#define WIFI_SOFTVAR    "04020008"
#define WIFI_HARDVER    "00ESP826"

#define UART_NAME       "/dev/ttyUSB0"
#define NET_ADAPTHER    "eth0"



extern void msleep();
#define HTONS(_n)  ((uint16_t)(((_n) & 0xff) << 8) | (((_n) >> 8) & 0xff))
#define HTONL(x) ((uint32)(                                  \
    (((uint32)(x) & (uint32)0x000000ffUL) << 24) |            \
    (((uint32)(x) & (uint32)0x0000ff00UL) <<  8) |            \
    (((uint32)(x) & (uint32)0x00ff0000UL) >>  8) |            \
    (((uint32)(x) & (uint32)0xff000000UL) >> 24)))

#endif

