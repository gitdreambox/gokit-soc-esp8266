﻿
/*********************************************************
*
* @file      gokit.c
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

#include "gagent.h"
#include "gagent_typedef.h"
#include "gokit.h"
#include "driver/hal_key.h"
#include "driver/hal_infrared.h"
#include "driver/hal_motor.h"
#include "driver/hal_rgb_led.h"
#include "driver/hal_temp_hum.h"

/*Gagent global Variable*/
extern pgcontext pgContextData;

/*Gokit global Variable*/
write_info_t wirte_typedef;
read_info_t read_typedef;

th_typedef_t temphum_typedef;
LOCAL key_typedef_t * single_key[GPIO_KEY_NUM]; 
LOCAL keys_typedef_t keys; 

uint8_t gokit_led_status = 0;
uint8_t gokit_config_flag = 0; 

LOCAL uint16_t ICACHE_FLASH_ATTR gokit_ntohs(uint16_t value)
{
    uint16_t tmp_value = 0;
    uint8_t  *index1 = NULL;
    uint8_t  *index2 = NULL;

    index1 = (uint8_t *)&tmp_value;
    index2 = (uint8_t *)&value;

    *index1 = *(index2+1);
    *(index1+1) = *index2;

    return tmp_value;
}

LOCAL uint8_t ICACHE_FLASH_ATTR gokit_th_handle(void)
{
    static uint16_t th_ctime = 0;
    static uint16_t th_meanstime = 0; 
    uint8_t curTem = 0, curHum = 0; 
    uint16_t tem_means = 0, hum_means = 0;
    
    if(TH_TIMEOUT < th_ctime) 
    {
        th_ctime = 0;
 
        dht11_read_data(&curTem, &curHum); 
        
        //Being the first time and the initiative to report
        if((temphum_typedef.pre_tem_means_val == NULL) || (temphum_typedef.pre_hum_means_val == NULL)) 
        {
            temphum_typedef.pre_tem_means_val = curTem; 
            temphum_typedef.pre_hum_means_val = curHum; 

            read_typedef.temperature = temphum_typedef.pre_tem_means_val + TEM_OFFSET_VAL; 
            read_typedef.humidity = temphum_typedef.pre_hum_means_val;
            
            return (1); 
        }
        
        //Cycle store ten times stronghold
        if(10 > temphum_typedef.th_num) 
        {
            temphum_typedef.th_bufs[temphum_typedef.th_num][0] = curTem; 
            temphum_typedef.th_bufs[temphum_typedef.th_num][1] = curHum; 
            
            temphum_typedef.th_num ++;
        }
        else
        {
            temphum_typedef.th_num = 0;
        }
    }
    
    //Periodically calculate the average temperature and humidity
    if(TH_MEANS_TIMEOUT < th_meanstime) 
    {
        uint8_t cur_i = 0; 
        
        th_meanstime = 0; 

        for(cur_i = 0; cur_i < 10; cur_i++) 
        {
            tem_means += temphum_typedef.th_bufs[cur_i][0]; 
            hum_means += temphum_typedef.th_bufs[cur_i][1]; 
        }
        
        tem_means = tem_means / 10;
        hum_means = hum_means / 10;
        
        //Before and after the two values, it is determined whether or not reported
        if((temphum_typedef.pre_tem_means_val != tem_means) || (temphum_typedef.pre_hum_means_val != hum_means)) 
        {
            temphum_typedef.pre_tem_means_val = (uint8_t)tem_means; 
            temphum_typedef.pre_hum_means_val = (uint8_t)hum_means; 

            //Initiative to report data
            read_typedef.temperature = temphum_typedef.pre_tem_means_val + TEM_OFFSET_VAL; 
            read_typedef.humidity = temphum_typedef.pre_hum_means_val; 
            
            GAgent_Printf(GAGENT_DEBUG, "Temperature : %d , Humidity : %d", read_typedef.temperature, read_typedef.humidity); 
            
            return (1); 
        }
    }
    
    th_meanstime++;
    th_ctime++; 
    
    return (0); 
}

LOCAL uint8_t ICACHE_FLASH_ATTR gokit_ir_handle(void)
{
    uint8_t ir_value = 0; 
    
    ir_value = ir_update_status();
    
    if(ir_value != read_typedef.infrared)
    {
        GAgent_Printf(GAGENT_DEBUG, "@@@@ ir status %d\n", ir_value);
        read_typedef.infrared = ir_value;
        
        return (1); 
    }
    
    return (0);
}

LOCAL uint8_t ICACHE_FLASH_ATTR gokit_report_handle(void)
{
    static uint32 rep_ctime = 0;

    //600s Regularly report
    if(TIM_REP_TIMOUT < rep_ctime) 
    {
        rep_ctime = 0;
        
        GAgent_Printf(GAGENT_DEBUG, "@@@@ gokit_report\n"); 

        return (1);
    }
    
    rep_ctime++;
    
    return (0);
}

void ICACHE_FLASH_ATTR gokit_read_data(void)
{
    ppacket rxbuf = pgContextData->rtinfo.UartRxbuf;

    resetPacket(rxbuf);

    read_typedef.action = P0_D2W_READ_DEVICESTATUS_ACTION_ACK;
    memcpy((uint8_t *)rxbuf->ppayload, (uint8_t *)&read_typedef, sizeof(read_info_t));
    rxbuf->pend = rxbuf->ppayload + sizeof(read_info_t);

    rxbuf->type = SetPacketType(rxbuf->type, LOCAL_DATA_IN, 1);
    rxbuf->type = SetPacketType(rxbuf->type, CLOUD_DATA_IN, 0);
    rxbuf->type = SetPacketType(rxbuf->type, LAN_TCP_DATA_IN, 0);
    ParsePacket(rxbuf);
    setChannelAttrs(pgContextData, NULL, NULL, 1);
    dealPacket(pgContextData, rxbuf);
}

void ICACHE_FLASH_ATTR gokit_report_data(void)
{
    ppacket rxbuf = pgContextData->rtinfo.UartRxbuf;

    resetPacket(rxbuf);

    read_typedef.action = P0_D2W_REPORT_DEVICESTATUS_ACTION; 
    memcpy((uint8_t *)rxbuf->ppayload, (uint8_t *)&read_typedef, sizeof(read_info_t)); 
    rxbuf->pend = rxbuf->ppayload + sizeof(read_info_t);

    rxbuf->type = SetPacketType( rxbuf->type,LOCAL_DATA_IN,1 );
    rxbuf->type = SetPacketType( rxbuf->type,CLOUD_DATA_IN,0 );
    rxbuf->type = SetPacketType( rxbuf->type,LAN_TCP_DATA_IN,0 );
    ParsePacket( rxbuf );
    setChannelAttrs(pgContextData, NULL, NULL, 1);
    dealPacket(pgContextData, rxbuf);
}

void ICACHE_FLASH_ATTR gokit_wifi_Status(pgcontext pgc)
{ 
    uint16 GAgentStatus = 0;

    GAgentStatus = pgc->rtinfo.GAgentStatus;

    if(((GAgentStatus & WIFI_CONNCLOUDS) == WIFI_CONNCLOUDS) && (gokit_config_flag == 1))
    {
        GAgent_Printf(GAGENT_CRITICAL, "@@@@ W2M->WIFI_CONNCLOUDS \r\n"); 
        
        gokit_config_flag = 0;
        rgb_control(0, 0, 0);
    }
}

void ICACHE_FLASH_ATTR gokit_ctl_process(pgcontext pgc, ppacket rx_buf)
{
    write_info_t *ctl_data = (write_info_t *)rx_buf->ppayload;

//  GAgent_Printf(GAGENT_DEBUG, "@@@@ data len %d action %x\n", rx_buf->pend-rx_buf->ppayload, ctl_data->action);

    switch(ctl_data->action) 
    {
        case P0_W2D_CONTROL_DEVICE_ACTION:
        {
            //Copy write data
            memcpy((uint8_t *)&wirte_typedef, (uint8_t *)ctl_data, sizeof(wirte_typedef));

            if(0x01 == (ctl_data->attr_flags >> 0) && 0x01)
            {
                if(gokit_led_status != 1)
                {

                    if(wirte_typedef.led_cmd == LED_OnOff)
                    {
                        rgb_control(0, 0, 0);
                        read_typedef.led_cmd = LED_OnOff;
                        read_typedef.led_r = 0;
                        read_typedef.led_g = 0;
                        read_typedef.led_b = 0; 
                        GAgent_Printf(GAGENT_DEBUG, "########## setled_Off \r\n");
                    }
                    if(wirte_typedef.led_cmd == LED_OnOn)
                    {
                        read_typedef.led_cmd = LED_OnOn;
                        rgb_control(254, 0, 0);
                        GAgent_Printf(GAGENT_DEBUG, "########## setled_On \r\n");
                    }
                }
            }

            if(0x01 == (ctl_data->attr_flags >> 1) && 0x01)
            {
                if(wirte_typedef.led_cmd == LED_Costom)
                {
                    read_typedef.led_cmd = LED_Costom;
                    read_typedef.led_r = 0;
                    read_typedef.led_g = 0;
                    read_typedef.led_b = 0;
                    gokit_led_status = 0;
                    rgb_control(0, 0, 0);
                    GAgent_Printf(GAGENT_DEBUG, "########## SetLED LED_Costom \r\n");
                }
                if(wirte_typedef.led_cmd == LED_Yellow)
                {
                    gokit_led_status = 1;
                    read_typedef.led_cmd = LED_Yellow;
                    read_typedef.led_r = 254;
                    read_typedef.led_g = 254;
                    read_typedef.led_b = 0;

                    rgb_control(254, 254, 0);
                    GAgent_Printf(GAGENT_DEBUG, "########## SetLED LED_Yellow \r\n");
                }

                if(wirte_typedef.led_cmd == LED_Purple)
                {
                    read_typedef.led_cmd = LED_Purple;
                    read_typedef.led_r = 254;
                    read_typedef.led_g = 0;
                    read_typedef.led_b = 70;
                    gokit_led_status = 1;
                    rgb_control(254, 0, 70);
                    GAgent_Printf(GAGENT_DEBUG, "########## SetLED LED_Purple \r\n");
                }
                if(wirte_typedef.led_cmd == LED_Pink)
                {
                    read_typedef.led_cmd = LED_Pink;
                    read_typedef.led_r = 238;
                    read_typedef.led_g = 30;
                    read_typedef.led_b = 30;
                    gokit_led_status = 1;
                    rgb_control(238, 30, 30);
                    GAgent_Printf(GAGENT_DEBUG, "########## SetLED LED_Pink \r\n");
                }
            }

            if(0x01 == (ctl_data->attr_flags >> 2) && 0x01)
            {
                if(gokit_led_status != 1)
                {
                    read_typedef.led_r = wirte_typedef.led_r;
                    GAgent_Printf(GAGENT_DEBUG, "########## W2D Control LED_R = %d \r\n", wirte_typedef.led_r);
                    rgb_control(read_typedef.led_r, read_typedef.led_g, read_typedef.led_b);
                }
            }

            if(0x01 == (ctl_data->attr_flags >> 3) && 0x01)
            {
                if(gokit_led_status != 1)
                {
                    read_typedef.led_g = wirte_typedef.led_g;
                    GAgent_Printf(GAGENT_DEBUG, "########## W2D Control LED_G = %d \r\n", wirte_typedef.led_g);
                    rgb_control(read_typedef.led_r, read_typedef.led_g, read_typedef.led_b);
                }
            }

            if(0x01 == (ctl_data->attr_flags >> 4) && 0x01)
            {
                if(gokit_led_status != 1)
                {
                    read_typedef.led_b = wirte_typedef.led_b;
                    GAgent_Printf(GAGENT_DEBUG, "########## W2D Control LED_B = %d \r\n", wirte_typedef.led_b);
                    rgb_control(read_typedef.led_r, read_typedef.led_g, read_typedef.led_b);
                }
            }

            if(0x01 == (ctl_data->attr_flags >> 5) && 0x01)
            {
                read_typedef.motor = wirte_typedef.motor;

                #ifdef MOTOR_16
                GAgent_Printf(GAGENT_DEBUG, "########## W2D Control Motor = %d \r\n", gokit_ntohs(read_typedef.motor)); 
                motor_control(gokit_ntohs(read_typedef.motor));
                #else
                GAgent_Printf(GAGENT_DEBUG, "########## W2D Control Motor = %d \r\n", read_typedef.motor); 
                motor_control(read_typedef.motor);
                #endif
            }
            
            //The initiative to report after every write
            gokit_report_data(); 
            
            break;
        }

        case P0_W2D_READ_DEVICESTATUS_ACTION:
        {
            gokit_read_data(); 

            break;
        }
        default:
            break;
    }

    return (0);
}

void ICACHE_FLASH_ATTR gokit_timer_func(void)
{
    uint8_t ret = 0;
    
    //Temperature and humidity sensors reported conditional
    ret = gokit_th_handle();
    if(1 == ret)
    {
        gokit_report_data(); 
    }
    
    //Infrared sensors reported conditional
    ret = gokit_ir_handle(); 
    if(1 == ret)
    {
        gokit_report_data(); 
    }
    
    //Regularly report conditional
    ret = gokit_report_handle(); 
    if(1 == ret)
    {
        gokit_report_data();
    }
}

LOCAL void ICACHE_FLASH_ATTR key2_short_press(void)
{
    GAgent_Printf(GAGENT_CRITICAL, "#### key2, soft ap mode \n"); 
    
    rgb_control(250, 0, 0); 
    gokit_config_flag = 1;
    GAgent_Config(WIFI_SOFTAPMODE, pgContextData); 
}

LOCAL void ICACHE_FLASH_ATTR key2_long_press(void)
{
    GAgent_Printf(GAGENT_CRITICAL, "#### key2, airlink mode\n"); 
    
    rgb_control(0, 250, 0); 
    gokit_config_flag = 1;
    GAgent_Config(WIFI_STATIONMODE, pgContextData); 
}

LOCAL void ICACHE_FLASH_ATTR key1_short_press(void)
{
    GAgent_Printf(GAGENT_CRITICAL, "#### key1, press \n"); 
}

LOCAL void ICACHE_FLASH_ATTR key1_long_press(void)
{
    GAgent_Printf(GAGENT_CRITICAL, "#### key1, default setup\n"); 

    GAgent_Clean_Config(pgContextData);
    GAgent_DevSaveConfigData(&(pgContextData->gc));
    msleep(1);
    GAgent_DevReset(); 
}

void ICACHE_FLASH_ATTR gokit_hardware_init(void)
{
    //rgb led init
    rgb_gpio_init();
    rgb_led_init();

    //key init
    single_key[0] = key_init_one(KEY_0_IO_NUM, KEY_0_IO_MUX, KEY_0_IO_FUNC,
                                    key1_long_press, key1_short_press); 
    single_key[1] = key_init_one(KEY_1_IO_NUM, KEY_1_IO_MUX, KEY_1_IO_FUNC,
                                    key2_long_press, key2_short_press); 
    keys.key_num = GPIO_KEY_NUM; 
    keys.key_timer_ms = 10;
    keys.single_key = single_key; 
    key_para_init(&keys);

    //motor init
    motor_init();
    motor_control(MOTOR_DF_VAL); 

    //temperature and humidity init
    dh11_init();

    //Infrared init
    ir_init(); 

    GAgent_Printf(GAGENT_DEBUG, "gokit hardware init OK \r\n");
}

void ICACHE_FLASH_ATTR gokit_software_init(void)
{
    memset((uint8_t *)&read_typedef, 0, sizeof(read_info_t));
    memset((uint8_t *)&wirte_typedef, 0, sizeof(write_info_t));
    memset((uint8_t *)&temphum_typedef, 0, sizeof(th_typedef_t)); 
    
    read_typedef.motor = MOTOR_DF_VAL; 
    
    GAgent_Printf(GAGENT_DEBUG, "gokit software init OK \r\n"); 
}
