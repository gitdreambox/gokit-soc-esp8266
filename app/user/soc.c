#include "soc.h"
#include "lan.h"
#include "cloud.h"
#include "iof_arch.h"
#include "Hal_key.h"
#include "Hal_infrared.h"
#include "Hal_motor.h"

//uint32 SysCountTime;

soc_pcontext soc_context_Data = NULL; 

//uint8_t gaterSensorFlag;
//uint8_t Set_LedStatus = 0;
//uint8_t NetConfigureFlag = 0;
//uint32 Last_KeyTime = 0;
//uint8_t lastTem = 0, lastHum = 0;

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

/** @addtogroup GizWits_HW_Init
  * @{
  */
void ICACHE_FLASH_ATTR
HW_Init(void)
{
//  Delay_Init(72);
//  UARTx_Init();
//  RGB_KEY_GPIO_Init();
//  RGB_LED_Init();
//  LED_GPIO_Init();
    KEY_GPIO_Init();
//  TIM3_Int_Init(7199,9);   //ms interrupt
    motor_init(); 
//  DHT11_Init();
    ir_init();
}

/** @addtogroup GizWits_SW_Init
  * @{
  */
void ICACHE_FLASH_ATTR
SW_Init(soc_pcontext *pgc)
{
    *pgc = (soc_pcontext)os_malloc(sizeof(soc_context)); 
    while(NULL == *pgc)
    {
        *pgc = (soc_pcontext)os_malloc(sizeof(soc_context)); 
//        sleep(1);
    }
    os_memset(*pgc, 0, sizeof(soc_context)); 

    return; 
    
//  memset((uint8_t *)pgc, 0, sizeof(soc_context_t));
    
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
* Function Name  : KEY_Handle
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

    Key_return = ReadKeyValue(); 
    
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
//  		LED_RGB_Control(255, 0, 0);
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
//  		LED_RGB_Control(0, 128, 0);
//  		GizWits_D2WConfigCmd(AirLink_Mode);
//          NetConfigureFlag = 1;
        }
    }
}

void ICACHE_FLASH_ATTR
SOC_SENSORTEST(soc_pcontext pgc)
{
    pgc->SysCountTime ++;
    
    if(pgc->SysCountTime >= MaxSocTimout)  //KeyCountTime 1MS+1  °´¼üÏû¶¶10MS
    {
        pgc->SysCountTime = 0;

        /* Test LOG model */
//      GAgent_Printf(GAGENT_CRITICAL, "ST : %d",system_get_time());
        GAgent_Printf(GAGENT_CRITICAL, "InfIO : %d", GPIO_INPUT_GET(GPIO_ID_PIN(Infrared_GPIO_PIN)));
//      GAgent_Printf(GAGENT_CRITICAL, "key1 : %d", GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_KEY1_PIN)));
//      GAgent_Printf(GAGENT_CRITICAL, "key2 : %d", GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_KEY2_PIN)));

        static uint16 Mocou = 0;

        if(0 == Mocou)
        {
//          user_motor_set_duty(0, 0);
//          user_motor_set_duty(0, 1);
            motor_control(5);
            GAgent_Printf(GAGENT_CRITICAL, "MO : 0");
            Mocou++;
        }
        else if(1 == Mocou)
        {
//          user_motor_set_duty(9000, 0);
//          user_motor_set_duty(0, 1);
            motor_control(0);
            GAgent_Printf(GAGENT_CRITICAL, "MO : +");
            Mocou++;
        }
        else if(2 == Mocou)
        {
//          user_motor_set_duty(9000, 0);
//          user_motor_set_duty(9000, 1);
            motor_control(9);
            GAgent_Printf(GAGENT_CRITICAL, "MO : -");
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
SOC_Tick(soc_pcontext pgc)
{
    SOC_SENSORTEST(pgc);

    key_handle(); 
    ir_update_status();
}

