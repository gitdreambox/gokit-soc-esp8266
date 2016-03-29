#ifndef _HAL_HEMP_HUM_H
#define _HAL_HEMP_HUM_H

#include <stdio.h>
#include <c_types.h>
#include <gpio.h>
#include <eagle_soc.h>

/* Define your drive pin */
#define DHT11_GPIO_PIN      5

/* Set GPIO Direction */
#define DHT11_IO_IN         GPIO_DIS_OUTPUT(GPIO_ID_PIN(DHT11_GPIO_PIN))// gpio_output_set(0, 0, 0, GPIO_ID_PIN(DHT11_GPIO_PIN))≤ªø…”√

#define DHT11_IO_OUT        gpio_output_set(0, 0, GPIO_ID_PIN(DHT11_GPIO_PIN), 0)

#define	DHT11_OUT_HIGH      GPIO_OUTPUT_SET(GPIO_ID_PIN(DHT11_GPIO_PIN), 1)
#define	DHT11_OUT_LOW       GPIO_OUTPUT_SET(GPIO_ID_PIN(DHT11_GPIO_PIN), 0)

#define	DHT11_IN            GPIO_INPUT_GET(GPIO_ID_PIN(DHT11_GPIO_PIN))

/*****************************************************
* P0 command √¸¡Ó¬Î
******************************************************/
typedef struct
{
    uint8_t pre_tem_means_val;
    uint8_t pre_hum_means_val;
    uint8_t th_num;
    uint8_t th_bufs[10][2];
}th_typedef_t; 

/* Function declaration */
u8 dh11_init(void); //Init DHT11
u8 dht11_read_data(u8 * temperature, u8 * humidity); //Read DHT11 Value
void dh11_sensortest(void);

#endif /*_HAL_HEMP_HUM_H*/

