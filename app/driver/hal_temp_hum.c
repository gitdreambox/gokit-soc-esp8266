/**
********************************************************
*
* @file      Hal_temp_hum.c
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
#include "driver/hal_temp_hum.h"
#include "osapi.h"

static void temp_hum_delay(unsigned int us)
{
    /* Define your delay function */

    os_delay_us(us);
}

//Reset DHT11
static void hdt11_rst(void)
{
    DHT11_IO_OUT; 											    //SET OUTPUT
    DHT11_OUT_LOW;                                              //GPIOA.0=0
    temp_hum_delay(18*1000);                                    //Pull down Least 18ms
    DHT11_OUT_HIGH;                                             //GPIOA.0=1
}

static u8 hdt11_check(void)
{
    u8 retry=0;
    DHT11_IO_IN;												//SET INPUT
    while (DHT11_IN&&retry<100)				                    //DHT11 Pull down 40~80us
    {
        retry++;
        temp_hum_delay(1);
    }

    if(retry>=100)
        return 1;
    else
        retry=0;

    while (!DHT11_IN&&retry<100)				                //DHT11 Pull up 40~80us
    {
        retry++;
        temp_hum_delay(1);
    }

    if(retry>=100)
        return 1;												//chack error

    return 0;
}

static u8 hdt11_read_bit(void)
{
    u8 retry=0;
    while(DHT11_IN&&retry<100)					                //wait become Low level
    {
        retry++;
        temp_hum_delay(1);
    }

    retry=0;
    while(!DHT11_IN&&retry<100)				                    //wait become High level
    {
        retry++;
        temp_hum_delay(1);
    }

    temp_hum_delay(40);//wait 40us

    if(DHT11_IN)
        return 1;
    else
        return 0;
}

static u8 hdt11_read_byte(void)
{
    u8 i,dat;
    dat=0;
    for (i=0; i<8; i++)
    {
        dat<<=1;
        dat |= hdt11_read_bit(); 
    }

    return dat;
}

u8 hdt11_read_data(u8 * temperature, u8 * humidity)
{
    u8 buf[5];
    u8 i;
    hdt11_rst(); 
    if(hdt11_check() == 0) 
    {
        for(i=0; i<5; i++)
        {
            buf[i] = hdt11_read_byte(); 
        }
        if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])
        {
            *humidity=buf[0];
            *temperature=buf[2];
        }
    }
    else
        return 1;

    return 0;
}

u8 dh11_init(void)
{
    /* Migrate your driver code */
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);

    hdt11_rst(); 
    return hdt11_check(); 
}
