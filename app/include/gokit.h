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
* gokit��ض���
******************************************************/
#define MCU_PROTOCOL_VER                        "00000001"
#define MCU_P0_VER                              "00000002"
#define MCU_HARDWARE_VER                        "00000003"
#define MCU_SOFTWARE_VER                        "00000004"
#define MCU_PRODUCT_KEY                         "6f3074fe43894547a4f1314bd7e3ae0b"

#define GPIO_KEY_NUM                            2

/*****************************************************
* I/O��ض���
******************************************************/
#define KEY_0_IO_MUX                            PERIPHS_IO_MUX_GPIO0_U
#define KEY_0_IO_NUM                            0
#define KEY_0_IO_FUNC                           FUNC_GPIO0

#define KEY_1_IO_MUX                            PERIPHS_IO_MUX_MTMS_U
#define KEY_1_IO_NUM                            14
#define KEY_1_IO_FUNC                           FUNC_GPIO14


/*****************************************************
* ��ʱ�����״̬
******************************************************/
#define TEM_OFFSET_VAL                          13                      //Temperature offset value
#define SOC_TIME_OUT                            10
#define MAX_SOC_TIMOUT                          (1000 / SOC_TIME_OUT)   //1S
#define EKY_PRLONG_TIMOUT                       (10 / SOC_TIME_OUT)     //10 MS De
#define TH_TIMEOUT                              (200 / SOC_TIME_OUT)    //100ms Temperature and humidity detection minimum time
#define TH_MEANS_TIMEOUT                        (2000 / SOC_TIME_OUT)   //Temperature and humidity to calculate the mean time

/*****************************************************
* WiFiģ�鹤��״̬
******************************************************/
#define WIFI_SOFTAPMODE                         (uint8_t)(1<<0)         //SOFTAP_MODE
#define WIFI_STATIONMODE                        (uint8_t)(1<<1)         //AIRLINK_MODE
#define WIFI_CONFIGMODE                         (uint8_t)(1<<2)
#define WIFI_BINDINGMODE                        (uint8_t)(1<<3)
#define WIFI_CONNROUTER                         (uint8_t)(1<<4)
#define WIFI_CONNCLOUDS                         (uint8_t)(1<<5)         //Connection Clouds

/*****************************************************
* P0 command ������
******************************************************/
typedef enum
{
    P0_W2D_CONTROL_DEVICE_ACTION                = 0x01,
    P0_W2D_READ_DEVICESTATUS_ACTION             = 0x02,
    P0_D2W_READ_DEVICESTATUS_ACTION_ACK         = 0x03,
    P0_D2W_REPORT_DEVICESTATUS_ACTION           = 0X04,
} P0_ActionTypeDef; 

/*****************************************************
* gokit flag ������
******************************************************/
typedef enum
{
    flag_report                                 = 0x01, 
    flag_ledstatus                              = 0x02, 
    flag_config                                 = 0x03, 
} gokit_flag;

/*****************************************************
* LED״̬��
******************************************************/
typedef enum
{
    LED_OnOff = 0x00,
    LED_OnOn = 0x01,
    LED_Costom = 0x00,
    LED_Yellow = 0x02,
    LED_Purple = 0x04,
    LED_Pink = 0x06,
} LED_ColorTypeDef; 

#pragma pack(1)
/*****************************************************
* gokitȫ�����
******************************************************/
typedef struct {
    uint32 gokit_tick_count;
    uint16_t flag;
} gokit_info_t; 

/*****************************************************
* P0 ���ݶ�ȡ�ṹ
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
* P0 ����д��ṹ
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

void gokit_tick(void);
void gokit_hardware_init(void);
void gokit_timer_func(void);
void gokit_wifi_Status(pgcontext pgc);
void gokit_ctl_process(pgcontext pgc, ppacket rx_buf);

#endif /*_GOKIT_H_*/
