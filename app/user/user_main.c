/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"

#include "user_devicefind.h"
#include "user_webserver.h"
#include "gagent_external.h"
#include "gizwits_product.h"
#include "driver/hal_key.h"
#include "driver/hal_infrared.h"
#include "driver/hal_motor.h"
#include "driver/hal_rgb_led.h"
#include "driver/hal_temp_hum.h"

#if ESP_PLATFORM
#include "user_esp_platform.h"
#endif

#ifdef SERVER_SSL_ENABLE
#include "ssl/cert.h"
#include "ssl/private_key.h"
#else
#ifdef CLIENT_SSL_ENABLE
unsigned char *default_certificate;
unsigned int default_certificate_len = 0;
unsigned char *default_private_key;
unsigned int default_private_key_len = 0;
#endif
#endif

#define TaskQueueLen    200
LOCAL  os_event_t   TaskQueue[TaskQueueLen];

#define userQueueLen    200
LOCAL os_event_t userTaskQueue[userQueueLen]; 

#define USER_TIME_MS 100
LOCAL os_timer_t userTimer; 
#define TH_TIMEOUT                              (1000 / USER_TIME_MS)       //Temperature and humidity detection minimum time
#define INF_TIMEOUT                             (500 / USER_TIME_MS)    

/*****************************************************
 * * I/O相关定义
 * ******************************************************/
#define GPIO_KEY_NUM                            2
#define KEY_0_IO_MUX                            PERIPHS_IO_MUX_GPIO0_U
#define KEY_0_IO_NUM                            0
#define KEY_0_IO_FUNC                           FUNC_GPIO0

#define KEY_1_IO_MUX                            PERIPHS_IO_MUX_MTMS_U
#define KEY_1_IO_NUM                            14
#define KEY_1_IO_FUNC                           FUNC_GPIO14

LOCAL key_typedef_t * singleKey[GPIO_KEY_NUM];
LOCAL keys_typedef_t keys;

LOCAL uint8_t ICACHE_FLASH_ATTR gizThHandle(uint8_t * curtem, uint8_t * curhumi)
{
    static uint16_t thCtime = 0;
    uint8_t curTem = 0, curHum = 0;
    uint8_t ret = 0;
    
    if(TH_TIMEOUT < thCtime)
    {
        thCtime = 0;

        ret = dh11Read(&curTem, &curHum);
        
        if(1 == ret)
        {
            os_printf("@@@@ dh11Read error ! \n");
            return 0;
        }

        //Assignment to report data
        *curtem = curTem; 
        *curhumi = curHum; 

        return (1);
    }

    thCtime++;

    return (0);
}

LOCAL uint8_t ICACHE_FLASH_ATTR gokitIrHandle(uint8_t * curir)
{
    static uint32 irCtime = 0;
    uint8_t irValue = 0;
    
    if(INF_TIMEOUT < irCtime)
    {
        irCtime = 0;

        irValue = irUpdateStatus();

        *curir = irValue; 

        return (1); 
    }

    irCtime++;

    return (0);
}

LOCAL void ICACHE_FLASH_ATTR user_handle(void)
{
    uint8_t ret = 0; 
    uint8_t curTemperature = 0; 
    uint8_t curHumidity = 0; 
    uint8_t curir = 0; 
    
    //Temperature and humidity sensors reported conditional
    ret = gizThHandle(&curTemperature, &curHumidity);
    if(1 == ret)
    {
        reportData.dev_status.temperature = Y2X(TEMPERATURE_RATIO, TEMPERATURE_ADDITION, curTemperature);
        reportData.dev_status.humidity = Y2X(HUMIDITY_RATIO, HUMIDITY_ADDITION, curHumidity); 
    }
    
    //Infrared sensors reported conditional
    ret = gokitIrHandle(&curir); 
    if(1 == ret)
    {
        reportData.dev_status.infrared = curir; 
    }
    
    gizReportData(ACTION_REPORT_DEV_STATUS, (uint8_t *)&reportData, sizeof(gizwits_report_t)); 
}

LOCAL void ICACHE_FLASH_ATTR key2ShortPress(void)
{
    os_printf("#### key2, soft ap mode \n");

    rgbControl(250, 0, 0);

    gizSetMode(1);
}

LOCAL void ICACHE_FLASH_ATTR key2LongPress(void)
{
    os_printf("#### key2, airlink mode\n");

    rgbControl(0, 250, 0);

    gizSetMode(2);
}

LOCAL void ICACHE_FLASH_ATTR key1ShortPress(void)
{
    os_printf("#### key1, press \n");

    rgbControl(0, 0, 250);
}

LOCAL void ICACHE_FLASH_ATTR key1LongPress(void)
{
    os_printf("#### key1, default setup\n");
    mSleep();
    gizConfigReset();
}

void ICACHE_FLASH_ATTR userTimerFunc(void)
{
    user_handle();
}

void ICACHE_FLASH_ATTR gizwitsUserTask(os_event_t * events)
{
    uint8_t i = 0;
    uint8 vchar = 0;

    if(NULL == events)
    {
        os_printf("!!! gizwitsUserTask Error \n"); 
    }

    vchar = (uint8)(events->par);

    switch(events->sig)
    {
//  case SIG_INFRARED_READ:
//      gizIrReadHandle();
        break;
    default:
        os_printf("---error sig! ---\n");
        break;
    }
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    struct devAttrs attrs;
    uint32 system_free_size = 0;

    wifi_station_set_auto_connect(1);
    wifi_set_sleep_type(NONE_SLEEP_T);//set none sleep mode
    espconn_tcp_set_max_con(10);
    uart_init_3(9600,115200);
    UART_SetPrintPort(1);
    os_printf( "---------------SDK version:%s--------------\n", system_get_sdk_version());
    os_printf( "system_get_free_heap_size=%d\n",system_get_free_heap_size());

    struct rst_info *rtc_info = system_get_rst_info();
    os_printf( "reset reason: %x\n", rtc_info->reason);
    if (rtc_info->reason == REASON_WDT_RST ||
        rtc_info->reason == REASON_EXCEPTION_RST ||
        rtc_info->reason == REASON_SOFT_WDT_RST)
    {
        if (rtc_info->reason == REASON_EXCEPTION_RST)
        {
            os_printf("Fatal exception (%d):\n", rtc_info->exccause);
        }
        os_printf( "epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",
                rtc_info->epc1, rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr, rtc_info->depc);
    }

    if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1)
    {
        os_printf( "---UPGRADE_FW_BIN1---\n");
    }
    else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2)
    {
        os_printf( "---UPGRADE_FW_BIN2---\n");
    }
    
    //user init
    
    //rgb led init
    rgbGpioInit();
    rgbLedInit();

    //key init
    singleKey[0] = keyInitOne(KEY_0_IO_NUM, KEY_0_IO_MUX, KEY_0_IO_FUNC,
                                key1LongPress, key1ShortPress);
    singleKey[1] = keyInitOne(KEY_1_IO_NUM, KEY_1_IO_MUX, KEY_1_IO_FUNC,
                                key2LongPress, key2ShortPress);
    keys.key_num = GPIO_KEY_NUM;
    keys.key_timer_ms = 10;
    keys.singleKey = singleKey;
    keyParaInit(&keys);

    //motor init
    motorInit();
    motorControl(MOTOR_SPEED_DEFAULT);
    
    //temperature and humidity init
    dh11Init();

    //Infrared init
    irInit(); 

    //gizwits Init
    gizwitsInit();

    system_os_task(gagentProcessRun, USER_TASK_PRIO_1, TaskQueue, TaskQueueLen);

    attrs.mBindEnableTime = 0;
    attrs.mDevAttr[0] = 0x00;
    attrs.mDevAttr[1] = 0x00;
    attrs.mDevAttr[2] = 0x00;
    attrs.mDevAttr[3] = 0x00;
    attrs.mDevAttr[4] = 0x00;
    attrs.mDevAttr[5] = 0x00;
    attrs.mDevAttr[6] = 0x00;
    attrs.mDevAttr[7] = 0x00;
    os_memcpy(attrs.mstrDevHV, HARDWARE_VERSION, os_strlen(HARDWARE_VERSION));
    os_memcpy(attrs.mstrDevSV, SOFTWARE_VERSION, os_strlen(SOFTWARE_VERSION));
    os_memcpy(attrs.mstrP0Ver, P0_VERSION, os_strlen(P0_VERSION));
    os_memcpy(attrs.mstrProductKey, PRODUCT_KEY, os_strlen(PRODUCT_KEY));
    os_memcpy(attrs.mstrProtocolVer, PROTOCOL_VERSION, os_strlen(PROTOCOL_VERSION));
    os_memcpy(attrs.mstrSdkVerLow, SDK_VERSION, os_strlen(SDK_VERSION));
    gagentInit(attrs);

    system_os_task(gizwitsUserTask, USER_TASK_PRIO_0, userTaskQueue, userQueueLen);

    //gokit timer start
    os_timer_disarm(&userTimer);
    os_timer_setfn(&userTimer, (os_timer_func_t *)userTimerFunc, NULL);
    os_timer_arm(&userTimer, USER_TIME_MS, 1);

    os_printf("--- system_free_size = %d ---\n", system_get_free_heap_size());
}

