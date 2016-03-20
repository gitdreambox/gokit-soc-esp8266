#include "gagent.h"
#include "gagent_typedef.h"
#include "gokit.h"
#include "driver/hal_key.h"
#include "driver/hal_infrared.h"

#define WIFI_MODE_SOFTAP 1
#define WIFI_MODE_AIRLINK 2

extern pgcontext pgContextData;
read_info_t sensor_status;
LOCAL uint32 gokit_tick_count = 0;
uint8_t report_flag = 0;

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

LOCAL void ICACHE_FLASH_ATTR gokit_key_handle(void)
{
    uint8_t key_value = 0;

    key_value = key_state_read();
    if(key_value & KEY_UP)
    {
        if(key_value & PRESS_KEY1)
        {
            GAgent_Printf(GAGENT_DEBUG, "@@@@ key1, press \n");
        }

        if(key_value & PRESS_KEY2)
        {
            GAgent_Printf(GAGENT_DEBUG, "@@@@ key2, soft ap mode \n");
            //GAgent_Config(WIFI_MODE_SOFTAP, pgContextData);
        }
    }

    if(key_value & KEY_LONG)
    {
        if(key_value & PRESS_KEY1)
        {
            GAgent_Printf(GAGENT_DEBUG, "@@@@ key1, default setup\n");
            //GAgent_Clean_Config(pgContextData);
            //GAgent_DevSaveConfigData( &(pgContextData->gc) );
            //msleep(1);
            //GAgent_DevReset();
        }

        if(key_value & PRESS_KEY2)
        {
            GAgent_Printf(GAGENT_DEBUG, "@@@@ key2, airlink mode\n");
            //GAgent_Config(WIFI_MODE_AIRLINK, pgContextData);
        }
    }

}

LOCAL void ICACHE_FLASH_ATTR gokit_timer_func(void)
{
    uint8_t ir_value = 0;

    gokit_key_handle();

    ir_value = ir_update_status();
    if(ir_value != sensor_status.infrared)
    {
        GAgent_Printf(GAGENT_DEBUG, "@@@@ ir status %d\n", ir_value);
        sensor_status.infrared = ir_value;
        report_flag = 1;
    }
}

void ICACHE_FLASH_ATTR gokit_report_data(void)
{
    ppacket rxbuf = pgContextData->rtinfo.UartRxbuf;

    resetPacket(rxbuf);

    sensor_status.action = 0x04;
    memcpy((uint8_t *)rxbuf->ppayload, (uint8_t *)&sensor_status, sizeof(read_info_t));
    rxbuf->pend = rxbuf->ppayload + sizeof(read_info_t);

    rxbuf->type = SetPacketType( rxbuf->type,LOCAL_DATA_IN,1 );
    rxbuf->type = SetPacketType( rxbuf->type,CLOUD_DATA_IN,0 );
    rxbuf->type = SetPacketType( rxbuf->type,LAN_TCP_DATA_IN,0 );
    ParsePacket( rxbuf );
    setChannelAttrs(pgContextData, NULL, NULL, 1);
    dealPacket(pgContextData, rxbuf);
}

void ICACHE_FLASH_ATTR gokit_ctl_process(pgcontext pgc, ppacket rx_buf)
{
    write_info_t *ctl_data = (write_info_t *)rx_buf->ppayload;

    GAgent_Printf(GAGENT_DEBUG, "@@@@ data len %d action %x\n", rx_buf->pend-rx_buf->ppayload, ctl_data->action);

    if(0x02 == ctl_data->action)
    {
        gokit_report_data();
    }
    else if(0x01 == ctl_data->action)
    {
        if(0x01 == (ctl_data->attr_flags>>0)&&0x01)
        {
            GAgent_Printf(GAGENT_DEBUG, "led onoff %x \n", ctl_data->led_cmd);
        }

        if(0x01 == (ctl_data->attr_flags>>1)&&0x01)
        {
            GAgent_Printf(GAGENT_DEBUG, "led color %d \n", ctl_data->motor);
        }

        if(0x01 == (ctl_data->attr_flags>>2)&&0x01)
        {
            GAgent_Printf(GAGENT_DEBUG, "led r %d \n", ctl_data->led_r);
        }

        if(0x01 == (ctl_data->attr_flags>>3)&&0x01)
        {
            GAgent_Printf(GAGENT_DEBUG, "led g %d \n", ctl_data->led_g);
        }

        if(0x01 == (ctl_data->attr_flags>>4)&&0x01)
        {
            GAgent_Printf(GAGENT_DEBUG, "led b %d \n", ctl_data->led_b);
        }

        if(0x01 == (ctl_data->attr_flags>>5)&&0x01)
        {
            GAgent_Printf(GAGENT_DEBUG, "##########motor %d \n", gokit_ntohs(ctl_data->motor));
            motor_control(gokit_ntohs(ctl_data->motor));
        }
    }
}

void ICACHE_FLASH_ATTR gokit_hardware_init(void)
{
    LOCAL os_timer_t gokit_timer;

    memset((uint8_t *)&sensor_status, 0, sizeof(read_info_t));

    key_gpio_init();
    ir_init();
    motor_init();

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

