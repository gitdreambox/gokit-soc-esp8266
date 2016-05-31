#include "gizwits_protocol.h"
#include "gagent_external.h"

volatile uint8_t issuedBuf[256]; 
volatile uint8_t reportBuf[256]; 
volatile gizwits_report_t lastReportData;
volatile uint32_t issuedLen = 0; 
volatile uint32_t reportLen = 0; 
event_info_t issuedProcessEvent;
uint8_t gizConditionFlag = 0; 

extern void ICACHE_FLASH_ATTR gizEventProcess(event_info_t *info, uint8_t *data);

/*
 * htons unsigned short -> 网络字节序
 * htonl unsigned long  -> 网络字节序
 * ntohs 网络字节序 -> unsigned short
 * ntohl 网络字节序 -> unsigned long
 */
uint16_t exchangeBytes(uint16_t value)
{
    uint16_t    tmpValue;
    uint8_t     *index_1, *index_2;

    index_1 = (uint8_t *)&tmpValue;
    index_2 = (uint8_t *)&value;

    *index_1 = *(index_2+1);
    *(index_1+1) = *index_2;

    return tmpValue;
}

/**
* @brief Data Type Conversions x : 转化为协议中的x值及实际通讯传输的值
*
* @param ratio : 修正系数k
* @param addition : 增量m
* @param pre_value : 作为协议中的y值, 是App UI界面的显示值
*
* @return uint16_t aftValue : 作为协议中的x值, 是实际通讯传输的值
*/
uint32 ICACHE_FLASH_ATTR Y2X(uint32 ratio, int32 addition, uint32 pre_value)
{
    uint32 tempValue = 0;
    uint32 aftValue = 0;

    tempValue = (uint32)pre_value;

    //x=(y - m)/k
    aftValue = ((tempValue - addition) / ratio);

    return aftValue;
}

/**
* @brief Data Type Conversions y : 转化为协议中的y值及App UI界面的显示值
*
* @param ratio : 修正系数k
* @param addition : 增量m
* @param pre_value : 作为协议中的x值, 是实际通讯传输的值
*
* @return int16_t : 作为协议中的y值, 是App UI界面的显示值
*/
int32 ICACHE_FLASH_ATTR X2Y(uint32 ratio, int32 addition, uint32 pre_value)
{
    uint32 tempValue = 0;
    int32 aftValue = 0;

    tempValue = pre_value;

    //y=k * x + m
    aftValue = (tempValue * ratio + addition);

    return aftValue;
}

void ICACHE_FLASH_ATTR mSleep(void)
{
    int i;
    
    for(i = 0; i < 20; i++)
    {
        os_delay_us(50000);
    }
}

uint32 gizTimeMs(void)
{
    return (system_get_time() / 1000); 
}

uint32 gizTimeS(void)
{
    return (system_get_time() / 1000*1000); 
}

uint32 ICACHE_FLASH_ATTR gizGetIntervalsMs(uint32 lastRpMs)
{

    uint32 intervals_ms = 0;

    if(0 < (gizTimeMs() - lastRpMs))
    {
        intervals_ms = gizTimeMs() - lastRpMs;
    }
    else
    {
        intervals_ms = (0xffffffff - lastRpMs) + gizTimeMs();
    }

    return (intervals_ms);
}

uint8_t ICACHE_FLASH_ATTR gizSetRpflag(void)
{
    gizConditionFlag = 1;
}

uint8_t ICACHE_FLASH_ATTR gizCleanRpflag(void)
{
    gizConditionFlag = 0;
}

uint8_t ICACHE_FLASH_ATTR gizJudgeRpflagHandle(void)
{
    if(1 == gizConditionFlag) 
    {
        system_os_post(USER_TASK_PRIO_0, SIG_REPORT_JUDGMENT, 0); 
    }
}

uint8_t ICACHE_FLASH_ATTR gizImmediateReport(void)
{
    gizwits_report_t * reportData = (gizwits_report_t *)&reportBuf; 
    
    reportData->action = ACTION_REPORT_DEV_STATUS;

    gizReportData(ACTION_REPORT_DEV_STATUS, (uint8_t *)&reportData->dev_status, sizeof(dev_status_t)); 
    
    os_printf("$$$$$ report_Imme : [act:%x],[onf:%x],[Col:%x],[R:%d],[G:%d],[B:%d],[mo:%d],[inf:%d],[tem:%d]],[hum:%d] \n",
              reportData->action, reportData->dev_status.led_onoff, reportData->dev_status.led_color, reportData->dev_status.led_r, reportData->dev_status.led_g,
              reportData->dev_status.led_b, reportData->dev_status.motor, reportData->dev_status.infrared, reportData->dev_status.temperature, reportData->dev_status.humidity); 
    
    return (0);
}



uint8_t ICACHE_FLASH_ATTR gizRegularlyReportHandle(void)
{
    static uint32 repCtime = 0;

    //600s Regularly report
    if(TIM_REP_TIMOUT < repCtime) 
    {
        repCtime = 0;
        
        os_printf("@@@@ gokit_timing report! \n"); 
        
        system_os_post(USER_TASK_PRIO_0, SIG_IMM_REPORT, 0); 

        return (1);
    }
    
    repCtime++;
    
    return (0);
}

uint8_t ICACHE_FLASH_ATTR gizReportJudge(void)
{
    static uint32 lastReportMs = 0; 
    
    gizwits_report_t * reportData = (gizwits_report_t *)&reportBuf; 
    
    if(MIN_INTERVAL_TIME < gizGetIntervalsMs(lastReportMs))
    {
        gizReportData(ACTION_REPORT_DEV_STATUS, (uint8_t *)&reportData->dev_status, sizeof(reportData->dev_status)); 
        
        lastReportMs = gizTimeMs(); 
        
        system_os_post(USER_TASK_PRIO_0, SIG_CLE_REPFLAG, 0); 
        
        return (1); 
    }

    return (0);
}

void ICACHE_FLASH_ATTR gizWiFiStatus(uint16_t value)
{
    uint8_t rssiValue = 0;
    wifi_status_t status;
    static wifi_status_t lastStatus;

    if(NULL != value)
    {
        status.value = value;

        os_printf("@@@@ GAgentStatus[hex]:%02x | [Bin]:%d,%d,%d,%d,%d,%d \r\n", status.value, status.types.con_m2m, status.types.con_route, status.types.binding, status.types.onboarding, status.types.station, status.types.softap);

        //OnBoarding mode status
        if(lastStatus.types.onboarding != status.types.onboarding)
        {
            if(1 == status.types.onboarding)
            {
                if(1 == status.types.softap)
                {
                    issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_SOFTAP;
                    issuedProcessEvent.num++;
                    os_printf("OnBoarding: SoftAP or Web mode\r\n");
                }

                if(1 == status.types.station)
                {
                    issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_AIRLINK;
                    issuedProcessEvent.num++;
                    os_printf("OnBoarding: AirLink mode\r\n");
                }
            }
            else
            {
                if(1 == status.types.softap)
                {
                    issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_SOFTAP;
                    issuedProcessEvent.num++;
                    os_printf("OnBoarding: SoftAP or Web mode\r\n");
                }

                if(1 == status.types.station)
                {
                    issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_STATION;
                    issuedProcessEvent.num++;
                    os_printf("OnBoarding: Station mode\r\n");
                }
            }
        }

        //binding mode status
        if(lastStatus.types.binding != status.types.binding)
        {
            lastStatus.types.binding = status.types.binding;
            if(1 == status.types.binding)
            {
                issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_OPEN_BINDING;
                issuedProcessEvent.num++;
                os_printf("WiFi status: in binding mode\r\n");
            }
            else
            {
                issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_CLOSE_BINDING;
                issuedProcessEvent.num++;
                os_printf("WiFi status: out binding mode\r\n");
            }
        }

        //router status
        if(lastStatus.types.con_route != status.types.con_route)
        {
            lastStatus.types.con_route = status.types.con_route;
            if(1 == status.types.con_route)
            {
                issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_CON_ROUTER;
                issuedProcessEvent.num++;
                os_printf("WiFi status: connected router\r\n");
            }
            else
            {
                issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_DISCON_ROUTER;
                issuedProcessEvent.num++;
                os_printf("WiFi status: disconnected router\r\n");
            }
        }

        //M2M server status
        if(lastStatus.types.con_m2m != status.types.con_m2m)
        {
            lastStatus.types.con_m2m = status.types.con_m2m;
            if(1 == status.types.con_m2m)
            {
                issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_CON_M2M;
                issuedProcessEvent.num++;
                os_printf("WiFi status: connected m2m\r\n");
            }
            else
            {
                issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_DISCON_M2M;
                issuedProcessEvent.num++;
                os_printf("WiFi status: disconnected m2m\r\n");
            }
        }

        //APP status
        if(lastStatus.types.app != status.types.app)
        {
            lastStatus.types.app = status.types.app;
            if(1 == status.types.app)
            {
                issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_CON_APP;
                issuedProcessEvent.num++;
                os_printf("WiFi status: app connect\r\n");
            }
            else
            {
                issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_DISCON_APP;
                issuedProcessEvent.num++;
                os_printf("WiFi status: no app connect\r\n");
            }
        }

        //test mode status
        if(lastStatus.types.test != status.types.test)
        {
            lastStatus.types.test = status.types.test;
            if(1 == status.types.test)
            {
                issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_OPEN_TESTMODE;
                issuedProcessEvent.num++;
                os_printf("WiFi status: in test mode\r\n");
            }
            else
            {
                issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_CLOSE_TESTMODE;
                issuedProcessEvent.num++;
                os_printf("WiFi status: out test mode\r\n");
            }
        }

        rssiValue = status.types.rssi;
        issuedProcessEvent.event[issuedProcessEvent.num] = WIFI_RSSI;
        issuedProcessEvent.num++;

        gizEventProcess(&issuedProcessEvent, (uint8_t *)&rssiValue);
        os_memset((uint8_t *)&issuedProcessEvent, 0, sizeof(event_info_t));

        lastStatus = status; 
    }
}

void ICACHE_FLASH_ATTR reportDataDTC(dev_status_t * dst, dev_status_t * src)
{
    int16_t value = 0;

    if((NULL != src)&&(NULL != dst))
    {
        os_memcpy((uint8_t *)dst, (uint8_t *)src, sizeof(dev_status_t));
        //custom
        
        dst->led_r = Y2X(LED_R_RATIO, LED_R_ADDITION, src->led_r);
        dst->led_g = Y2X(LED_G_RATIO, LED_G_ADDITION, src->led_g);
        dst->led_b = Y2X(LED_B_RATIO, LED_B_ADDITION, src->led_b);

        value = Y2X(MOTOR_SPEED_RATIO, MOTOR_SPEED_ADDITION, src->motor);
        dst->motor = exchangeBytes(value);
        dst->humidity = Y2X(HUMIDITY_RATIO, HUMIDITY_ADDITION, src->humidity);
        dst->temperature = Y2X(TEMPERATURE_RATIO, TEMPERATURE_ADDITION, src->temperature);
    }
}

void ICACHE_FLASH_ATTR issuedDataDTC(gizwits_issued_t * dst, gizwits_issued_t * src)
{   
    int16_t value = 0;

    if((NULL != src)&&(NULL != dst))
    {
        os_memcpy((uint8_t *)dst, (uint8_t *)src, sizeof(gizwits_issued_t)); 
        
        //custom
        dst->attr_vals.led_r = X2Y(LED_R_RATIO, LED_R_ADDITION, src->attr_vals.led_r); 
        dst->attr_vals.led_g = X2Y(LED_G_RATIO, LED_G_ADDITION, src->attr_vals.led_g); 
        dst->attr_vals.led_b = X2Y(LED_B_RATIO, LED_B_ADDITION, src->attr_vals.led_b); 
        value = exchangeBytes(src->attr_vals.motor_speed); 
        dst->attr_vals.motor_speed = X2Y(MOTOR_SPEED_RATIO, MOTOR_SPEED_ADDITION, value); 
    }
}

void ICACHE_FLASH_ATTR dataPoint2Event(event_info_t * info, gizwits_issued_t * issuedData)
{
    os_printf("$$$$$ CONTROL_DEVICE[vals]-> [onoff:%x],[Color:%x],[LED_R:%d],[LED_G:%d],[SetLED_B:%d],[motor:%d] \n",
              issuedData->attr_vals.led_onoff, issuedData->attr_vals.led_color, issuedData->attr_vals.led_r, issuedData->attr_vals.led_g, issuedData->attr_vals.led_b, issuedData->attr_vals.motor_speed);

    if((NULL != info)&&(NULL != issuedData))
    {
        if(0x01 == issuedData->attr_flags.led_onoff)
        {
            info->event[info->num] = SetLED_OnOff;
            info->num++;
        }
        if(0x01 == issuedData->attr_flags.led_color)
        {
            info->event[info->num] = SetLED_Color;
            info->num++;
        }
        if(0x01 == issuedData->attr_flags.led_r)
        {
            info->event[info->num] = SetLED_R;
            info->num++;
        }
        if(0x01 == issuedData->attr_flags.led_g)
        {
            info->event[info->num] = SetLED_G;
            info->num++;
        }
        if(0x01 == issuedData->attr_flags.led_b)
        {
            info->event[info->num] = SetLED_B;
            info->num++;
        }
        if(0x01 == issuedData->attr_flags.motor)
        {
            info->event[info->num] = SetMotor;
            info->num++;
        }
    }
}

void ICACHE_FLASH_ATTR gizSetMode(uint8_t mode)
{
    gagentConfig(mode);
}

void ICACHE_FLASH_ATTR gizConfigReset(void)
{
    gagentReset();
}

int32_t ICACHE_FLASH_ATTR gizIssuedProcess(uint8_t *inData, uint32_t inLen,uint8_t *outData,int32_t *outLen)
{
    gizwits_issued_t *gizIssuedData= (gizwits_issued_t *)&inData[1];
    gizwits_report_t * reportData = (gizwits_report_t *)&reportBuf; 

    if((NULL == gizIssuedData) || (NULL == outData) || (NULL == outLen)) 
    {
        os_printf("!!! IssuedProcess Error \n"); 
        
        return (-1); 
    }

    switch(inData[0]) 
    {
        case ACTION_CONTROL_DEVICE:
            dataPoint2Event((event_info_t *)&issuedProcessEvent, gizIssuedData); 
            issuedDataDTC((gizwits_issued_t *)&issuedBuf, gizIssuedData); 
            
            system_os_post(USER_TASK_PRIO_0, SIG_ISSUED_DATA, 0); 

            *outLen = 0; 
            
            break;
            
        case ACTION_READ_DEV_STATUS:
            lastReportData.action = ACTION_READ_DEV_STATUS_ACK; 
            reportDataDTC((dev_status_t *)&lastReportData.dev_status, (dev_status_t *)&reportData->dev_status); 
            
            os_memcpy(outData, (uint8_t *)&lastReportData, sizeof(gizwits_report_t)); 
            *outLen = sizeof(gizwits_report_t);
            
            os_printf("$$$$$ report_STA : [act:%x],[onf:%x],[Col:%x],[R:%d],[G:%d],[B:%d],[mo:%d],[inf:%d],[tem:%d]],[hum:%d] \n", 
                      lastReportData.action, lastReportData.dev_status.led_onoff, lastReportData.dev_status.led_color, lastReportData.dev_status.led_r, lastReportData.dev_status.led_g,
                      lastReportData.dev_status.led_b, lastReportData.dev_status.motor, lastReportData.dev_status.infrared, lastReportData.dev_status.temperature, lastReportData.dev_status.humidity); 
 
            break;
            
        case ACTION_W2D_PASSTHROUGH: //透传
            os_memcpy(issuedBuf, &inData[1], inLen-1);
            issuedLen = inLen-1;
            
            system_os_post(USER_TASK_PRIO_0, SIG_PASSTHROUGH, 0); 
            
            *outLen = 0;

            break;
            
        default:
        
            break;
    }

    return 0;
}

int32_t ICACHE_FLASH_ATTR gizReportData(uint8_t action, uint8_t *data, uint32_t len)
{
    uint8_t sendData[256];

    if(NULL == data) 
    {
        os_printf("!!! gizReportData Error \n"); 

        return (-1);
    }
    
    sendData[0] = action;
    reportDataDTC((dev_status_t *)&sendData[1], (dev_status_t *)data); 

    gagentUploadData(sendData, len+1);

    return 0;
}

uint32_t ICACHE_FLASH_ATTR gizGetTimeStamp(void)
{
    _tm tmValue;

    gagentGetNTP(&tmValue);

    return tmValue.ntp;
}

void ICACHE_FLASH_ATTR gizwitsInit(void)
{
    gizwits_report_t * reportData = (gizwits_report_t *)&reportBuf; 
    
    os_memset(issuedBuf, 0, sizeof(issuedBuf)); 
    os_memset(reportBuf, 0, sizeof(reportBuf)); 
    issuedLen = 0;
    reportLen = 0;
        
    os_printf("gizwitsInit OK \r\n"); 
}



