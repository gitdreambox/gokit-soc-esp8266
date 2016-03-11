#ifndef _SOC_H_
#define _SOC_H_
#include "gagent_typedef.h"
#include "utils.h"
#include "gpio.h"

//#define MOTOR_ON
//#define RGBLED_ON
//#define INFRARED_ON

extern pgcontext pgContextData; 

#define PROTOCOL_DEBUG

#define SOC_TIME_OUT                    1
#define MaxSocTimout 					(1000 / SOC_TIME_OUT)  //1S
#define DebounceTimout 					(10 / SOC_TIME_OUT)  //10 MS Debounce time 
#define KeyPrLongTimout 				(10 / SOC_TIME_OUT)  //10 MS De
                                                             //bounce time
/* global context, or gagent context */
typedef struct soc_context_t
{
    uint32 SysCountTime; 
    
}soc_context, * soc_pcontext;

void HW_Init(void); 
void SW_Init(soc_pcontext * pgc); 
void key_handle(void); 
void SOC_Tick(soc_pcontext pgc); 

#endif
