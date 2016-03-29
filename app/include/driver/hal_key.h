#ifndef _HAL_KEY_H_
#define _HAL_KEY_H_

#include <stdio.h>
#include <c_types.h>
#include <gpio.h>
#include "os_type.h"
#include "osapi.h"

#define PRESS_KEY1                              0x01
#define PRESS_KEY2                              0x02
#define PRESS_KEY3                              0x04

#define NO_KEY                                  0x00
#define KEY_DOWN                                0x10
#define KEY_UP                                  0x20
#define KEY_LIAN                                0x40
#define KEY_LONG                                0x80

typedef void (*gokit_key_function)(void);

typedef struct
{
    uint8 gpio_id;
    uint8 gpio_func;
    uint32 gpio_name;
    gokit_key_function short_press; 
    gokit_key_function long_press; 
}key_typedef_t; 

typedef struct
{
    uint8 key_num;
    os_timer_t key_10ms;
    uint8 key_timer_delay; 
    key_typedef_t ** single_key; 
}keys_typedef_t; 

/* Function declaration */

void gokit_key_handle(keys_typedef_t * keys); 
key_typedef_t * key_init_one(uint8 gpio_id, uint32 gpio_name, uint8 gpio_func, gokit_key_function long_press, gokit_key_function short_press); 
void key_para_init(keys_typedef_t * keys);
void key_sensortest(void); 

#endif /*_HAL_KEY_H*/

