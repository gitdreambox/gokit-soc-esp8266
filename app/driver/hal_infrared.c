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

#include "driver/hal_infrared.h"
#include "driver/gpio16.h"

bool ICACHE_FLASH_ATTR
ir_update_status(void)
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

void ICACHE_FLASH_ATTR
ir_init(void)
{
    /* Migrate your driver code */

    gpio16_input_conf();
    //PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
    //GPIO_DIS_OUTPUT(GPIO_ID_PIN(Infrared_GPIO_PIN));
    //gpio_output_set(0, 0, 0, GPIO_ID_PIN(Infrared_GPIO_PIN));
}

