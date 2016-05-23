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
#include "gizwits.h"

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

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    struct devAttrs attrs;
    LOCAL os_timer_t gokit_timer; 
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

    gizwitsInit();
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
    
    //gokit timer start
    os_timer_disarm(&gokit_timer);
    os_timer_setfn(&gokit_timer, (os_timer_func_t *)gizwitsTimerFunc, NULL); 
    os_timer_arm(&gokit_timer, SOC_TIME_OUT, 1); 
    
    os_printf("--- system_free_size = %d ---\n", system_get_free_heap_size()); 
}

