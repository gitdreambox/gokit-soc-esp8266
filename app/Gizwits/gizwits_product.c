
#include <stdio.h>
#include <string.h>
#include "gizwits_product.h"
#include "driver/hal_key.h"
#include "driver/hal_infrared.h"
#include "driver/hal_motor.h"
#include "driver/hal_rgb_led.h"
#include "driver/hal_temp_hum.h"

gizwits_report_t reportData;

void ICACHE_FLASH_ATTR gizEventProcess(event_info_t *info, uint8_t *data, uint32_t len)
{
    uint8_t i = 0;
    uint8_t rssi = *data;
    gizwits_issued_t *issued = (gizwits_issued_t *)data;
    int16_t valueMotor = 0; 
    uint8_t valueR = 0; 
    uint8_t valueG = 0; 
    uint8_t valueB = 0; 

    if((NULL == info) || (NULL == data)) 
    {
        os_printf("!!! gizEventProcess Error \n"); 

        return ;
    }

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

                rgbControl(0, 0, 0);
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
                    os_printf("########## setled_Off \r\n"); 
                    
                    rgbControl(0, 0, 0);

                    reportData.dev_status.led_onoff = LED_Off; 
                }
                if(issued->attr_vals.led_onoff == LED_On) 
                {
                    os_printf("########## setled_On \r\n"); 
                    
                    rgbControl(254, 0, 0);

                    reportData.dev_status.led_onoff = LED_On; 
                }
                break;
            case SetLED_Color:
                os_printf("########## led_color is %d\n", issued->attr_vals.led_color); 
                if(issued->attr_vals.led_color == LED_Costom) 
                {
                    rgbControl(
                       X2Y(LED_R_RATIO, LED_R_ADDITION, reportData.dev_status.led_r),
                       X2Y(LED_G_RATIO, LED_G_ADDITION, reportData.dev_status.led_g),
                       X2Y(LED_B_RATIO, LED_B_ADDITION, reportData.dev_status.led_b)); 
                    
                    reportData.dev_status.led_color = LED_Costom; 
                }
                if(issued->attr_vals.led_color == LED_Yellow) 
                {
                    os_printf("########## SetLED LED_Yellow \r\n"); 

                    rgbControl(254, 254, 0);

                    reportData.dev_status.led_color = LED_Yellow; 
                }
                if(issued->attr_vals.led_color == LED_Purple) 
                {
                    os_printf("########## SetLED LED_Purple \r\n"); 

                    rgbControl(254, 0, 70);

                    reportData.dev_status.led_color = LED_Purple; 
                }
                if(issued->attr_vals.led_color == LED_Pink) 
                {
                    os_printf("########## SetLED LED_Pink \r\n"); 
                    
                    rgbControl(238, 30, 30); 

                    reportData.dev_status.led_color = LED_Pink; 
                }
                break;
            case SetLED_R:
                os_printf("########## led_r is %d\n", issued->attr_vals.led_r); 

                valueR = X2Y(LED_R_RATIO, LED_R_ADDITION, issued->attr_vals.led_r); 
                
                rgbControl(
                   valueR,
                   X2Y(LED_G_RATIO, LED_G_ADDITION, reportData.dev_status.led_g),
                   X2Y(LED_B_RATIO, LED_B_ADDITION, reportData.dev_status.led_b)); 
                
                reportData.dev_status.led_r = issued->attr_vals.led_r; 
                break;
            case SetLED_G:
                os_printf("########## led_g is %d\n", issued->attr_vals.led_g); 
                
                valueG = X2Y(LED_G_RATIO, LED_G_ADDITION, issued->attr_vals.led_g); 
                
                rgbControl(
                   X2Y(LED_R_RATIO, LED_R_ADDITION, reportData.dev_status.led_r),
                   valueG,
                   X2Y(LED_B_RATIO, LED_B_ADDITION, reportData.dev_status.led_b)); 
                
                reportData.dev_status.led_g = issued->attr_vals.led_g; 
                break;
            case SetLED_B:
                os_printf("########## led_b is %d\n", issued->attr_vals.led_b); 
                
                valueB = X2Y(LED_B_RATIO, LED_B_ADDITION, issued->attr_vals.led_b); 
                
                rgbControl(
                   X2Y(LED_R_RATIO, LED_R_ADDITION, reportData.dev_status.led_r),
                   X2Y(LED_G_RATIO, LED_G_ADDITION, reportData.dev_status.led_g),
                   valueB); 
                
                reportData.dev_status.led_b = issued->attr_vals.led_b; 
                break;
            case SetMotor:
                os_printf("########## motor speed is %d\n", issued->attr_vals.motor); 
                
                valueMotor = X2Y(MOTOR_SPEED_RATIO, MOTOR_SPEED_ADDITION, exchangeBytes(issued->attr_vals.motor)); 
                
                motorControl(valueMotor); 
                
                reportData.dev_status.motor = issued->attr_vals.motor; 
                break;
            default:
                break;
        }
    }

    gizReportData(ACTION_REPORT_DEV_STATUS, (uint8_t *)&reportData, sizeof(gizwits_report_t)); 
}
