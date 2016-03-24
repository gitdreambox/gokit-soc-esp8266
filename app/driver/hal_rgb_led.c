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
#include "driver/hal_rgb_led.h"

static void rgb_delay(unsigned int  us)
{
    /* Define your delay function */
    
    volatile unsigned  int i=0;
    for(i=0; i<us; i++);
}


/************ generation clock *********************/
static void ClkProduce(void)
{
    SCL_LOW;    // SCL=0
    rgb_delay(40); 
    SCL_HIGH;     // SCL=1
    rgb_delay(40); 
}


/**********  send 32 zero ********************/
static void send_32zero(void)
{
    unsigned char i;
    SDA_LOW;   // SDA=0
    for (i=0; i<32; i++)
        ClkProduce();
}


/********* invert the grey value of the first two bits ***************/
static uint8_t take_anti_code(uint8_t dat)
{
    uint8_t tmp = 0;

    tmp=((~dat) & 0xC0)>>6;
    return tmp;
}


/****** send gray data *********/
static void data_send(uint32 dx)
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
static void data_dealwith_send(uint8_t r, uint8_t g, uint8_t b)
{
    uint32 dx = 0; 

    dx |= (uint32)0x03 << 30;             // The front of the two bits 1 is flag bits
    dx |= (uint32)take_anti_code(b) << 28; 
    dx |= (uint32)take_anti_code(g) << 26; 
    dx |= (uint32)take_anti_code(r) << 24; 

    dx |= (uint32)b << 16; 
    dx |= (uint32)g << 8; 
    dx |= r;

    data_send(dx);
}


void rgb_control(uint8_t R, uint8_t G, uint8_t B)
{
    //contron power
//  POW_HIGH;
    
    send_32zero();
    data_dealwith_send(R, G, B);	  // display red
    data_dealwith_send(R, G, B);	  // display red
}


void rgb_led_init(void)
{
    //contron power
//  POW_HIGH;

    send_32zero();
    data_dealwith_send(0, 0, 0);   // display red
    data_dealwith_send(0, 0, 0);
}


void rgb_gpio_init(void)
{
    /* Migrate your driver code */
    
    // power con
//  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);

    // SCL/SDA
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15); 

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4); 

    gpio_output_set(0, 0, GPIO_ID_PIN(GPIO_RGB_SCL) | GPIO_ID_PIN(GPIO_RGB_SDA), 0); //| GPIO_ID_PIN(GPIO_RGB_POW)

}


