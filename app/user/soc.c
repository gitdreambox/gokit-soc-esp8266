#include "soc.h"
#include "lan.h"
#include "cloud.h"
#include "iof_arch.h"
#include "driver/Hal_key.h"
#include "driver/Hal_infrared.h"
#include "driver/Hal_motor.h"
#include "driver/Hal_rgb_led.h"
#include "driver/Hal_temp_hum.h"

soc_pcontext soc_context_Data = NULL; 

/* SOC */

/**
* @brief gokit_time_ms
*
* @return uint32 ICACHE_FLASH_ATTR
*/
uint32 ICACHE_FLASH_ATTR
   gokit_time_ms(void)
{
    return (system_get_time() / 1000);
}

/**
* @brief gokit_time_s
*
* @return uint32 ICACHE_FLASH_ATTR
*/
uint32 ICACHE_FLASH_ATTR
   gokit_time_s(void)
{
    return (system_get_time() / 1000 / 1000);
}

/** @addtogroup soc_hw_init
  * @{
  */
void ICACHE_FLASH_ATTR
soc_hw_init(void)
{
    #ifdef RGBLED_ON
    rgb_gpio_init();
    rgb_led_init();
    #endif
    #ifdef KEY_ON
    key_gpio_init(); 
    #endif
    #ifdef MOTOR_ON
    motor_init(); 
    #endif
    #ifdef TEMPHUM_ON
    uint8 ret; 
    ret = dh11_init();
    GAgent_Printf(GAGENT_INFO, "dh11_init : %d ", ret); 
    #endif
    #ifdef INFRARED_ON
    ir_init();
    #endif
}

/** @addtogroup soc_sw_init
  * @{
  */
void ICACHE_FLASH_ATTR
soc_sw_init(soc_pcontext * pgc)
{
    *pgc = (soc_pcontext)os_malloc(sizeof(soc_context)); 
    while(NULL == *pgc)
    {
        *pgc = (soc_pcontext)os_malloc(sizeof(soc_context)); 
//        sleep(1);
    }
    os_memset(*pgc, 0, sizeof(soc_context)); 

    return; 
    
//  ReadTypeDef.Alert = 0;
//  ReadTypeDef.LED_Cmd = 0;
//  ReadTypeDef.LED_R = 0;
//  ReadTypeDef.LED_G = 0;
//  ReadTypeDef.LED_B = 0;
//  ReadTypeDef.Motor = 5;
//  ReadTypeDef.Infrared = 0;
//  ReadTypeDef.Temperature = 0;
//  ReadTypeDef.Humidity = 0;
//  ReadTypeDef.Alert = 0;
//  ReadTypeDef.Fault = 0;
//  GizWits_init(sizeof(ReadTypeDef_t));
//  printf("Gokit Init Ok ...\r\n");
}


/*******************************************************************************
* Function Name  : key_handle
* Description    : Key processing function
* Input          : None
* Output         : None
* Return         : None
* Attention		 	 : None
*******************************************************************************/
void ICACHE_FLASH_ATTR
key_handle(void)
{
    uint8_t Key_return = 0;

    Key_return = key_state_read(); 
    
    if(Key_return & KEY_UP)
    {
        if(Key_return & PRESS_KEY1)
        {
            #ifdef PROTOCOL_DEBUG
            GAgent_Printf(GAGENT_CRITICAL, "KEY1 PRESS\r\n");
            #endif
        }
        if(Key_return & PRESS_KEY2)
        {
            #ifdef PROTOCOL_DEBUG
            GAgent_Printf(GAGENT_CRITICAL, "KEY2 PRESS ,Soft AP mode\r\n");
            #endif
            //Soft AP mode, RGB red
//  		rgb_control(255, 0, 0);
//  		GizWits_D2WConfigCmd(SoftAp_Mode);
//          NetConfigureFlag = 1;
        }
    }

    if(Key_return & KEY_LONG)
    {
        if(Key_return & PRESS_KEY1)
        {
            #ifdef PROTOCOL_DEBUG
            GAgent_Printf(GAGENT_CRITICAL, "KEY1 PRESS LONG ,Wifi Reset\r\n");
            #endif
//  		GizWits_D2WResetCmd();
        }
        if(Key_return & PRESS_KEY2)
        {
            //AirLink mode, RGB Green
            #ifdef PROTOCOL_DEBUG
            GAgent_Printf(GAGENT_CRITICAL, "KEY2 PRESS LONG ,AirLink mode\r\n");
            #endif
//  		rgb_control(0, 128, 0);
//  		GizWits_D2WConfigCmd(AirLink_Mode);
//          NetConfigureFlag = 1;
        }
    }
}

void ICACHE_FLASH_ATTR
soc_sensortest(soc_pcontext pgc)
{
    pgc->SysCountTime ++;
    
    if(pgc->SysCountTime >= MaxSocTimout) 
    {
        static uint16 Mocou = 0; 
        
        pgc->SysCountTime = 0;

        /* Test LOG model */
//      GAgent_Printf(GAGENT_CRITICAL, "ST : %d",system_get_time());
        
        #ifdef TEMPHUM_ON
        uint8_t curTem = 0, curHum = 0;
        hdt11_read_data(&curTem, &curHum); 
        GAgent_Printf(GAGENT_CRITICAL, "Temperature : %d , Humidity : %d", curTem, curHum); 
        #endif
        
        #ifdef INFRARED_ON
        GAgent_Printf(GAGENT_CRITICAL, "InfIO : %d", ir_update_status()); 
        #endif
        
        #ifdef KEY_ON
        GAgent_Printf(GAGENT_CRITICAL, "key1 : %d", GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_KEY1_PIN)));
        GAgent_Printf(GAGENT_CRITICAL, "key2 : %d", GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_KEY2_PIN)));
        #endif
        
        #ifdef MOTOR_ON
        if(0 == Mocou)
        {
            motor_control(5);
            GAgent_Printf(GAGENT_CRITICAL, "MO : 0");
        }
        else if(1 == Mocou)
        {
            motor_control(0);
            GAgent_Printf(GAGENT_CRITICAL, "MO : +");
        }
        else if(2 == Mocou)
        {
            motor_control(9);
            GAgent_Printf(GAGENT_CRITICAL, "MO : -");
        }
        #endif
        
        #ifdef RGBLED_ON
        if(0 == Mocou)
        {
            rgb_control(0, 0, 250);
            GAgent_Printf(GAGENT_CRITICAL, "RGB : B"); 
        }
        else if(1 == Mocou)
        {
            rgb_control(0, 250, 0); 
            GAgent_Printf(GAGENT_CRITICAL, "RGB : G");
        }
        else if(2 == Mocou)
        {
            rgb_control(250, 0, 0);
            GAgent_Printf(GAGENT_CRITICAL, "RGB : R"); 
        }
        #endif
        
        if(0 == Mocou)
        {
            Mocou++;
        }
        else if(1 == Mocou)
        {
            Mocou++;
        }
        else if(2 == Mocou)
        {
            Mocou = 0;
        }
    }
    
}

/**
* @brief SOC_Tick
*
* @param pgc
*
* @return void ICACHE_FLASH_ATTR
*/
void ICACHE_FLASH_ATTR
soc_tick(soc_pcontext pgc)
{
//  soc_sensortest(pgc);

    #ifdef KEY_ON
    key_handle();
    #endif
     
}

