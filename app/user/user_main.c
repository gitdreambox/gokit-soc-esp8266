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

#include "gagent.h"
#include "soc.h"

#include "gagent_typedef.h"

#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"

#include "user_devicefind.h"
#include "user_webserver.h"

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

extern pgcontext pgContextData;

extern soc_pcontext soc_context_Data; 

#define TaskQueueLen    200
LOCAL  os_event_t   TaskQueue[TaskQueueLen];

void  ICACHE_FLASH_ATTR user_rf_pre_init(void)
{
}

void ICACHE_FLASH_ATTR
dataHandle_task(os_event_t *events)
{
    uint32 start_addr;
    uint16 GAgentstatus = 0;
    uint8 vchar=(uint8)(events->par);
    switch(events->sig)
    {
        case SIG_UART:
            GAgent_Local_Handle( pgContextData, pgContextData->rtinfo.UartRxbuf, GAGENT_BUF_LEN );
            break;
        case SIG_TCP:
            GAgent_Lan_Handle( pgContextData, pgContextData->rtinfo.TcpRxbuf , GAGENT_BUF_LEN );
            break;
        case SIG_UDP:
            Lan_udpDataHandle(pgContextData, pgContextData->rtinfo.Rxbuf , GAGENT_BUF_LEN );
            break;
        case SIG_CLOUD_HTTP:
            Cloud_ConfigDataHandle( pgContextData );
            break;
        case SIG_CLOUD_M2M:
//            if( 0 == pgContextData->rtinfo.isOtaRunning ||
//                RET_SUCCESS == pgContextData->rtinfo.m2mDnsflag )
            {
                GAgent_Cloud_Handle( pgContextData, pgContextData->rtinfo.MqttRxbuf, GAGENT_BUF_LEN );
            }
            break;
        case SIG_WRITE_FW:
            GAgent_GetFwHeadInfo(pgContextData,&(pgContextData->rtinfo.firmwareInfo));
            if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1)
            {
                start_addr = 0x101000;
            }
            else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2)
            {
                start_addr = 0x1000;
            }
            GAgent_copyFirmware( pgContextData, 0x209000, start_addr,
                pgContextData->rtinfo.firmwareInfo.file_len);
            break;
        case SIG_WEBCONFIG:
            handleWebConfig( pgContextData );
            break;
        case SIG_BIGDATA:
            throwgarbage(pgContextData, &pgContextData->rtinfo.file);
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
    wifi_station_set_auto_connect(1);
    wifi_set_sleep_type(NONE_SLEEP_T);//set none sleep mode
    espconn_tcp_set_max_con(10);
    LOCAL os_timer_t test_timer;
    LOCAL os_timer_t test_timer_1; 
    uart_init_3(9600,74880);
    UART_SetPrintPort(1);
    os_printf("\r\n\n\n---------------SDK version:%s--------------\n", system_get_sdk_version());
    os_printf("\r\nsystem_get_free_heap_size=%d\n",system_get_free_heap_size());

    struct rst_info *rtc_info = system_get_rst_info();
    os_printf("reset reason: %x\n", rtc_info->reason);
    if (rtc_info->reason == REASON_WDT_RST ||
        rtc_info->reason == REASON_EXCEPTION_RST ||
        rtc_info->reason == REASON_SOFT_WDT_RST)
    {
        if (rtc_info->reason == REASON_EXCEPTION_RST)
        {
            os_printf("Fatal exception (%d):\n", rtc_info->exccause);
        }
        os_printf("epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",
                rtc_info->epc1, rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr, rtc_info->depc);
    }
    if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1)
    {
        os_printf("---UPGRADE_FW_BIN1---\n");
    }
    else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2)
    {
        os_printf("---UPGRADE_FW_BIN2---\n");
    }

    system_os_task(dataHandle_task, 1, TaskQueue, TaskQueueLen);
    GAgent_Init( &pgContextData );
    GAgent_dumpInfo( pgContextData );
       
    //start timer
    os_timer_disarm(&test_timer);
    os_timer_setfn(&test_timer, (os_timer_func_t *)GAgent_Tick, pgContextData);
    os_timer_arm(&test_timer, 1000, 1);
    
    
    /* USER LEVEL */
    HW_Init();
    
    SW_Init(&soc_context_Data); 

    //start soc timer
    os_timer_disarm(&test_timer_1);
    os_timer_setfn(&test_timer_1, (os_timer_func_t *)SOC_Tick, soc_context_Data); 
    os_timer_arm(&test_timer_1, SOC_TIME_OUT, 1); 
}

