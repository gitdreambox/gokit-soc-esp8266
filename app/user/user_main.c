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

#define gizwitsTaskQueueLen    200
LOCAL  os_event_t   gizwitsTaskQueue[gizwitsTaskQueueLen];

LOCAL os_timer_t gizTimer; 
LOCAL key_typedef_t * singleKey[2];
LOCAL keys_typedef_t keys; 
uint8_t gizConfigFlag = 0; 
extern volatile uint8_t issuedBuf[256];
extern volatile uint8_t reportBuf[256]; 
extern volatile uint32_t issuedLen; 
extern volatile uint32_t reportLen; 
extern event_info_t issuedProcessEvent; 

LOCAL uint8_t ICACHE_FLASH_ATTR gizThReadHandle(void)
{
    static uint8_t preTemMeansVal = 0;
    static uint8_t pretemMeansVal = 0;
    uint8_t curTem = 0, curHum = 0;
    uint16_t temMeans = 0, hum_means = 0;
    uint8_t ret = 0;
    gizwits_report_t * reportData = (gizwits_report_t *)&reportBuf;

    ret = dh11_read(&curTem, &curHum);
    if(1 == ret)
    {
        os_printf("@@@@ dh11_read error ! \n");
        return 0;
    }

    //Being the first time and the initiative to report
    if((preTemMeansVal == 0) || (pretemMeansVal == 0))
    {
        preTemMeansVal = curTem;
        pretemMeansVal = curHum;

        //Assignment to report data
        reportData->dev_status.temperature = (uint8_t)Y2X(TEMPERATURE_RATIO, TEMPERATURE_ADDITION, (uint32)preTemMeansVal);
        reportData->dev_status.humidity = (uint8_t)Y2X(HUMIDITY_RATIO, HUMIDITY_ADDITION, (uint32)pretemMeansVal);

        system_os_post(USER_TASK_PRIO_0, SIG_SET_REPFLAG, 0);

        return (1);
    }

    //Before and after the two values, it is determined whether or not reported
    if((preTemMeansVal != curTem) || (pretemMeansVal != curHum))
    {
        preTemMeansVal = curTem;
        pretemMeansVal = curHum;

        //Assignment to report data
        reportData->dev_status.temperature = (uint8_t)Y2X(TEMPERATURE_RATIO, TEMPERATURE_ADDITION, (uint32)preTemMeansVal);
        reportData->dev_status.humidity = (uint8_t)Y2X(HUMIDITY_RATIO, HUMIDITY_ADDITION, (uint32)pretemMeansVal);

        system_os_post(USER_TASK_PRIO_0, SIG_SET_REPFLAG, 0);

        return (1);
    }

    return (0);
}

LOCAL uint8_t ICACHE_FLASH_ATTR gizThHandle(void)
{
    static uint16_t thCtime = 0;

    if(TH_TIMEOUT < thCtime)
    {
        thCtime = 0;

        system_os_post(USER_TASK_PRIO_0, SIG_HD11_READ, 0);

        return (1);
    }

    thCtime++;

    return (0);
}

LOCAL uint8_t ICACHE_FLASH_ATTR gizIrReadHandle(void)
{
    uint8_t irValue = 0;
    gizwits_report_t * reportData = (gizwits_report_t *)&reportBuf;

    irValue = ir_update_status();

    if(irValue != reportData->dev_status.infrared)
    {
        reportData->dev_status.infrared = irValue;

        gizImmediateReport();

        return (1);
    }

    return (0);
}

LOCAL uint8_t ICACHE_FLASH_ATTR gokitIrHandle(void)
{
    static uint32 irCtime = 0;

    if(INF_TIMEOUT < irCtime)
    {
        irCtime = 0;

        system_os_post(USER_TASK_PRIO_0, SIG_INFRARED_READ, 0);

        return (1);
    }

    irCtime++;

    return (0);
}

LOCAL void ICACHE_FLASH_ATTR key2ShortPress(void)
{
    os_printf("#### key2, soft ap mode \n");

    rgb_control(250, 0, 0);
    gizConfigFlag = 1;

    gizSetMode(1);
}

LOCAL void ICACHE_FLASH_ATTR key2LongPress(void)
{
    os_printf("#### key2, airlink mode\n");

    rgb_control(0, 250, 0);
    gizConfigFlag = 1;

    gizSetMode(2);
}

LOCAL void ICACHE_FLASH_ATTR key1ShortPress(void)
{
    os_printf("#### key1, press \n");

    rgb_control(0, 0, 250);
}

LOCAL void ICACHE_FLASH_ATTR key1LongPress(void)
{
    os_printf("#### key1, default setup\n");
    mSleep();
    gizConfigReset();
}

void ICACHE_FLASH_ATTR gizwitsTimerFunc(void)
{
    //Temperature and humidity sensors reported conditional
    gizThHandle();

    //Infrared sensors reported conditional
    gokitIrHandle();

    //Regularly report conditional
    gizRegularlyReportHandle();

    //Qualifying the initiative to report
    gizJudgeRpflagHandle();
}

void ICACHE_FLASH_ATTR gizwitsTask(os_event_t * events)
{
    uint8_t i = 0;
    uint8 vchar = 0;

    if(NULL == events)
    {
        os_printf("!!! gizwitsTask Error \n");
    }

    vchar = (uint8)(events->par);

    switch(events->sig)
    {
    case SIG_ISSUED_DATA:
        gizEventProcess(&issuedProcessEvent, (uint8_t *)issuedBuf);

        //clean event info
        os_memset((uint8_t *)&issuedProcessEvent, 0, sizeof(event_info_t));
        break;
    case SIG_PASSTHROUGH:
        for(i = 0; i < issuedLen; i++)
        {
            os_printf("%x ", issuedBuf[i]);
        }
        os_printf("\n");
        os_memset(issuedBuf, 0, 256);
        issuedLen = 0;
        break;
    case SIG_HD11_READ:
        gizThReadHandle();
        break;
    case SIG_INFRARED_READ:
        gizIrReadHandle();
        break;
    case SIG_REPORT_JUDGMENT:
        gizReportJudge();
        break;
    case SIG_SET_REPFLAG:
        gizSetRpflag();
        break;
    case SIG_CLE_REPFLAG:
        gizCleanRpflag();
        break;
    case SIG_IMM_REPORT:
        gizImmediateReport();
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
    rgb_gpio_init();
    rgb_led_init();

    //key init
    singleKey[0] = key_init_one(KEY_0_IO_NUM, KEY_0_IO_MUX, KEY_0_IO_FUNC,
                                key1LongPress, key1ShortPress);
    singleKey[1] = key_init_one(KEY_1_IO_NUM, KEY_1_IO_MUX, KEY_1_IO_FUNC,
                                key2LongPress, key2ShortPress);
    keys.key_num = GPIO_KEY_NUM;
    keys.key_timer_ms = 10;
    keys.singleKey = singleKey;
    key_para_init(&keys);

    //motor init
    motor_init();
    motor_control(MOTOR_SPEED_DEFAULT);
    
    //temperature and humidity init
    dh11_init();

    //Infrared init
    ir_init(); 

    //gizwits Init
    gizwitsInit(); 
    
    //gokit timer start
    os_timer_disarm(&gizTimer);
    os_timer_setfn(&gizTimer, (os_timer_func_t *)gizwitsTimerFunc, NULL);
    os_timer_arm(&gizTimer, SOC_TIME_OUT, 1); 
    
    system_os_task(gizwitsTask, 0, gizwitsTaskQueue, gizwitsTaskQueueLen);

    system_os_task(gagentProcessRun, 1, TaskQueue, TaskQueueLen);
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
    gagentInit(attrs);
    
    os_printf("--- system_free_size = %d ---\n", system_get_free_heap_size()); 
}

