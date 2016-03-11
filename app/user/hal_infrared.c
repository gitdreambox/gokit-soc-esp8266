/**
********************************************************
*
* @file      Hal_infrared.c
* @author    Gizwtis
* @version   V2.3
* @date      2015-07-06
*
* @brief     ������ ֻΪ����Ӳ������
*            Gizwits Smart Cloud  for Smart Products
*            ����|��ֵ|����|����|��ȫ|����|����|��̬
*            www.gizwits.com
*
*********************************************************/

#include "Hal_infrared.h"

void ICACHE_FLASH_ATTR
ir_init(void)
{   
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15); 
    gpio_output_set(0, 0, 0, GPIO_ID_PIN(Infrared_GPIO_PIN)); 
}

bool ICACHE_FLASH_ATTR
ir_update_status(void)
{
    if(GPIO_INPUT_GET(GPIO_ID_PIN(Infrared_GPIO_PIN)))
    {
//      GAgent_Printf(GAGENT_CRITICAL, "IR OFF");
        
        return 0;
    }
    else
    {
//      GAgent_Printf(GAGENT_CRITICAL, "IR ON");
        
        return 1;
    }
}
