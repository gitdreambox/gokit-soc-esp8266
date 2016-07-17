#include "gizwits_protocol.h"
#include "gagent_external.h"
#include "mem.h"

gizwits_protocol_t gizwitsProtocol;

#define gizwitsTaskQueueLen    200
LOCAL os_event_t gizwitsTaskQueue[gizwitsTaskQueueLen]; 
LOCAL os_timer_t gizTimer; 

extern void ICACHE_FLASH_ATTR gizEventProcess(event_info_t *info, uint8_t *data, uint32_t len);
extern gizwits_report_t reportData; 

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

uint32 ICACHE_FLASH_ATTR gizTimeMs(void)
{
    return (system_get_time() / 1000); 
}

uint32 ICACHE_FLASH_ATTR gizTimeS(void)
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
        if(1 == status.types.onboarding)
        {
            if(1 == status.types.softap)
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_SOFTAP;
                gizwitsProtocol.processEvent.num++;
                os_printf("OnBoarding: SoftAP or Web mode\r\n");
            }

            if(1 == status.types.station)
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_AIRLINK;
                gizwitsProtocol.processEvent.num++;
                os_printf("OnBoarding: AirLink mode\r\n");
            }
        }
        else
        {
            if(1 == status.types.softap)
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_SOFTAP;
                gizwitsProtocol.processEvent.num++;
                os_printf("OnBoarding: SoftAP or Web mode\r\n");
            }

            if(1 == status.types.station)
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_STATION;
                gizwitsProtocol.processEvent.num++;
                os_printf("OnBoarding: Station mode\r\n");
            }
        }

        //binding mode status
        if(lastStatus.types.binding != status.types.binding)
        {
            lastStatus.types.binding = status.types.binding;
            if(1 == status.types.binding)
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_OPEN_BINDING;
                gizwitsProtocol.processEvent.num++;
                os_printf("WiFi status: in binding mode\r\n");
            }
            else
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_CLOSE_BINDING;
                gizwitsProtocol.processEvent.num++;
                os_printf("WiFi status: out binding mode\r\n");
            }
        }

        //router status
        if(lastStatus.types.con_route != status.types.con_route)
        {
            lastStatus.types.con_route = status.types.con_route;
            if(1 == status.types.con_route)
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_CON_ROUTER;
                gizwitsProtocol.processEvent.num++;
                os_printf("WiFi status: connected router\r\n");
            }
            else
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_DISCON_ROUTER;
                gizwitsProtocol.processEvent.num++;
                os_printf("WiFi status: disconnected router\r\n");
            }
        }

        //M2M server status
        if(lastStatus.types.con_m2m != status.types.con_m2m)
        {
            lastStatus.types.con_m2m = status.types.con_m2m;
            if(1 == status.types.con_m2m)
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_CON_M2M;
                gizwitsProtocol.processEvent.num++;
                os_printf("WiFi status: connected m2m\r\n");
            }
            else
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_DISCON_M2M;
                gizwitsProtocol.processEvent.num++;
                os_printf("WiFi status: disconnected m2m\r\n");
            }
        }

        //APP status
        if(lastStatus.types.app != status.types.app)
        {
            lastStatus.types.app = status.types.app;
            if(1 == status.types.app)
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_CON_APP;
                gizwitsProtocol.processEvent.num++;
                os_printf("WiFi status: app connect\r\n");
            }
            else
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_DISCON_APP;
                gizwitsProtocol.processEvent.num++;
                os_printf("WiFi status: no app connect\r\n");
            }
        }

        //test mode status
        if(lastStatus.types.test != status.types.test)
        {
            lastStatus.types.test = status.types.test;
            if(1 == status.types.test)
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_OPEN_TESTMODE;
                gizwitsProtocol.processEvent.num++;
                os_printf("WiFi status: in test mode\r\n");
            }
            else
            {
                gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_CLOSE_TESTMODE;
                gizwitsProtocol.processEvent.num++;
                os_printf("WiFi status: out test mode\r\n");
            }
        }

        rssiValue = status.types.rssi;
        gizwitsProtocol.processEvent.event[gizwitsProtocol.processEvent.num] = WIFI_RSSI;
        gizwitsProtocol.processEvent.num++;

        gizEventProcess(&gizwitsProtocol.processEvent, (uint8_t *)&rssiValue, sizeof(rssiValue));
        os_memset((uint8_t *)&gizwitsProtocol.processEvent, 0, sizeof(event_info_t));

        lastStatus = status;
    }
}

/*
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
*/

/*void ICACHE_FLASH_ATTR issuedDataDTC(gizwits_issued_t * dst, gizwits_issued_t * src)
{   
    int16_t value = 0;

    if((NULL != src)&&(NULL != dst))
    {
        os_memcpy((uint8_t *)dst, (uint8_t *)src, sizeof(gizwits_issued_t)); 
        
        //custom
        dst->attr_vals.led_r = X2Y(LED_R_RATIO, LED_R_ADDITION, src->attr_vals.led_r); 
        dst->attr_vals.led_g = X2Y(LED_G_RATIO, LED_G_ADDITION, src->attr_vals.led_g); 
        dst->attr_vals.led_b = X2Y(LED_B_RATIO, LED_B_ADDITION, src->attr_vals.led_b); 
        value = exchangeBytes(src->attr_vals.motor); 
        dst->attr_vals.motor = X2Y(MOTOR_SPEED_RATIO, MOTOR_SPEED_ADDITION, value); 
    }
}
*/

void ICACHE_FLASH_ATTR dataPoint2Event(event_info_t * info, gizwits_issued_t * issuedData)
{
    os_printf("$$$$$ CONTROL_DEVICE[vals]-> [onoff:%x],[Color:%x],[LED_R:%d],[LED_G:%d],[SetLED_B:%d],[motor:%d] \n",
              issuedData->attr_vals.led_onoff, issuedData->attr_vals.led_color, issuedData->attr_vals.led_r, issuedData->attr_vals.led_g, issuedData->attr_vals.led_b, issuedData->attr_vals.motor);

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

int8_t ICACHE_FLASH_ATTR checkReport(gizwits_report_t * cur, gizwits_report_t * last)
{
    int8_t ret = 0;
    static uint32 lastReportMs = 0;

    if((NULL == cur) || (NULL == last))
    {
        os_printf("!!! checkReport Error \n");

        return (-1);
    }

    if(last->dev_status.led_onoff != cur->dev_status.led_onoff)
    {
        os_printf("led_onoff %d %d\r\n", last->dev_status.led_onoff, cur->dev_status.led_onoff);
        ret = 1;
    }
    
    if(last->dev_status.led_color != cur->dev_status.led_color)
    {
        os_printf("led_color \r\n", last->dev_status.led_color, cur->dev_status.led_color); 
        ret = 1;
    }
    
    if(last->dev_status.led_r != cur->dev_status.led_r)
    {
        os_printf("led_r %d %d\r\n", last->dev_status.led_r, cur->dev_status.led_r); 
        ret = 1;
    }
    
    if(last->dev_status.led_g != cur->dev_status.led_g)
    {
        os_printf("led_g %d %d\r\n", last->dev_status.led_g, cur->dev_status.led_g); 
        ret = 1;
    }
    
    if(last->dev_status.led_b != cur->dev_status.led_b)
    {
        os_printf("led_b %d %d\r\n", last->dev_status.led_b, cur->dev_status.led_b); 
        ret = 1;
    }
    
    if(last->dev_status.motor != cur->dev_status.motor)
    {
        os_printf("motor %d %d\r\n", last->dev_status.motor, cur->dev_status.motor); 
        ret = 1;
    }
    
    if(last->dev_status.infrared != cur->dev_status.infrared)
    {
        os_printf("infrared %d %d\r\n", last->dev_status.infrared, cur->dev_status.infrared); 
        ret = 1;
    }
    
    if(last->dev_status.temperature != cur->dev_status.temperature)
    {
        if(MIN_INTERVAL_TIME < gizGetIntervalsMs(lastReportMs)) 
        {
            os_printf("temperature %d %d\r\n", last->dev_status.temperature, cur->dev_status.temperature); 
            lastReportMs = gizTimeMs(); 
            ret = 1;
        }
    }
    
    if(last->dev_status.humidity != cur->dev_status.humidity)
    {
        if(MIN_INTERVAL_TIME < gizGetIntervalsMs(lastReportMs)) 
        {
            os_printf("humidity %d %d\r\n", last->dev_status.humidity, cur->dev_status.humidity); 
            lastReportMs = gizTimeMs(); 
            ret = 1;
        }
    }

    if(last->dev_status.alert != cur->dev_status.alert)
    {
        if(MIN_INTERVAL_TIME < gizGetIntervalsMs(lastReportMs)) 
        {
            os_printf("alert \r\n"); 
            lastReportMs = gizTimeMs(); 
            ret = 1;
        }
    }

    if(last->dev_status.fault != cur->dev_status.fault)
    {
        if(MIN_INTERVAL_TIME < gizGetIntervalsMs(lastReportMs)) 
        {
            os_printf("fault \r\n"); 
            lastReportMs = gizTimeMs(); 
            ret = 1;
        }
    }

    return ret;
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
    //gizwits_report_t * reportData = (gizwits_report_t *)&gizwitsProtocol.reportTransparentBuf;

    if((NULL == gizIssuedData) || (NULL == outData) || (NULL == outLen))
    {
        os_printf("!!! IssuedProcess Error \n");

        return (-1);
    }

    switch(inData[0])
    {
        case ACTION_CONTROL_DEVICE:
            dataPoint2Event((event_info_t *)&gizwitsProtocol.processEvent, gizIssuedData);
//          issuedDataDTC((gizwits_issued_t *)&gizwitsProtocol.issuedData, gizIssuedData);
            
            os_memcpy((uint8_t *)&gizwitsProtocol.issuedData, (uint8_t *)gizIssuedData, sizeof(gizwits_issued_t)); 
            
            system_os_post(USER_TASK_PRIO_2, SIG_ISSUED_DATA, 0); 
            *outLen = 0; 
            
            break;
            
        case ACTION_READ_DEV_STATUS:
            gizwitsProtocol.lastReportData.action = ACTION_READ_DEV_STATUS_ACK;
            //reportDataDTC((dev_status_t *)&gizwitsProtocol.lastReportData.dev_status, (dev_status_t *)&reportData->dev_status);
            
            os_memcpy(outData, (uint8_t *)&gizwitsProtocol.lastReportData, sizeof(gizwits_report_t));
            *outLen = sizeof(gizwits_report_t);
            
            os_printf("$$$$$ report_STA : [act:%x],[onf:%x],[Col:%x],[R:%d],[G:%d],[B:%d],[mo:%d],[inf:%d],[tem:%d]],[hum:%d] \n",
                      gizwitsProtocol.lastReportData.action, gizwitsProtocol.lastReportData.dev_status.led_onoff, gizwitsProtocol.lastReportData.dev_status.led_color, gizwitsProtocol.lastReportData.dev_status.led_r, gizwitsProtocol.lastReportData.dev_status.led_g,
                      gizwitsProtocol.lastReportData.dev_status.led_b, gizwitsProtocol.lastReportData.dev_status.motor, gizwitsProtocol.lastReportData.dev_status.infrared, gizwitsProtocol.lastReportData.dev_status.temperature, gizwitsProtocol.lastReportData.dev_status.humidity);

            break;

        case ACTION_W2D_PASSTHROUGH: //透传
            os_memcpy(gizwitsProtocol.issuedTransparentBuf, &inData[1], inLen-1);
            gizwitsProtocol.issuedTransparentLen = inLen-1;

            system_os_post(USER_TASK_PRIO_2, SIG_PASSTHROUGH, 0);
            *outLen = 0;
            break;

        default:
            break;
    }

    return 0;
}

int32_t ICACHE_FLASH_ATTR gizReportData(uint8_t action, uint8_t * data, uint32_t len)
{
    uint8_t *passReportBuf = NULL;
    gizwits_report_t * curReportData = (gizwits_report_t *)data;

    if(NULL == data)
    {
        os_printf("!!! gizReportData Error \n");

        return (-1);
    }

    if(ACTION_REPORT_DEV_STATUS == action)
    {
//      reportDataDTC((dev_status_t *)&curReportData.dev_status, (dev_status_t *)&(reportData->dev_status));
        
        //Regularly report conditional
        if((1 == checkReport(curReportData, (gizwits_report_t *)&gizwitsProtocol.lastReportData))) 
        {
            os_memcpy((uint8_t *)&gizwitsProtocol.lastReportData, (uint8_t *)curReportData, sizeof(gizwits_report_t)); 
            gizwitsProtocol.lastReportData.action = action;

            os_printf("$$$$$ changed report data : [act:%x],[onf:%x],[Col:%x],[R:%d],[G:%d],[B:%d],[mo:%d],[inf:%d],[tem:%d]],[hum:%d] \n",
                      reportData.action, reportData.dev_status.led_onoff, reportData.dev_status.led_color, reportData.dev_status.led_r, reportData.dev_status.led_g,
                      reportData.dev_status.led_b, reportData.dev_status.motor, reportData.dev_status.infrared, reportData.dev_status.temperature, reportData.dev_status.humidity);
            
            system_os_post(USER_TASK_PRIO_2, SIG_IMM_REPORT, 0);
        }
    }
    else if(ACTION_D2W_PASSTHROUGH == action)
    {
        passReportBuf = (uint8 * )os_malloc(len + 1);

        passReportBuf[0] = action;
        os_memcpy((uint8_t *)&passReportBuf[1], data, len);

        gagentUploadData(passReportBuf, len+1);

        os_free(passReportBuf);
    }

    return 0;
}

uint32_t ICACHE_FLASH_ATTR gizGetTimeStamp(void)
{
    _tm tmValue;

    gagentGetNTP(&tmValue);

    return tmValue.ntp;
}

void ICACHE_FLASH_ATTR gizwitsTimerFunc(void)
{
    static uint32 repCtime = 0; 

    //600s Regularly report
    if(TIM_REP_TIMOUT < repCtime)
    {
        repCtime = 0;

        os_printf("@@@@ gokit_timing report! \n");

        system_os_post(USER_TASK_PRIO_2, SIG_IMM_REPORT, 0);
    }

    repCtime++; 
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
        gizEventProcess(&gizwitsProtocol.processEvent, (uint8_t *)&gizwitsProtocol.issuedData, sizeof(gizwits_issued_t));

        //clean event info
        os_memset((uint8_t *)&gizwitsProtocol.processEvent, 0, sizeof(event_info_t));
        break;
    case SIG_PASSTHROUGH:
        gizwitsProtocol.processEvent.event[0] = TRANSPARENT_DATA;
        gizwitsProtocol.processEvent.num = 1;
        gizEventProcess(&gizwitsProtocol.processEvent, (uint8_t *)gizwitsProtocol.issuedTransparentBuf, gizwitsProtocol.issuedTransparentLen);

        os_memset((uint8_t *)&gizwitsProtocol.processEvent, 0, sizeof(event_info_t));
        os_memset(gizwitsProtocol.issuedTransparentBuf, 0, BUFFER_LEN_MAX);
        gizwitsProtocol.issuedTransparentLen = 0;
        break;
    case SIG_IMM_REPORT:
        gagentUploadData((uint8_t *)&gizwitsProtocol.lastReportData, sizeof(gizwits_report_t));
        break;
    default:
        os_printf("---error sig! ---\n");
        break;
    }
}

void ICACHE_FLASH_ATTR gizwitsInit(void)
{
    int16_t value = 0; 
    
    os_memset(gizwitsProtocol.issuedTransparentBuf, 0, sizeof(gizwitsProtocol.issuedTransparentBuf));
    os_memset(gizwitsProtocol.lastReportData, 0, sizeof(gizwits_report_t)); 
    gizwitsProtocol.issuedTransparentLen = 0;
    
    value = Y2X(MOTOR_SPEED_RATIO, MOTOR_SPEED_ADDITION, MOTOR_SPEED_DEFAULT); 
    reportData.dev_status.motor = exchangeBytes(value); 

    system_os_task(gizwitsTask, USER_TASK_PRIO_2, gizwitsTaskQueue, gizwitsTaskQueueLen);

    //gokit timer start
    os_timer_disarm(&gizTimer);
    os_timer_setfn(&gizTimer, (os_timer_func_t *)gizwitsTimerFunc, NULL);
    os_timer_arm(&gizTimer, MAX_SOC_TIMOUT, 1);

    os_printf("gizwitsInit OK \r\n");
}

