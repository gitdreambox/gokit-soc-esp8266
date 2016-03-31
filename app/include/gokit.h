#ifndef _GOKIT_H_
#define _GOKIT_H_

#include "driver/key.h"
#include "gagent_typedef.h"

#define MOTOR_16

#ifdef MOTOR_16
typedef uint16_t MOTOR_T;
#else
typedef uint8_t MOTOR_T;
#endif

/*****************************************************
* gokit相关定义
******************************************************/
#define MCU_PROTOCOL_VER                        "00000001"
#define MCU_P0_VER                              "00000001"
#define MCU_HARDWARE_VER                        "03000001"
#define MCU_SOFTWARE_VER                        "03000200"
#define MCU_PRODUCT_KEY                         "4dd210ab2ce84ba58e72d80efa8947ab"

#define MOTOR_DEFAULT_VAL                       5
#define TEM_OFFSET_VAL                          13                      //Temperature offset value
#define GPIO_KEY_NUM                            2

/*****************************************************
* I/O相关定义
******************************************************/
#define KEY_0_IO_MUX                            PERIPHS_IO_MUX_GPIO0_U
#define KEY_0_IO_NUM                            0
#define KEY_0_IO_FUNC                           FUNC_GPIO0

#define KEY_1_IO_MUX                            PERIPHS_IO_MUX_MTMS_U
#define KEY_1_IO_NUM                            14
#define KEY_1_IO_FUNC                           FUNC_GPIO14

/*****************************************************
* 定时器相关状态
******************************************************/
#define SOC_TIME_OUT                            10
#define MAX_SOC_TIMOUT                          (1000 / SOC_TIME_OUT)   //1S
#define TIM_REP_TIMOUT                          (600000 / SOC_TIME_OUT) //600S Regularly report
#define EKY_PRLONG_TIMOUT                       (10 / SOC_TIME_OUT)     //10 MS De
#define TH_TIMEOUT                              (200 / SOC_TIME_OUT)    //100ms Temperature and humidity detection minimum time
#define TH_MEANS_TIMEOUT                        (1000 / SOC_TIME_OUT)   //Temperature and humidity to calculate the mean time
#define IF_TIMEOUT                              (2000 / SOC_TIME_OUT)

/*****************************************************
* WiFi模组工作状态
******************************************************/
#define WIFI_SOFTAPMODE                         (uint8_t)(1<<0)         //SOFTAP_MODE
#define WIFI_STATIONMODE                        (uint8_t)(1<<1)         //AIRLINK_MODE
#define WIFI_CONFIGMODE                         (uint8_t)(1<<2)
#define WIFI_BINDINGMODE                        (uint8_t)(1<<3)
#define WIFI_CONNROUTER                         (uint8_t)(1<<4)
#define WIFI_CONNCLOUDS                         (uint8_t)(1<<5)         //Connection Clouds

/*****************************************************
* P0 command 命令码
******************************************************/
typedef enum
{
    P0_W2D_CONTROL_DEVICE_ACTION                = 0x01,
    P0_W2D_READ_DEVICESTATUS_ACTION             = 0x02,
    P0_D2W_READ_DEVICESTATUS_ACTION_ACK         = 0x03,
    P0_D2W_REPORT_DEVICESTATUS_ACTION           = 0X04,
} P0_ActionTypeDef; 

/*****************************************************
* LED状态宏
******************************************************/
typedef enum
{
    LED_OnOff                                   = 0x00,
    LED_OnOn                                    = 0x01,
    LED_Costom                                  = 0x00,
    LED_Yellow                                  = 0x02,
    LED_Purple                                  = 0x04,
    LED_Pink                                    = 0x06,
} LED_ColorTypeDef; 

#pragma pack(1)
/*****************************************************
* P0 数据读取结构
******************************************************/
typedef struct 
{
    uint8_t action;
    uint8_t led_cmd;
    uint8_t led_r;
    uint8_t led_g;
    uint8_t led_b;
    MOTOR_T motor; 
    uint8_t infrared;
    uint8_t temperature;
    uint8_t humidity;
    uint8_t alert;
    uint8_t fault;
}read_info_t;

/*****************************************************
* P0 数据写入结构
******************************************************/
typedef struct 
{
    uint8_t action;
    uint8_t attr_flags;
    uint8_t led_cmd;
    uint8_t led_r;
    uint8_t led_g;
    uint8_t led_b;
    MOTOR_T motor; 
}write_info_t;
#pragma pack()

void gokit_hardware_init(void);
void gokit_timer_func(void);
void gokit_wifi_Status(pgcontext pgc);
void gokit_ctl_process(pgcontext pgc, ppacket rx_buf);

#endif /*_GOKIT_H_*/
