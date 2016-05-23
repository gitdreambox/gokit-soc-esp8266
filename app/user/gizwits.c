#include "gizwits.h"
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "gagent_external.h"
#include "driver/hal_key.h"
#include "driver/hal_infrared.h"
#include "driver/hal_motor.h"
#include "driver/hal_rgb_led.h"
#include "driver/hal_temp_hum.h"

LOCAL uint8_t issuedBuf[256]; 
LOCAL uint8_t reportBuf[256]; 
uint32_t issuedLen = 0;
uint32_t reportLen = 0;
event_info_t info;
LOCAL key_typedef_t * single_key[2];
LOCAL keys_typedef_t keys;
uint8_t gokit_config_flag = 0; 
uint8_t condition_flag = 0; 

extern void ICACHE_FLASH_ATTR gizEventProcess(event_info_t *info, uint8_t *data);

/*
 * htons unsigned short -> 网络字节序
 * htonl unsigned long  -> 网络字节序
 * ntohs 网络字节序 -> unsigned short
 * ntohl 网络字节序 -> unsigned long
 */
uint16_t exchange_bytes(uint16_t value)
{
    uint16_t    tmp_value;
    uint8_t     *index_1, *index_2;

    index_1 = (uint8_t *)&tmp_value;
    index_2 = (uint8_t *)&value;

    *index_1 = *(index_2+1);
    *(index_1+1) = *index_2;

    return tmp_value;
}

/**
* @brief Data Type Conversions x : 转化为协议中的x值及实际通讯传输的值
*
* @param ratio : 修正系数k
* @param addition : 增量m
* @param pre_value : 作为协议中的y值, 是App UI界面的显示值
*
* @return uint16_t aft_value : 作为协议中的x值, 是实际通讯传输的值
*/
LOCAL uint32 ICACHE_FLASH_ATTR dtcX(uint32 ratio, int32 addition, uint32 pre_value)
{
    uint32 temp_value = 0;
    uint32 aft_value = 0;

    temp_value = (uint32)pre_value;

    //x=(y - m)/k
    aft_value = ((temp_value - addition) / ratio);

    return aft_value;
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
LOCAL int32 ICACHE_FLASH_ATTR dtcY(uint32 ratio, int32 addition, uint32 pre_value)
{
    uint32 temp_value = 0;
    int32 aft_value = 0;

    temp_value = pre_value;

    //y=k * x + m
    aft_value = (temp_value * ratio + addition);

    return aft_value;
}

void ICACHE_FLASH_ATTR msleep(void)
{
    int i;
    
    for(i = 0; i < 20; i++)
    {
        os_delay_us(50000);
    }
}

unsigned long gokit_time_ms(void)
{
    return (system_get_time() / 1000); 
}

unsigned long gokit_time_s(void)
{
    return (system_get_time() / 1000*1000); 
}

uint8_t ICACHE_FLASH_ATTR gokit_set_rpflag(void)
{
    condition_flag = 1;
}

uint8_t ICACHE_FLASH_ATTR gokit_clean_rpflag(void)
{
    condition_flag = 0;
}

LOCAL uint8_t ICACHE_FLASH_ATTR gokit_judge_rpflag_handle(void)
{
    if(1 == condition_flag) 
    {
        system_os_post(USER_TASK_PRIO_0, SIG_REPORT_JUDGMENT, 0); 
    }
}

uint8_t ICACHE_FLASH_ATTR gokit_Immediate_report(void)
{
    gizwitsReport_t * reportData = (gizwitsReport_t *)&reportBuf; 
    
    os_printf("$$$$$ report_val : [onf:%x],[Col:%x],[R:%d],[G:%d],[B:%d],[mo:%d],[inf:%d],[tem:%d]],[hum:%d] \n",
              reportData->dev_status.led_onoff, reportData->dev_status.led_color, reportData->dev_status.led_r, reportData->dev_status.led_g,
              reportData->dev_status.led_b, reportData->dev_status.motor, reportData->dev_status.infrared, reportData->dev_status.temperature, reportData->dev_status.humidity);

    gizReportData(ACTION_REPORT_DEV_STATUS, (uint8_t *)&reportData->dev_status, sizeof(reportData->dev_status)); 
    
    return (0);
}

uint8_t ICACHE_FLASH_ATTR gokit_th_read_handle(void)
{
    static uint8_t pre_tem_means_val = 0;
    static uint8_t pre_hum_means_val = 0;
    uint8_t curTem = 0, curHum = 0;
    uint16_t tem_means = 0, hum_means = 0; 
    uint8_t ret = 0;
    gizwitsReport_t * reportData = (gizwitsReport_t *)&reportBuf; 
    
    ret = dh11_read(&curTem, &curHum);
    if(1 == ret)
    {
        os_printf("@@@@ dh11_read error ! \n"); 
        return 0;
    }
    
    //Being the first time and the initiative to report
    if((pre_tem_means_val == 0) || (pre_hum_means_val == 0))
    {
        pre_tem_means_val = curTem;
        pre_hum_means_val = curHum;

        //Assignment to report data
        reportData->dev_status.temperature = (uint8_t)dtcX(TEMPERATURE_RATIO, TEMPERATURE_ADDITION, (uint32)pre_tem_means_val); 
        reportData->dev_status.humidity = (uint8_t)dtcX(HUMIDITY_RATIO, HUMIDITY_ADDITION, (uint32)pre_hum_means_val); 
        
        os_printf("#### Tem = %d; Hum = %d \n", pre_tem_means_val, pre_hum_means_val); 
        os_printf("#### Tem = %d; Hum = %d \n", reportData->dev_status.temperature, reportData->dev_status.humidity); 

        system_os_post(USER_TASK_PRIO_0, SIG_SET_REPFLAG, 0); 
        
        return (1);
    }

    //Before and after the two values, it is determined whether or not reported
    if((pre_tem_means_val != curTem) || (pre_hum_means_val != curHum))
    {
        pre_tem_means_val = curTem;
        pre_hum_means_val = curHum;

        //Assignment to report data
        reportData->dev_status.temperature = (uint8_t)dtcX(TEMPERATURE_RATIO, TEMPERATURE_ADDITION, (uint32)pre_tem_means_val); 
        reportData->dev_status.humidity = (uint8_t)dtcX(HUMIDITY_RATIO, HUMIDITY_ADDITION, (uint32)pre_hum_means_val); 
        
        os_printf("#### Tem = %d; Hum = %d \n", pre_tem_means_val, pre_hum_means_val);
        os_printf("#### rp_Tem = %d; rp_Hum = %d \n", reportData->dev_status.temperature, reportData->dev_status.humidity); 

        system_os_post(USER_TASK_PRIO_0, SIG_SET_REPFLAG, 0); 

        return (1);
    }
    
    return (0); 
}

LOCAL uint8_t ICACHE_FLASH_ATTR gokit_th_handle(void)
{
    static uint16_t th_ctime = 0;
    
    if(TH_TIMEOUT < th_ctime) 
    {
        th_ctime = 0;
 
        system_os_post(USER_TASK_PRIO_0, SIG_HD11_READ, 0); 
        
        return (1); 
    }
        
    th_ctime++; 
    
    return (0); 
}

uint8_t ICACHE_FLASH_ATTR gokit_ir_read_handle(void)
{
    uint8_t ir_value = 0; 
    gizwitsReport_t * reportData = (gizwitsReport_t *)&reportBuf; 
    
    ir_value = ir_update_status();

    if(ir_value != reportData->dev_status.infrared) 
    {
        reportData->dev_status.infrared = ir_value; 

        system_os_post(USER_TASK_PRIO_0, SIG_IMM_REPORT, 0); 

        return (1);
    }
    
    return (0);
}

LOCAL uint8_t ICACHE_FLASH_ATTR gokit_ir_handle(void)
{
    static uint32 ir_ctime = 0; 
        
    if(INF_TIMEOUT < ir_ctime) 
    {
        ir_ctime = 0;
        
        system_os_post(USER_TASK_PRIO_0, SIG_INFRARED_READ, 0); 
        
        return (1);
    }
    
    ir_ctime++;
        
    return (0);
}

LOCAL uint8_t ICACHE_FLASH_ATTR gokit_regularly_report_handle(void)
{
    static uint32 rep_ctime = 0;

    //600s Regularly report
    if(TIM_REP_TIMOUT < rep_ctime) 
    {
        rep_ctime = 0;
        
        os_printf("@@@@ gokit_timing report! \n"); 
        
        system_os_post(USER_TASK_PRIO_0, SIG_SET_REPFLAG, 0); 

        return (1);
    }
    
    rep_ctime++;
    
    return (0);
}

uint8_t ICACHE_FLASH_ATTR gokit_report_judge(void)
{
    static uint32 last_report_ms = 0;
    uint32 intervals_ms = 0;
    gizwitsReport_t * reportData = (gizwitsReport_t *)&reportBuf; 
    
    if(0 < (gokit_time_ms() - last_report_ms))
    {
        intervals_ms = gokit_time_ms() - last_report_ms;
    }
    else
    {
        intervals_ms = (0xffffffff - last_report_ms) + gokit_time_ms();
    }
    
    if(MIN_INTERVAL_TIME < intervals_ms) 
    {
        os_printf("$$$$$ report_val : [onf:%x],[Col:%x],[R:%d],[G:%d],[B:%d],[mo:%d],[inf:%d],[tem:%d]],[hum:%d] \n", 
                  reportData->dev_status.led_onoff, reportData->dev_status.led_color, reportData->dev_status.led_r, reportData->dev_status.led_g, 
                  reportData->dev_status.led_b, reportData->dev_status.motor, reportData->dev_status.infrared, reportData->dev_status.temperature, reportData->dev_status.humidity); 

        gizReportData(ACTION_REPORT_DEV_STATUS, (uint8_t *)&reportData->dev_status, sizeof(reportData->dev_status)); 
        
        last_report_ms = gokit_time_ms(); 
        
        system_os_post(USER_TASK_PRIO_0, SIG_CLE_REPFLAG, 0); 
        
        return (1); 
    }

    return (0);
}

void ICACHE_FLASH_ATTR gizSetMode(uint8_t mode)
{
    gagentConfig(mode);
}

void ICACHE_FLASH_ATTR gizConfigReset(void)
{
    gagentReset();
}

void ICACHE_FLASH_ATTR gizWiFiStatus(uint16_t value)
{
    uint8_t rssiValue = 0;
    wifi_status_t status;
    static wifi_status_t lastStatus;

    status.value = value; 

    os_printf("@@@@ GAgentStatus[hex]:%02x | [Bin]:%d,%d,%d,%d,%d,%d \r\n", status.value, status.types.con_m2m, status.types.con_route, status.types.binding, status.types.onboarding, status.types.station, status.types.softap); 
    
    //OnBoarding mode status
    if(lastStatus.types.onboarding != status.types.onboarding) 
    {
        if(1 == status.types.onboarding)
        {
            if(1 == status.types.softap)
            {
                info.event[info.num] = WIFI_SOFTAP;
                info.num++;
                os_printf("OnBoarding: SoftAP or Web mode\r\n");
            }

            if(1 == status.types.station)
            {
                info.event[info.num] = WIFI_AIRLINK;
                info.num++;
                os_printf("OnBoarding: AirLink mode\r\n");
            }
        }
        else
        {
            if(1 == status.types.softap)
            {
                info.event[info.num] = WIFI_SOFTAP;
                info.num++;
                os_printf("OnBoarding: SoftAP or Web mode\r\n");
            }

            if(1 == status.types.station)
            {
                info.event[info.num] = WIFI_STATION;
                info.num++;
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
            info.event[info.num] = WIFI_OPEN_BINDING;
            info.num++;
            os_printf("WiFi status: in binding mode\r\n");
        }
        else
        {
            info.event[info.num] = WIFI_CLOSE_BINDING;
            info.num++;
            os_printf("WiFi status: out binding mode\r\n");
        }
    }

    //router status
    if(lastStatus.types.con_route != status.types.con_route) 
    {
        lastStatus.types.con_route = status.types.con_route; 
        if(1 == status.types.con_route)
        {
            info.event[info.num] = WIFI_CON_ROUTER;
            info.num++;
            os_printf("WiFi status: connected router\r\n");
        }
        else
        {
            info.event[info.num] = WIFI_DISCON_ROUTER;
            info.num++;
            os_printf("WiFi status: disconnected router\r\n");
        }
    }

    //M2M server status
    if(lastStatus.types.con_m2m != status.types.con_m2m) 
    {
        lastStatus.types.con_m2m = status.types.con_m2m; 
        if(1 == status.types.con_m2m)
        {
            info.event[info.num] = WIFI_CON_M2M;
            info.num++;
            os_printf("WiFi status: connected m2m\r\n");
        }
        else
        {
            info.event[info.num] = WIFI_DISCON_M2M;
            info.num++;
            os_printf("WiFi status: disconnected m2m\r\n");
        }
    }

    //APP status
    if(lastStatus.types.app != status.types.app) 
    {
        lastStatus.types.app = status.types.app; 
        if(1 == status.types.app)
        {
            info.event[info.num] = WIFI_CON_APP;
            info.num++;
            os_printf("WiFi status: app connect\r\n");
        }
        else
        {
            info.event[info.num] = WIFI_DISCON_APP;
            info.num++;
            os_printf("WiFi status: no app connect\r\n");
        }
    }

    //test mode status
    if(lastStatus.types.test != status.types.test) 
    {
        lastStatus.types.test = status.types.test; 
        if(1 == status.types.test)
        {
            info.event[info.num] = WIFI_OPEN_TESTMODE;
            info.num++;
            os_printf("WiFi status: in test mode\r\n");
        }
        else
        {
            info.event[info.num] = WIFI_CLOSE_TESTMODE;
            info.num++;
            os_printf("WiFi status: out test mode\r\n");
        }
    }

    rssiValue = status.types.rssi;
    info.event[info.num] = WIFI_RSSI;
    info.num++;

    gizEventProcess(&info, (uint8_t *)&rssiValue);
    memset((uint8_t *)&info, 0, sizeof(event_info_t)); 

    lastStatus = status; 
}

void ICACHE_FLASH_ATTR dataPoint2Event(event_info_t * info, gizwitsIssued_t * issuedData, gizwitsIssued_t * controlData)
{
//  os_printf("$$$$$ CONTROL_DEVICE[flag]-> [onoff:%d],[Color:%d],[LED_R:%d],[LED_G:%d],[SetLED_B:%d],[motor:%d] \n",
//            issuedData->attr_vals.attr_flags.led_onoff, issuedData->attr_vals.attr_flags.led_color, issuedData->attr_vals.attr_flags.led_r, issuedData->attr_vals.attr_flags.led_g, issuedData->attr_vals.attr_flags.led_b, issuedData->attr_vals.attr_flags.motor);
    os_printf("$$$$$ CONTROL_DEVICE[vals]-> [onoff:%x],[Color:%x],[LED_R:%d],[LED_G:%d],[SetLED_B:%d],[motor:%d] \n",
              issuedData->attr_vals.led_onoff, issuedData->attr_vals.led_color, issuedData->attr_vals.led_r, issuedData->attr_vals.led_g, issuedData->attr_vals.led_b, issuedData->attr_vals.motor_speed);

    if((NULL != info)&&(NULL != issuedData))
    {
        if(0x01 == issuedData->attr_flags.led_onoff)
        {
            controlData->attr_vals.led_onoff = issuedData->attr_vals.led_onoff; 
            
            info->event[info->num] = SetLED_OnOff;
            info->num++;
        }
        if(0x01 == issuedData->attr_flags.led_color)
        {
            controlData->attr_vals.led_color = issuedData->attr_vals.led_color;
                
            info->event[info->num] = SetLED_Color;
            info->num++;
        }
        if(0x01 == issuedData->attr_flags.led_r)
        {
            controlData->attr_vals.led_r = dtcX(LED_R_RATIO, LED_R_ADDITION, issuedData->attr_vals.led_r); 
            
            info->event[info->num] = SetLED_R;
            info->num++;
        }
        if(0x01 == issuedData->attr_flags.led_g)
        {
            controlData->attr_vals.led_g = dtcX(LED_G_RATIO, LED_G_ADDITION, issuedData->attr_vals.led_g); 
            
            info->event[info->num] = SetLED_G;
            info->num++;
        }
        if(0x01 == issuedData->attr_flags.led_b)
        {
            controlData->attr_vals.led_b = dtcX(LED_B_RATIO, LED_B_ADDITION, issuedData->attr_vals.led_b); 
            
            info->event[info->num] = SetLED_B;
            info->num++;
        }
        if(0x01 == issuedData->attr_flags.motor)
        {
            controlData->attr_vals.motor_speed = exchange_bytes(issuedData->attr_vals.motor_speed);
            
            info->event[info->num] = SetMotor;
            info->num++;
        }
    }
}

int32_t ICACHE_FLASH_ATTR gizIssuedProcess(uint8_t *inData, uint32_t inLen, uint8_t *outData, int32_t *outLen)
{
    uint8_t action = inData[0];
    gizwitsIssued_t *gizIssuedData= (gizwitsIssued_t *)&inData[1];
    gizwitsReport_t * reportData = (gizwitsReport_t *)&reportBuf; 

    outData = reportBuf;
    outLen = (int32_t *)&reportLen;
    
    switch(action)
    {
        case ACTION_CONTROL_DEVICE:
            dataPoint2Event((event_info_t *)&info, gizIssuedData, (gizwitsIssued_t *)&issuedBuf); 
            
            system_os_post(0, SIG_ISSUED_DATA, NULL);
            
            *outLen = 0; 
            
            break;
            
        case ACTION_READ_DEV_STATUS:
            os_printf("$$$$$ READ_DEV_STATUS \n"); 
            
            outData[0] = ACTION_READ_DEV_STATUS_ACK;
            *outLen = sizeof(gizwitsReport_t); 
 
            break;
            
        case ACTION_W2D_PASSTHROUGH: //透传
            os_memcpy(issuedBuf, &inData[1], inLen-1);
            issuedLen = inLen-1;
            *outLen = 0;
            
            system_os_post(0, SIG_PASSTHROUGH, NULL);
            
            break;
            
        default:
        
            break;
    }

    return 0;
}

int32_t ICACHE_FLASH_ATTR gizReportData(uint8_t action, uint8_t *data, uint32_t len)
{
    uint8_t sendData[256];

    sendData[0] = action;
    memcpy(&sendData[1], data, len); 

    gagentUploadData(sendData, len+1);

    return 0;
}

uint32_t ICACHE_FLASH_ATTR gizGetTimeStamp(void)
{
    _tm tmValue;

    gagentGetNTP(&tmValue);

    return tmValue.ntp;
}

void ICACHE_FLASH_ATTR gizEventProcess(event_info_t *info, uint8_t *data)
{
    uint8_t i = 0;
    uint8_t rssi = *data;
    gizwitsIssued_t *issued = (gizwitsIssued_t *)data;
    gizwitsReport_t * reportData = (gizwitsReport_t *)&reportBuf; 
    static uint8_t gokit_led_status = 0; 

    for(i=0; i<info->num; i++)
    {
        switch(info->event[i])
        {
            case WIFI_SOFTAP:
                break;
            case WIFI_AIRLINK:
                break;
            case WIFI_CON_ROUTER:
                os_printf("connected router\n");
                
                if(gokit_config_flag == 1)
                {
                    os_printf( "@@@@ W2M->WIFI_CONNROUTER \n"); 

                    gokit_config_flag = 0;
                    rgb_control(0, 0, 0);
                }
                break; 
            case WIFI_DISCON_ROUTER:
                os_printf("disconnected router\n");
                break;
            case WIFI_CON_M2M:
                os_printf("connected m2m\n");
                break;
            case WIFI_DISCON_M2M:
                os_printf("disconnected m2m\n");
                break;
            case WIFI_RSSI:
                os_printf("rssi is %d\n", rssi);
                break;

            //coustm
            case SetLED_OnOff:
                os_printf("########## led_onoff is %d\n", issued->attr_vals.led_onoff); 
                if(issued->attr_vals.led_onoff == LED_Off) 
                {
                    reportData->dev_status.led_onoff = LED_Off; 

                    rgb_control(0, 0, 0);

                    os_printf("########## setled_Off \r\n"); 
                }
                if(issued->attr_vals.led_onoff == LED_On) 
                {
                    reportData->dev_status.led_onoff = LED_On; 

                    rgb_control(254, 0, 0);

                    os_printf("########## setled_On \r\n"); 
                }
                break;
            case SetLED_Color:
                os_printf("########## led_color is %d\n", issued->attr_vals.led_color); 
                if(issued->attr_vals.led_color == LED_Costom) 
                {
                    gokit_led_status = 0;

                    reportData->dev_status.led_color = LED_Costom;

                    rgb_control(reportData->dev_status.led_r, reportData->dev_status.led_g, reportData->dev_status.led_b); 
                }

                if(issued->attr_vals.led_color == LED_Yellow) 
                {
                    gokit_led_status = 1;

                    reportData->dev_status.led_color = LED_Yellow; 

                    rgb_control(254, 254, 0);

                    os_printf("########## SetLED LED_Yellow \r\n"); 
                }

                if(issued->attr_vals.led_color == LED_Purple) 
                {
                    gokit_led_status = 1;

                    reportData->dev_status.led_color = LED_Purple; 

                    rgb_control(254, 0, 70);

                    os_printf("########## SetLED LED_Purple \r\n"); 
                }
                if(issued->attr_vals.led_color == LED_Pink) 
                {
                    gokit_led_status = 1;

                    reportData->dev_status.led_color = LED_Pink; 

                    rgb_control(238, 30, 30);

                    os_printf("########## SetLED LED_Pink \r\n"); 
                }
                break;
            case SetLED_R:
                os_printf("########## led_r is %d\n", issued->attr_vals.led_r); 
                if(gokit_led_status != 1)
                {
                    reportData->dev_status.led_r = issued->attr_vals.led_r; 

                    rgb_control(reportData->dev_status.led_r, reportData->dev_status.led_g, reportData->dev_status.led_b); 
                }
                break;
            case SetLED_G:
                os_printf("########## led_g is %d\n", issued->attr_vals.led_g); 
                if(gokit_led_status != 1)
                {
                    reportData->dev_status.led_g = issued->attr_vals.led_g; 

                    rgb_control(reportData->dev_status.led_r, reportData->dev_status.led_g, reportData->dev_status.led_b); 
                }
                break;
            case SetLED_B:
                os_printf("########## led_b is %d\n", issued->attr_vals.led_b); 
                if(gokit_led_status != 1)
                {
                    reportData->dev_status.led_b = issued->attr_vals.led_b; 

                    rgb_control(reportData->dev_status.led_r, reportData->dev_status.led_g, reportData->dev_status.led_b); 
                }
                break;
            case SetMotor:
                os_printf("########## motor speed is %d\n", issued->attr_vals.motor_speed); 
                reportData->dev_status.motor = exchange_bytes(issued->attr_vals.motor_speed); 
                
                motor_control((_MOTOR_T)dtcY(MOTOR_SPEED_RATIO, MOTOR_SPEED_ADDITION, issued->attr_vals.motor_speed)); 
                break;
            default:
                break;
        }
    }
    
    //Immediately reported data
    gokit_Immediate_report(); 
}

LOCAL void ICACHE_FLASH_ATTR key2_short_press(void)
{
    os_printf("#### key2, soft ap mode \n"); 

    rgb_control(250, 0, 0);
    gokit_config_flag = 1; 
    
    gizSetMode(1);
}

LOCAL void ICACHE_FLASH_ATTR key2_long_press(void)
{
    os_printf("#### key2, airlink mode\n"); 

    rgb_control(0, 250, 0);
    gokit_config_flag = 1; 
    
    gizSetMode(2);
}

LOCAL void ICACHE_FLASH_ATTR key1_short_press(void)
{
    os_printf("#### key1, press \n");

    rgb_control(0, 0, 250); 
}

LOCAL void ICACHE_FLASH_ATTR key1_long_press(void)
{
    os_printf("#### key1, default setup\n"); 
    msleep();
    gizConfigReset();
}

void ICACHE_FLASH_ATTR gizwitsInit(void)
{
    gizwitsReport_t * reportData = (gizwitsReport_t *)&reportBuf; 
    
    memset(issuedBuf, 0, sizeof(issuedBuf)); 
    memset(reportBuf, 0, sizeof(reportBuf)); 
    issuedLen = 0;
    reportLen = 0;

    //rgb led init
    rgb_gpio_init();
    rgb_led_init(); 
    
    //key init
    single_key[0] = key_init_one(KEY_0_IO_NUM, KEY_0_IO_MUX, KEY_0_IO_FUNC,
                                 key1_long_press, key1_short_press);
    single_key[1] = key_init_one(KEY_1_IO_NUM, KEY_1_IO_MUX, KEY_1_IO_FUNC,
                                 key2_long_press, key2_short_press);
    keys.key_num = GPIO_KEY_NUM;
    keys.key_timer_ms = 10;
    keys.single_key = single_key;
    key_para_init(&keys); 
    
    //motor init
    motor_init();
    motor_control(MOTOR_SPEED_DEFAULT); 
    reportData->dev_status.motor = exchange_bytes((uint16_t)dtcX(MOTOR_SPEED_RATIO, MOTOR_SPEED_ADDITION, (uint32)MOTOR_SPEED_DEFAULT));
    
    //temperature and humidity init
    dh11_init();

    //Infrared init
    ir_init();

    os_printf("gizwitsInit OK \r\n"); 
}

void ICACHE_FLASH_ATTR gizwitsTimerFunc(void)
{
    //Temperature and humidity sensors reported conditional
    gokit_th_handle();

    //Infrared sensors reported conditional
    gokit_ir_handle();

    //Regularly report conditional
    gokit_regularly_report_handle();

    //Qualifying the initiative to report
    gokit_judge_rpflag_handle();
}

void ICACHE_FLASH_ATTR gizwitsTask(os_event_t *events)
{
    uint8_t i = 0;
    uint8 vchar = 0; 
    
    vchar = (uint8)(events->par); 

    switch(events->sig)
    {
    case SIG_ISSUED_DATA:
        gizEventProcess(&info, (uint8_t *)issuedBuf);
        
        //clean event info
        memset((uint8_t *)&info, 0, sizeof(event_info_t)); 
        break;
    case SIG_PASSTHROUGH:
        for(i = 0; i < issuedLen; i++)
        {
            os_printf("%x ", issuedBuf[i]);
        }
        os_printf("\n");
        memset(issuedBuf, 0, 256);
        issuedLen = 0;
        break; 
    case SIG_HD11_READ:
        gokit_th_read_handle();
        break;
    case SIG_INFRARED_READ:
        gokit_ir_read_handle();
        break;
    case SIG_REPORT_JUDGMENT:
        gokit_report_judge();
        break;
    case SIG_SET_REPFLAG:
        gokit_set_rpflag();
        break;
    case SIG_CLE_REPFLAG:
        gokit_clean_rpflag();
        break;
    case SIG_IMM_REPORT:
        gokit_Immediate_report();
        break; 
    default:
        os_printf("---error sig! ---\n");
        break;
    }
}

