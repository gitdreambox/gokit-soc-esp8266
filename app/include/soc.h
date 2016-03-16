#ifndef _SOC_H_
#define _SOC_H_
#include "gagent_typedef.h"
#include "utils.h"
#include "gpio.h"

/* Sensor module selection */
#define MOTOR_ON
#define INFRARED_ON
#define KEY_ON
//#define RGBLED_ON
//#define TEMPHUM_ON

#define PROTOCOL_DEBUG

#define SOC_TIME_OUT                    1
#define MaxSocTimout 					(1000 / SOC_TIME_OUT)  //1S
#define DebounceTimout 					(10 / SOC_TIME_OUT)  //10 MS Debounce time 
#define KeyPrLongTimout 				(10 / SOC_TIME_OUT)  //10 MS De

/* global context, or gagent context */
typedef struct soc_context_t
{
    uint32 SysCountTime; 
//  uint8_t gaterSensorFlag;
//  uint8_t Set_LedStatus = 0;
//  uint8_t NetConfigureFlag = 0;
//  uint32 Last_KeyTime = 0;
//  uint8_t lastTem = 0, lastHum = 0;
    
}soc_context, * soc_pcontext;

extern soc_pcontext soc_context_Data; 

void soc_hw_init(void); 
void soc_sw_init(soc_pcontext * pgc); 
void key_handle(void); 
void soc_tick(soc_pcontext pgc); 

#endif
