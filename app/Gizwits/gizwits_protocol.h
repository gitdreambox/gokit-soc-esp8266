#ifndef _GIZWITS_PROTOCOL_H
#define _GIZWITS_PROTOCOL_H
#include <stdint.h>
#include "osapi.h"
#include "user_interface.h"
#include "user_webserver.h"

/*****************************************************
* gokit相关定义
******************************************************/
#define PROTOCOL_VERSION                        "00000001"
#define P0_VERSION                              "00000001"
#define PRODUCT_KEY                             "4dd210ab2ce84ba58e72d80efa8947ab"

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
#define MIN_INTERVAL_TIME                       2000                    //The minimum interval for sending
#define MAX_SOC_TIMOUT                          (1000 / SOC_TIME_OUT)   //1s
#define TIM_REP_TIMOUT                          (600000 / SOC_TIME_OUT) //600s Regularly report
#define EKY_PRLONG_TIMOUT                       (10 / SOC_TIME_OUT)     //10 mS De
#define TH_TIMEOUT                              (1000 / SOC_TIME_OUT)   //Temperature and humidity detection minimum time
#define INF_TIMEOUT                             (500 / SOC_TIME_OUT)    

/*****************************************************
* task相关定义
******************************************************/
#define SIG_ISSUED_DATA                         0x01
#define SIG_PASSTHROUGH                         0x02
#define SIG_HD11_READ                           0x03
#define SIG_INFRARED_READ                       0x04
#define SIG_REPORT_TMIMING                      0x05
#define SIG_REPORT_JUDGMENT                     0x06
#define SIG_SET_REPFLAG                         0x07
#define SIG_CLE_REPFLAG                         0x08
#define SIG_IMM_REPORT                          0x09

/*****************************************************
* 数据点相关定义
******************************************************/
#define GPIO_KEY_NUM                            2

#define LED_R_RATIO                             1
#define LED_R_ADDITION                          0
#define LED_R_DEFAULT                           0

#define LED_G_RATIO                             1
#define LED_G_ADDITION                          0
#define LED_G_DEFAULT                           0

#define LED_B_RATIO                             1
#define LED_B_ADDITION                          0
#define LED_B_DEFAULT                           0

#define MOTOR_SPEED_RATIO                       1
#define MOTOR_SPEED_ADDITION                    (-5)
#define MOTOR_SPEED_DEFAULT                     0

#define TEMPERATURE_RATIO                       1
#define TEMPERATURE_ADDITION                    (-13)
#define TEMPERATURE_DEFAULT                     0

#define HUMIDITY_RATIO                          1
#define HUMIDITY_ADDITION                       0
#define HUMIDITY_DEFAULT                        0

#pragma pack(1)

//custom
typedef struct 
{
    struct 
    {
        uint8_t led_onoff:1;
        uint8_t led_color:1;
        uint8_t led_r:1;
        uint8_t led_g:1;
        uint8_t led_b:1;
        uint8_t motor:1;
    }attr_flags;
    struct 
    {
        uint8_t led_onoff:1;
        uint8_t led_color:2;
        uint8_t reserve:5;
        uint8_t led_r;
        uint8_t led_g;
        uint8_t led_b;
        uint16_t motor_speed; 
    }attr_vals;
}gizwits_issued_t;

//custom
typedef struct
{
    uint8_t led_onoff:1;
    uint8_t led_color:2;
    uint8_t reserve:5;
    uint8_t led_r;
    uint8_t led_g;
    uint8_t led_b;
    int16_t motor;
    uint8_t infrared;
    int8_t temperature;
    uint8_t humidity;
    uint8_t alert;
    uint8_t fault;
}dev_status_t; 

typedef struct 
{
    uint8_t action;
    dev_status_t dev_status;
}gizwits_report_t;

/*****************************************************
* WiFi模组工作状态
******************************************************/
typedef union
{
    uint16_t value;
    struct
    {
        uint16_t softap:1;
        uint16_t station:1;
        uint16_t onboarding:1;
        uint16_t binding:1;
        uint16_t con_route:1;
        uint16_t con_m2m:1;
        uint16_t reserve1:2;
        uint16_t rssi:3;
        uint16_t app:1;
        uint16_t test:1;
        uint16_t reserve2:3;
    }types;
} wifi_status_t; 

#pragma pack()

//custom
typedef enum
{
    WIFI_SOFTAP                                 = 0x00,
    WIFI_AIRLINK,
    WIFI_STATION,
    WIFI_OPEN_BINDING,
    WIFI_CLOSE_BINDING,
    WIFI_CON_ROUTER,
    WIFI_DISCON_ROUTER,
    WIFI_CON_M2M,
    WIFI_DISCON_M2M,
    WIFI_OPEN_TESTMODE,
    WIFI_CLOSE_TESTMODE,
    WIFI_CON_APP,
    WIFI_DISCON_APP,
    WIFI_RSSI,
    TRANSPARENT_DATA,
    //custom
    SetLED_OnOff,
    SetLED_Color,
    SetLED_R,
    SetLED_G,
    SetLED_B,
    SetMotor,
    EVENT_TYPE_MAX
} EVENT_TYPE_T;

/*****************************************************
* LED状态宏
******************************************************/
typedef enum
{
    LED_Off                                     = 0x00,
    LED_On                                      = 0x01,
    LED_Costom                                  = 0x00,
    LED_Yellow                                  = 0x01,
    LED_Purple                                  = 0x02,
    LED_Pink                                    = 0x03,
} LED_ColorTypeDef; 

typedef enum
{
    ACTION_CONTROL_DEVICE                       = 0x01,
    ACTION_READ_DEV_STATUS                      = 0x02,
    ACTION_READ_DEV_STATUS_ACK                  = 0x03,
    ACTION_REPORT_DEV_STATUS                    = 0X04,
    ACTION_W2D_PASSTHROUGH                      = 0x05,
    ACTION_D2W_PASSTHROUGH                      = 0x06,
} action_type_t;

//custom
typedef struct {
    uint8_t num;
    uint8_t event[EVENT_TYPE_MAX];
}event_info_t;

uint16_t exchangeBytes(uint16_t value);
uint32 Y2X(uint32 ratio, int32 addition, uint32 pre_value);
int32 X2Y(uint32 ratio, int32 addition, uint32 pre_value);
uint8_t gizRegularlyReportHandle(void);
uint8_t gizReportJudge(void);
void gizSetMode(uint8_t mode);
void gizConfigReset(void);
int32_t gizIssuedProcess(uint8_t * inData, uint32_t inLen, uint8_t * outData, int32_t * outLen); 
int32_t gizReportData(uint8_t action, uint8_t *data, uint32_t len);
uint32_t gizGetTimeStamp(void);
void gizwitsInit(void);

#endif
