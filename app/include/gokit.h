#ifndef _GOKIT_H_
#define _GOKIT_H_

#include "driver/key.h"

#define MCU_PROTOCOL_VER "00000001"
#define MCU_P0_VER "00000002"
#define MCU_HARDWARE_VER "00000003"
#define MCU_SOFTWARE_VER "00000004"
#define MCU_PRODUCT_KEY "6f3074fe43894547a4f1314bd7e3ae0b"

#define KEY_NUM            2

#define KEY_1_IO_MUX     PERIPHS_IO_MUX_MTCK_U
#define KEY_1_IO_NUM     4
#define KEY_1_IO_FUNC    FUNC_GPIO4

#define KEY_2_IO_MUX     PERIPHS_IO_MUX_MTCK_U
#define KEY_2_IO_NUM     14
#define KEY_2_IO_FUNC    FUNC_GPIO14


#pragma pack(1)
typedef struct {
    uint8_t  action;
    uint8_t  led_cmd;
    uint8_t  led_r;
    uint8_t  led_g;
    uint8_t  led_b;
    uint16_t motor;
    uint8_t  infrared;
    uint8_t  temperature;
    uint8_t  humidity;
    uint8_t  alert;
    uint8_t  fault;
}read_info_t;

typedef struct {
    uint8_t  action;
    uint8_t  attr_flags;
    uint8_t  led_cmd;
    uint8_t  led_r;
    uint8_t  led_g;
    uint8_t  led_b;
    uint16_t motor;
}write_info_t;
#pragma pack()

void gokit_tick(void);
void gokit_hardware_init(void);
void gokit_ctl_process(pgcontext pgc, ppacket rx_buf);

#endif /*_GOKIT_H_*/
