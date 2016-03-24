#include "gagent.h"
#include "gagent_typedef.h"
#include "gokit.h"
#include "driver/hal_key.h"
#include "driver/hal_infrared.h"

extern pgcontext pgContextData;

/*Global Variable*/
write_info_t wirte_typedef; 
read_info_t read_typedef;

th_typedef_t temphum_typedef;
LOCAL uint32 gokit_tick_count = 0; 
uint8_t report_flag = 0; 
uint8_t Set_LedStatus = 0;
uint8_t NetConfigureFlag = 0;

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

LOCAL void ICACHE_FLASH_ATTR gokit_th_handle(void)
{
    static uint16_t th_ctime = 0;
    static uint16_t th_meanstime = 0; 
    uint8_t curTem = 0, curHum = 0; 
    uint16_t tem_means = 0, hum_means = 0;
    
    if(TH_TIMEOUT < th_ctime) 
    {
        th_ctime = 0;
 
        hdt11_read_data(&curTem, &curHum);
//      GAgent_Printf(GAGENT_DEBUG, "Temperature : %d , Humidity : %d", curTem, curHum);
        
        //é¦–æ¬¡æ•°æ®ä¸»åŠ¨ä¸Šä¼ 
        if((temphum_typedef.pre_tem_means_val == NULL) || (temphum_typedef.pre_hum_means_val == NULL)) 
        {
            temphum_typedef.pre_tem_means_val = curTem; 
            temphum_typedef.pre_hum_means_val = curHum; 

            //å‡†å¤‡ä¸ŠæŠ¥æ•°æ®
            read_typedef.temperature = temphum_typedef.pre_tem_means_val + TEM_OFFSET_VAL; 
            read_typedef.humidity = temphum_typedef.pre_hum_means_val;
            
            report_flag = 1; 
        }
        
        //å‚¨å­˜è¿‘åæ¬¡çš„æ¸©æ¹¿åº¦æ•°å€¼
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
    
    //æ¯ TH_MEANS_TIMEOUT 'Sè®¡ç®—ä¸€æ¬¡æ¸©æ¹¿åº¦å¹³å‡å€¼
    if(TH_MEANS_TIMEOUT < th_meanstime) 
    {
        uint8_t cur_i;
        
        th_meanstime = 0; 

        for(cur_i = 0; cur_i < 10; cur_i++) 
        {
            tem_means += temphum_typedef.th_bufs[cur_i][0]; 
            hum_means += temphum_typedef.th_bufs[cur_i][1]; 
        }
        
        tem_means = tem_means / 10;
        hum_means = hum_means / 10;  
//      GAgent_Printf(GAGENT_CRITICAL, "Temperature : %d , Humidity : %d", tem_means, hum_means);
        
        //åˆ¤æ–­å‰åŽä¸¤æ¬¡å¹³å‡å€¼æ˜¯å¦ç›¸åŒ ä¸åŒåˆ™ä¸»åŠ¨ä¸Šä¼ 
        if((temphum_typedef.pre_tem_means_val != tem_means) || (temphum_typedef.pre_hum_means_val != hum_means)) 
        {
            temphum_typedef.pre_tem_means_val = tem_means;
            temphum_typedef.pre_hum_means_val = hum_means;
                
            //å‡†å¤‡ä¸ŠæŠ¥æ•°æ®
            read_typedef.temperature = temphum_typedef.pre_tem_means_val + TEM_OFFSET_VAL; 
            read_typedef.humidity = temphum_typedef.pre_hum_means_val; 
            
            report_flag = 1; 
        }
    }
    
    th_meanstime++;
    th_ctime++; 
    
}

LOCAL void ICACHE_FLASH_ATTR gokit_ir_handle(void)
{
    uint8_t ir_value = 0; 
    
    ir_value = ir_update_status();
    if(ir_value != read_typedef.infrared)
    {
        GAgent_Printf(GAGENT_DEBUG, "@@@@ ir status %d\n", ir_value);
        read_typedef.infrared = ir_value;
        report_flag = 1;
    }
}

LOCAL void ICACHE_FLASH_ATTR gokit_key_handle(void)
{
    uint8_t key_value = 0;

    key_value = key_state_read();
    if(key_value & KEY_UP)
    {
        if(key_value & PRESS_KEY1)
        {
            GAgent_Printf(GAGENT_CRITICAL, "@@@@ key1, press \n"); 
        }

        if(key_value & PRESS_KEY2)
        {
            GAgent_Printf(GAGENT_CRITICAL, "@@@@ key2, soft ap mode \n"); 
            
            rgb_control(250, 0, 0); 
            NetConfigureFlag = 1;
            GAgent_Config(SOFTAP_MODE, pgContextData); 
        }
    }

    if(key_value & KEY_LONG)
    {
        if(key_value & PRESS_KEY1)
        {
            GAgent_Printf(GAGENT_CRITICAL, "@@@@ key1, default setup\n"); 
            //GAgent_Clean_Config(pgContextData);
            //GAgent_DevSaveConfigData( &(pgContextData->gc) );
            //msleep(1);
            //GAgent_DevReset();
        }

        if(key_value & PRESS_KEY2)
        {
            GAgent_Printf(GAGENT_CRITICAL, "@@@@ key2, airlink mode\n"); 
            
            rgb_control(0, 250, 0); 
            NetConfigureFlag = 1;
            GAgent_Config(AIRLINK_MODE, pgContextData); 
        }
    }

}

LOCAL void ICACHE_FLASH_ATTR gokit_timer_func(void)
{
    /* TEST MODE */
//  soc_sensortest();
        
    gokit_key_handle();
    gokit_th_handle();
    gokit_ir_handle();

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

    if(((GAgentStatus & WIFI_CONNCLOUDS) == WIFI_CONNCLOUDS) && (NetConfigureFlag == 1))
    {
        GAgent_Printf(GAGENT_CRITICAL, "@@@@ W2M->WIFI_CONNCLOUDS \r\n"); 
        
        NetConfigureFlag = 0;
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
//          GAgent_Printf(GAGENT_DEBUG, "########## led onoff %x \n", ctl_data->led_cmd);

                if(Set_LedStatus != 1)
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
                    Set_LedStatus = 0;
                    rgb_control(0, 0, 0);
                    GAgent_Printf(GAGENT_DEBUG, "########## SetLED LED_Costom \r\n");
                }
                if(wirte_typedef.led_cmd == LED_Yellow)
                {
                    Set_LedStatus = 1;
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
                    Set_LedStatus = 1;
                    rgb_control(254, 0, 70);
                    GAgent_Printf(GAGENT_DEBUG, "########## SetLED LED_Purple \r\n");
                }
                if(wirte_typedef.led_cmd == LED_Pink)
                {
                    read_typedef.led_cmd = LED_Pink;
                    read_typedef.led_r = 238;
                    read_typedef.led_g = 30;
                    read_typedef.led_b = 30;
                    Set_LedStatus = 1;
                    rgb_control(238, 30, 30);
                    GAgent_Printf(GAGENT_DEBUG, "########## SetLED LED_Pink \r\n");
                }
            }

            if(0x01 == (ctl_data->attr_flags >> 2) && 0x01)
            {
                if(Set_LedStatus != 1)
                {
                    read_typedef.led_r = wirte_typedef.led_r;
                    GAgent_Printf(GAGENT_DEBUG, "########## W2D Control LED_R = %d \r\n", wirte_typedef.led_r);
                    rgb_control(read_typedef.led_r, read_typedef.led_g, read_typedef.led_b);
                }
            }

            if(0x01 == (ctl_data->attr_flags >> 3) && 0x01)
            {
                if(Set_LedStatus != 1)
                {
                    read_typedef.led_g = wirte_typedef.led_g;
                    GAgent_Printf(GAGENT_DEBUG, "########## W2D Control LED_G = %d \r\n", wirte_typedef.led_g);
                    rgb_control(read_typedef.led_r, read_typedef.led_g, read_typedef.led_b);
                }
            }

            if(0x01 == (ctl_data->attr_flags >> 4) && 0x01)
            {
                if(Set_LedStatus != 1)
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
            
            //Ã¿ÉèÖÃÒ»´Î´«¸ÐÆ÷ÔòÖØÐÂÉÏ±¨Ò»´Î×îÐÂ×´Ì¬
            report_flag = 1; 
            
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

    return;
}

void ICACHE_FLASH_ATTR gokit_hardware_init(void)
{
    LOCAL os_timer_t gokit_timer;

    memset((uint8_t *)&read_typedef, 0, sizeof(read_info_t)); 
    memset((uint8_t *)&wirte_typedef, 0, sizeof(write_info_t)); 
    memset((uint8_t *)&temphum_typedef, 0, sizeof(th_typedef_t)); 

    #ifdef RGBLED_ON
    rgb_gpio_init();
    rgb_led_init();
    #endif
    #ifdef KEY_ON
    key_gpio_init();
    #endif
    #ifdef MOTOR_ON
    motor_init();
    motor_control(5); 
    read_typedef.motor = 5;
    #endif
    #ifdef TEMPHUM_ON
    uint8 ret;
    ret = dh11_init();
    GAgent_Printf(GAGENT_INFO, "dh11_init : %d ", ret);
    #endif
    #ifdef INFRARED_ON
    ir_init();
    #endif    

    os_timer_disarm(&gokit_timer);
    os_timer_setfn(&gokit_timer, (os_timer_func_t *)gokit_timer_func, NULL);
    os_timer_arm(&gokit_timer, 1, 1);
}

void ICACHE_FLASH_ATTR gokit_tick(void)
{

    GAgent_Printf(GAGENT_DEBUG, "@@@@ gokit_task \n");

    if(0 == (gokit_tick_count % 600)) //600s，传感器状态不变，上报数据。
    {
        report_flag = 1;
    }

    if(1 == report_flag)
    {
        gokit_report_data();
        report_flag = 0;
    }

    gokit_tick_count++;
}

void ICACHE_FLASH_ATTR
soc_sensortest(void)
{
    static uint32 SysCountTime = 0;
    
    SysCountTime++;

    if(SysCountTime >= MAX_SOC_TIMOUT) 
    {
        static uint8 Mocou = 0;
        static uint8 rgbcou = 0; 
        static uint8 thcou = 0; 

        SysCountTime = 0; 

        /* Test LOG model */
//      GAgent_Printf(GAGENT_CRITICAL, "ST : %d",system_get_time());

        #ifdef TEMPHUM_ON
        uint8_t curTem = 0, curHum = 0;
        hdt11_read_data(&curTem, &curHum);
        GAgent_Printf(GAGENT_CRITICAL, "Temperature : %d , Humidity : %d", curTem, curHum);
        #endif

        #ifdef INFRARED_ON
        GAgent_Printf(GAGENT_CRITICAL, "InfIO : %d", ir_update_status());
        #endif

        #ifdef KEY_ON
        GAgent_Printf(GAGENT_CRITICAL, "key1 : %d", GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_KEY1_PIN)));
        GAgent_Printf(GAGENT_CRITICAL, "key2 : %d", GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_KEY2_PIN)));
        #endif
        
        #ifdef MOTOR_ON

        if(0 == Mocou)
        {
            GAgent_Printf(GAGENT_CRITICAL, "MO : 0");
            motor_control(0);

        }
        else if(1 == Mocou)
        {
            GAgent_Printf(GAGENT_CRITICAL, "MO : 2");
            motor_control(2);

        }
        else if(2 == Mocou)
        {
            GAgent_Printf(GAGENT_CRITICAL, "MO : 4"); 
            motor_control(4);
            
        } 
        else if(3 == Mocou)
        {
            GAgent_Printf(GAGENT_CRITICAL, "MO : 5");
            motor_control(5);

        }
        else if(4 == Mocou)
        {
            GAgent_Printf(GAGENT_CRITICAL, "MO : 6");
            motor_control(6);

        }
        else if(5 == Mocou)
        {
            GAgent_Printf(GAGENT_CRITICAL, "MO : 8");
            motor_control(8);

        }
        else if(6 == Mocou)
        {
            GAgent_Printf(GAGENT_CRITICAL, "MO : 10");
            motor_control(10);

        }
              
        #endif

        #ifdef RGBLED_ON
        if(0 == rgbcou) 
        {
            rgb_control(0, 0, 250);
            GAgent_Printf(GAGENT_CRITICAL, "RGB : B");
        }
        else if(1 == rgbcou) 
        {
            rgb_control(0, 250, 0);
            GAgent_Printf(GAGENT_CRITICAL, "RGB : G");
        }
        else if(2 == rgbcou) 
        {
            rgb_control(250, 0, 0);
            GAgent_Printf(GAGENT_CRITICAL, "RGB : R");
        }
        #endif

        if(6 > Mocou)
        {
            Mocou++;
        }
        else
        {
            Mocou = 0;
        }
        
        if(2 > rgbcou) 
        {
            rgbcou++; 
        }
        else
        {
            rgbcou = 0; 
        } 
    }

}
