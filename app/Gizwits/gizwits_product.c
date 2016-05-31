
#include <stdio.h>
#include <string.h>
#include "gizwits_product.h"
#include "driver/hal_key.h"
#include "driver/hal_infrared.h"
#include "driver/hal_motor.h"
#include "driver/hal_rgb_led.h"
#include "driver/hal_temp_hum.h"

extern volatile uint8_t reportBuf[256]; 
extern uint8_t gizConfigFlag; 

void ICACHE_FLASH_ATTR gizEventProcess(event_info_t *info, uint8_t *data)
{
    uint8_t i = 0;
    uint8_t rssi = *data;
    gizwits_issued_t *issued = (gizwits_issued_t *)data;
    gizwits_report_t * reportData = (gizwits_report_t *)&reportBuf; 
    static uint8_t gokit_led_status = 0; 

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
                os_printf("connected router\n");
                
                if(gizConfigFlag == 1)
                {
                    os_printf( "@@@@ W2M->WIFI_CONNROUTER \n"); 

                    gizConfigFlag = 0;
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
                reportData->dev_status.motor = issued->attr_vals.motor_speed;
                
                motor_control((_MOTOR_T)issued->attr_vals.motor_speed); 
                break;
            default:
                break;
        }
    }
    
    if(1 == gokit_led_status) 
    {
        //Immediately reported data
        gizImmediateReport(); 
    }
}
