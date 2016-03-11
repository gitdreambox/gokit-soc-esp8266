/**
********************************************************
*
* @file      Hal_rgb_led.c
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
#include "Hal_rgb_led.h"

void RGB_GPIO_Init(void)
{
    // SCL/SDA
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
    
//  // power con
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15); 
    //contron power
    GPIO_OUTPUT_SET(GPIO_ID_PIN(15), 1); 
    
}

void LED_delay(unsigned int  ms)
{
    volatile unsigned  int i=0;
    for(i=0; i<ms; i++);

}


/************ generation clock *********************/
void ClkProduce(void)
{
    SCL_LOW;    // SCL=0
    LED_delay(40);
    SCL_HIGH;     // SCL=1
    LED_delay(40);
}


/**********  send 32 zero ********************/
void Send32Zero(void)
{
    unsigned char i;
    SDA_LOW;   // SDA=0
    for (i=0; i<32; i++)
        ClkProduce();
}


/********* invert the grey value of the first two bits ***************/
uint8_t TakeAntiCode(uint8_t dat)
{
    uint8_t tmp = 0;

    tmp=((~dat) & 0xC0)>>6;
    return tmp;
}


/****** send gray data *********/
void DatSend(uint32 dx)
{
    uint8_t i;

    for (i=0; i<32; i++)
    {
        if ((dx & 0x80000000) != 0)
        {

            SDA_HIGH;     //  SDA=1;
        }
        else
        {
            SDA_LOW;    //  SDA=0;
        }

        dx <<= 1;
        ClkProduce();
    }
}

/******* data processing  ********************/
void DataDealWithAndSend(uint8_t r, uint8_t g, uint8_t b)
{
    uint32 dx = 0; 

    dx |= (uint32)0x03 << 30;             // The front of the two bits 1 is flag bits
    dx |= (uint32)TakeAntiCode(b) << 28; 
    dx |= (uint32)TakeAntiCode(g) << 26; 
    dx |= (uint32)TakeAntiCode(r) << 24; 

    dx |= (uint32)b << 16; 
    dx |= (uint32)g << 8; 
    dx |= r;

    DatSend(dx);
}

void RGB_LED_Init(void)
{
    Send32Zero();
    DataDealWithAndSend(0,0,0);	  // display red
    DataDealWithAndSend(0,0,0);

}

void LED_RGB_Control(uint8_t R, uint8_t G, uint8_t B)
{
    //contron power
    GPIO_OUTPUT_SET(GPIO_ID_PIN(15), 1); 
    
    Send32Zero();
    DataDealWithAndSend(R, G, B);	  // display red
    DataDealWithAndSend(R, G, B);	  // display red
}



