#ifndef _HAL_KEY_H_
#define _HAL_KEY_H_

#include <stdio.h>
#include "soc.h"

/* Define your drive pin */
#define GPIO_KEY1_PIN           14
#define GPIO_KEY2_PIN           0

/* Set GPIO Direction */
#define GET_KEY1 	            GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_KEY1_PIN))
#define GET_KEY2 	            GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_KEY2_PIN))


#define PRESS_KEY1              0x01
#define PRESS_KEY2              0x02
#define PRESS_KEY3              0x04

#define NO_KEY                  0x00
#define KEY_DOWN                0x10
#define KEY_UP    	            0x20
#define KEY_LIAN                0x40
#define KEY_LONG                0x80

#define KEY_CLICK_DELAY         10

#define KEY1_Long_Action        0x01         
#define KEY2_Long_Action        0x02

/* Function declaration */
void key_gpio_init(void); 
uint8_t key_value_read(void); 
uint8_t key_state_read(void); 

#endif /*_HAL_KEY_H*/

