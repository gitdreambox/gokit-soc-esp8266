
/*********************************************************
*
* @file      hal_infrared.c
* @author    Gizwtis
* @version   V3.0
* @date      2016-03-09
*
* @brief     机智云 只为智能硬件而生
*            Gizwits Smart Cloud  for Smart Products
*            链接|增值|开放|中立|安全|自有|自由|生态
*            www.gizwits.com
*
*********************************************************/

#include "driver/hal_infrared.h"
#include "driver/gpio16.h"
#include "gagent.h"

bool ICACHE_FLASH_ATTR ir_update_status(void)
{
    //if(GET_INF)
    if(gpio16_input_get())
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

void ICACHE_FLASH_ATTR ir_init(void)
{
    /* Migrate your driver code */

    gpio16_input_conf();
    
    GAgent_Printf(GAGENT_DEBUG, "ir_init \r\n"); 
}

void ir_sensortest(void)
{
        /* Test LOG model */

        GAgent_Printf(GAGENT_CRITICAL, "InfIO : %d", ir_update_status());
}
