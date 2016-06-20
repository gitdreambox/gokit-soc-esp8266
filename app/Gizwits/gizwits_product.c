
#include <stdio.h>
#include <string.h>
#include "gizwits_product.h"

void ICACHE_FLASH_ATTR gizEventProcess(event_info_t *info, uint8_t *data, uint32_t len)
{
    uint8_t i = 0;
    uint8_t rssi = *data;
    gizwits_issued_t *issued = (gizwits_issued_t *)data;
    gizwits_report_t reportData;

    if((NULL == info) || (NULL == data))
    {
        os_printf("!!! gizEventProcess Error \n");

        return ;
    }

    os_memcpy((uint8_t *)&reportData, (uint8_t *)&gizwitsProtocol.lastReportData, sizeof(gizwits_report_t));

    for(i=0; i<info->num; i++)
    {
        switch(info->event[i])
        {
            case WIFI_SOFTAP:
                break;
            case WIFI_AIRLINK:
                break;
            case WIFI_CON_ROUTER:
                os_printf( "@@@@ W2M->WIFI_CONNROUTER \n");
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
            case TRANSPARENT_DATA:
                os_printf("transparent buf len %d\n", len);
                break;

            //coustm
            case SetLED_OnOff:
                os_printf("########## led_onoff is %d\n", issued->attr_vals.led_onoff); 
                if(issued->attr_vals.led_onoff == LED_Off) 
                {
                    reportData.dev_status.led_onoff = LED_Off; 

                    os_printf("########## setled_Off \r\n"); 
                }
                if(issued->attr_vals.led_onoff == LED_On) 
                {
                    reportData.dev_status.led_onoff = LED_On; 

                    os_printf("########## setled_On \r\n"); 
                }
                break;
            case SetLED_Color:
                os_printf("########## led_color is %d\n", issued->attr_vals.led_color); 
                if(issued->attr_vals.led_color == LED_Costom) 
                {
                    reportData.dev_status.led_color = LED_Costom;

                }
                if(issued->attr_vals.led_color == LED_Yellow) 
                {
                    reportData.dev_status.led_color = LED_Yellow; 

                    os_printf("########## SetLED LED_Yellow \r\n"); 
                }
                if(issued->attr_vals.led_color == LED_Purple) 
                {
                    reportData.dev_status.led_color = LED_Purple; 

                    os_printf("########## SetLED LED_Purple \r\n"); 
                }
                if(issued->attr_vals.led_color == LED_Pink) 
                {
                    reportData.dev_status.led_color = LED_Pink; 

                    os_printf("########## SetLED LED_Pink \r\n"); 
                }
                break;
            case SetLED_R:
                os_printf("########## led_r is %d\n", issued->attr_vals.led_r); 
                reportData.dev_status.led_r = issued->attr_vals.led_r; 

                break;
            case SetLED_G:
                os_printf("########## led_g is %d\n", issued->attr_vals.led_g); 
                reportData.dev_status.led_g = issued->attr_vals.led_g; 

                break;
            case SetLED_B:
                os_printf("########## led_b is %d\n", issued->attr_vals.led_b); 
                reportData.dev_status.led_b = issued->attr_vals.led_b; 

                break;
            case SetMotor:
                os_printf("########## motor speed is %d\n", issued->attr_vals.motor_speed); 
                reportData.dev_status.motor = issued->attr_vals.motor_speed;
                break;
            default:
                break;
        }
    }

    gizReportData(ACTION_REPORT_DEV_STATUS, (uint8_t *)&reportData, sizeof(gizwits_report_t)); 
}
