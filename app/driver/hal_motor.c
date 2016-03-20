/**
********************************************************
*
* @file      Hal_motor.c
* @author    Gizwtis
* @version   V3.0
* @date      2016-03-09
*
* @brief     机智云 只为智能硬件而生
*            Gizwits Smart Cloud  for Smart Products
*            链接|增值|开放|中立|安全|自有|自由|生态
*            www.gizwits.com
*
*********************************************************/
#include "driver/hal_motor.h"
#include "pwm.h"
#include "gagent.h"

struct pwm_param Motor_param;

/******************************************************************************
 * FunctionName : user_light_set_duty
 * Description  : set each channel's duty params
 * Parameters   : uint8 duty    : 0 ~ PWM_DEPTH
 *                uint8 channel : LIGHT_RED/LIGHT_GREEN/LIGHT_BLUE
 * Returns      : NONE
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_motor_set_duty(uint32 duty, uint8 channel) 
{
    if(duty != Motor_param.duty[channel])
    {

        pwm_set_duty(duty, channel);

        pwm_start();

        Motor_param.duty[channel] = pwm_get_duty(channel);
    }
}

void ICACHE_FLASH_ATTR
Motor_PWM_Control(uint8_t m1, uint8_t m2)
{
    uint16_t temp = MOTOR_PRAR; 
	
    user_motor_set_duty(m1 * temp, 0); 
    user_motor_set_duty(m2 * temp, 1); 
	
}

void ICACHE_FLASH_ATTR
motor_control(MOTOR_T status)
{		
    if((0 > status) || (9 < status)) 
    {
        GAgent_Printf(GAGENT_ERROR, "Motor_status Error : [%d]", status); 
    }
    
    if(status == 5) 
	{
        Motor_PWM_Control(0, 0); 
	}
	else if (status > 5)
	{
        Motor_PWM_Control(status, status); 
	}
	else if (status < 5)
	{
        Motor_PWM_Control(9 - status, 0); 
	}

}

void ICACHE_FLASH_ATTR
motor_init(void)
{
    /* Migrate your driver code */
    
    if(Motor_param.period > 10000 || Motor_param.period < 1000)
    {
        Motor_param.period = 1000;
    }

    uint32 io_info[][3] = {
        { PWM_0_OUT_IO_MUX, PWM_0_OUT_IO_FUNC, PWM_0_OUT_IO_NUM },
        { PWM_1_OUT_IO_MUX, PWM_1_OUT_IO_FUNC, PWM_1_OUT_IO_NUM },
    };

    uint32 pwm_duty_init[PWM_CHANNEL] = { 0 };

    /*PIN FUNCTION INIT FOR PWM OUTPUT*/
    pwm_init(Motor_param.period, pwm_duty_init, PWM_CHANNEL, io_info);

    os_printf("Motor A: %d \r\n", Motor_param.duty[MOTOR_A]);
    os_printf("Motor B: %d \r\n", Motor_param.duty[MOTOR_B]);
    os_printf("Motor P: %d \r\n", Motor_param.period);

    uint32 light_init_target[8] = { 0 };
    os_memcpy(light_init_target, Motor_param.duty, sizeof(Motor_param.duty));
    
    set_pwm_debug_en(0); //disable debug print in pwm driver
    os_printf("PWM version : %08x \r\n", get_pwm_version());
    
    return;
}

