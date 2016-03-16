/**
********************************************************
*
* @file      Hal_infrared.c
* @author    Gizwtis
* @version   V2.3
* @date      2015-07-06
*
* @brief     机智云 只为智能硬件而生
*            Gizwits Smart Cloud  for Smart Products
*            链接|增值|开放|中立|安全|自有|自由|生态
*            www.gizwits.com
*
*********************************************************/

#include "driver/Hal_infrared.h"

bool ICACHE_FLASH_ATTR
ir_update_status(void)
{
    if(GET_INF) 
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

void ICACHE_FLASH_ATTR
ir_init(void)
{
    /* Migrate your driver code */

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
    gpio_output_set(0, 0, 0, GPIO_ID_PIN(Infrared_GPIO_PIN));
}

