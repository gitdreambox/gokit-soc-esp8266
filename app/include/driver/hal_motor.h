#ifndef _HAL_MOTOR_H
#define _HAL_MOTOR_H

#include <stdio.h>
#include <c_types.h>
#include <gpio.h>
#include <eagle_soc.h>

/* Define your drive pin */
#define PWM_0_GPIO_PIN      12
#define PWM_1_GPIO_PIN      13

/* Set GPIO Direction */

/*Define the channel number of PWM*/
#define PWM_CHANNEL	        2

/*Definition of GPIO PIN params, for GPIO initialization*/
#define PWM_0_OUT_IO_MUX    PERIPHS_IO_MUX_MTDI_U
#define PWM_0_OUT_IO_NUM    12
#define PWM_0_OUT_IO_FUNC   FUNC_GPIO12

#define PWM_1_OUT_IO_MUX    PERIPHS_IO_MUX_MTCK_U
#define PWM_1_OUT_IO_NUM    13
#define PWM_1_OUT_IO_FUNC   FUNC_GPIO3

#define MOTOR_PERIOD        1000
#define MOTOR_MAX_DUTY      (MOTOR_PERIOD * 1000 / 45)  //1KHz(1000 us) PWM£¬ duty ·¶Î§ÊÇ£º 0 ~ 22222
#define MOTOR_MIN_DUTY      0
#define MOTOR_MAX_STA       10
#define MOTOR_MIN_STA       0
#define MOTOR_SFCT_STA      5

#define CHANNEL_0           0
#define CHANNEL_1           1

#ifdef	MOTOR_16
typedef uint16_t MOTOR_T;
#else
typedef uint8_t MOTOR_T;
#endif

/* Function declaration */
void motor_init(void);
void motor_control(MOTOR_T status); 

#endif /*_HAL_MOTOR_H*/



