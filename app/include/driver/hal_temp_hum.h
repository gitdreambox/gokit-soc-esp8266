#ifndef _HAL_HEMP_HUM_H
#define _HAL_HEMP_HUM_H

#include <stdio.h>
#include "soc.h"

/* Define your drive pin */
#define DHT11_GPIO_PIN      12

/* Set GPIO Direction */
#define DHT11_IO_IN         GPIO_DIS_OUTPUT(GPIO_ID_PIN(DHT11_GPIO_PIN))// gpio_output_set(0, 0, 0, GPIO_ID_PIN(DHT11_GPIO_PIN))������

#define DHT11_IO_OUT        gpio_output_set(0, 0, GPIO_ID_PIN(DHT11_GPIO_PIN), 0)
	
#define	DHT11_OUT_HIGH      GPIO_OUTPUT_SET(GPIO_ID_PIN(DHT11_GPIO_PIN), 1)
#define	DHT11_OUT_LOW       GPIO_OUTPUT_SET(GPIO_ID_PIN(DHT11_GPIO_PIN), 0)

#define	DHT11_IN            GPIO_INPUT_GET(GPIO_ID_PIN(DHT11_GPIO_PIN))

/* Function declaration */
u8 dh11_init(void); //Init DHT11
u8 hdt11_read_data(u8 *temperature,u8 *humidity); //Read DHT11 Value

#endif /*_HAL_HEMP_HUM_H*/
