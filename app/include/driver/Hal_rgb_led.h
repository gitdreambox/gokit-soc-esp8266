#ifndef _HAL_RGB_LED_H
#define _HAL_RGB_LED_H

#include <stdio.h>
#include "soc.h"

/* Define your drive pin */
#define GPIO_RGB_SCL            4
#define GPIO_RGB_SDA            14
#define GPIO_RGB_POW            15

/* Set GPIO Direction */
#define SCL_LOW 	            GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_RGB_SCL), 0)
#define SCL_HIGH 	            GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_RGB_SCL), 1)

#define SDA_LOW		            GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_RGB_SDA), 0)
#define SDA_HIGH	            GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_RGB_SDA), 1)

#define POW_HIGH	            GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_RGB_POW), 1)


#define R_MAX                   255
#define G_MAX                   255
#define B_MAX                   255

/* Function declaration */
void rgb_control(uint8_t R, uint8_t B, uint8_t G); 
void rgb_led_init(void); 
void rgb_gpio_init(void); 

#endif /*_HAL_RGB_LED_H*/

