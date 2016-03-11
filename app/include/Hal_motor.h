#ifndef _HAL_MOTOR_H
#define _HAL_MOTOR_H

#include <stdio.h>
#include "soc.h"

#define Motor_stop          0
#define Motor_Forward       1
#define Motor_Reverse       2

#define MOTOR_PRAR          1000

/*Define the channel number of PWM*/
#define PWM_CHANNEL	        2  //  3:3channel
//
#define MOTOR_A             0
#define MOTOR_B             1

/*Definition of GPIO PIN params, for GPIO initialization*/
#define PWM_0_OUT_IO_MUX PERIPHS_IO_MUX_MTMS_U
#define PWM_0_OUT_IO_NUM 14
#define PWM_0_OUT_IO_FUNC  FUNC_GPIO14

#define PWM_1_OUT_IO_MUX PERIPHS_IO_MUX_GPIO4_U
#define PWM_1_OUT_IO_NUM 4
#define PWM_1_OUT_IO_FUNC  FUNC_GPIO4

#ifdef	MOTOR_16
typedef uint16_t MOTOR_T;
#else
typedef uint8_t MOTOR_T;
#endif

void motor_init(void);
void motor_control(MOTOR_T status); 

#endif /*_HAL_MOTOR_H*/



