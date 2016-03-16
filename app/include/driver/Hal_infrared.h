#ifndef _HAL_INFRARED_H
#define _HAL_INFRARED_H
#include <stdio.h>
#include "soc.h"

/* Define your drive pin */
#define Infrared_GPIO_PIN       15

/* Set GPIO Direction */
#define GET_INF                 GPIO_INPUT_GET(GPIO_ID_PIN(Infrared_GPIO_PIN))

/* Function declaration */
void ir_init(void);
bool ir_update_status(void);

#endif /*_HAL_INFRARED_H*/


