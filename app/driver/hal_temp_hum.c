
/*********************************************************
*
* @file      hal_temp_hum.c
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

#include "driver/hal_temp_hum.h"
#include "osapi.h"

th_typedef_t temphum_typedef;

static void ICACHE_FLASH_ATTR temp_hum_delay(unsigned int us)
{
    /* Define your delay function */

    os_delay_us(us);
}

//Reset DHT11
static void ICACHE_FLASH_ATTR hdt11_rst(void)
{
    DHT11_IO_OUT;                                               //SET OUTPUT
    DHT11_OUT_LOW;                                              //GPIOA.0=0
    temp_hum_delay(18*1000);                                    //Pull down Least 18ms
    DHT11_OUT_HIGH;                                             //GPIOA.0=1
}

static u8 ICACHE_FLASH_ATTR hdt11_check(void)
{
    u8 retry=0;
    DHT11_IO_IN;                                                //SET INPUT
    while (DHT11_IN&&retry<100)                                 //DHT11 Pull down 40~80us
    {
        retry++;
        temp_hum_delay(1);
    }

    if(retry>=100)
        return 1;
    else
        retry=0;

    while (!DHT11_IN&&retry<100)                                //DHT11 Pull up 40~80us
    {
        retry++;
        temp_hum_delay(1);
    }

    if(retry>=100)
        return 1;                                               //chack error

    return 0;
}

static u8 ICACHE_FLASH_ATTR hdt11_read_bit(void)
{
    u8 retry=0;
    while(DHT11_IN&&retry<100)                                  //wait become Low level
    {
        retry++;
        temp_hum_delay(1);
    }

    retry=0;
    while(!DHT11_IN&&retry<100)                                 //wait become High level
    {
        retry++;
        temp_hum_delay(1);
    }

    temp_hum_delay(40);                                         //wait 40us

    if(DHT11_IN)
        return 1;
    else
        return 0;
}

static u8 ICACHE_FLASH_ATTR hdt11_read_byte(void)
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

static u8 ICACHE_FLASH_ATTR dht11_read_data(u8 * temperature, u8 * humidity)
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

uint8_t ICACHE_FLASH_ATTR dh11_read(uint8_t * temperature, uint8_t * humidity)
{
    uint8_t curTem = 0, curHum = 0;
    uint16_t temMeans = 0, hum_means = 0;
    uint8_t cur_i = 0;
    uint8_t ret = 0; 

    ret = dht11_read_data(&curTem, &curHum);

    if(1 != ret) 
    {
        //Cycle store ten times stronghold
        if(MEAN_NUM > temphum_typedef.th_num)
        {
            temphum_typedef.th_bufs[temphum_typedef.th_num][0] = curTem;
            temphum_typedef.th_bufs[temphum_typedef.th_num][1] = curHum;

            temphum_typedef.th_num++;
        }
        else
        {
            temphum_typedef.th_num = 0;

            temphum_typedef.th_bufs[temphum_typedef.th_num][0] = curTem;
            temphum_typedef.th_bufs[temphum_typedef.th_num][1] = curHum;

            temphum_typedef.th_num++;
        }
    }
    else
    {
        return (1); 
    }
    
    if(MEAN_NUM <= temphum_typedef.th_num) 
    {
        temphum_typedef.th_amount = MEAN_NUM;
    }

    if(0 == temphum_typedef.th_amount) 
    {
        //Calculate Before ten the mean
        for(cur_i = 0; cur_i < temphum_typedef.th_num; cur_i++)
        {
            temMeans += temphum_typedef.th_bufs[cur_i][0];
            hum_means += temphum_typedef.th_bufs[cur_i][1];
        }

        temMeans = temMeans / temphum_typedef.th_num;
        hum_means = hum_means / temphum_typedef.th_num; 
        
        *temperature = temMeans;
        *humidity = hum_means;
    }
    else if(MEAN_NUM == temphum_typedef.th_amount) 
    {
        //Calculate After ten times the mean
        for(cur_i = 0; cur_i < temphum_typedef.th_amount; cur_i++) 
        {
            temMeans += temphum_typedef.th_bufs[cur_i][0];
            hum_means += temphum_typedef.th_bufs[cur_i][1];
        }

        temMeans = temMeans / temphum_typedef.th_amount; 
        hum_means = hum_means / temphum_typedef.th_amount; 
        
        *temperature = (uint8_t)temMeans; 
        *humidity = (uint8_t)hum_means; 
    }

    return (0);
}

u8 ICACHE_FLASH_ATTR dh11_init(void)
{
    /* Migrate your driver code */
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);

    hdt11_rst(); 
    
    os_memset((uint8_t *)&temphum_typedef, 0, sizeof(th_typedef_t)); 
    
    os_printf("dh11_init \r\n"); 
    
    return hdt11_check(); 
}

void ICACHE_FLASH_ATTR dh11_sensortest(void)
{
    /* Test LOG model */

    uint8_t curTem = 0, curHum = 0; 
    
    dht11_read_data(&curTem, &curHum); 
    
    os_printf("Temperature : %d , Humidity : %d", curTem, curHum); 
}
