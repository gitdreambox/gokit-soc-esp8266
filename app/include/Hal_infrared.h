#ifndef _HAL_INFRARED_H
#define _HAL_INFRARED_H
#include <stdio.h>
#include "soc.h"

#define Infrared_GPIO_PIN          15

void ir_init(void);
bool ir_update_status(void);

#endif /*_HAL_INFRARED_H*/


