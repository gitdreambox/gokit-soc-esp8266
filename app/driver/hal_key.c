
/*********************************************************
*
* @file      hal_key.c
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
#include "driver/hal_key.h"
#include "gagent.h"

uint32 key_count_time; 

/*******************************************************************************
* Function Name  : key_value_read
* Description    : Read the KEY state
* Input          : None
* Output         : None
* Return         : uint8_t KEY state
* Attention      : None
*******************************************************************************/
static uint8_t key_value_read(keys_typedef_t * keys)
{
    uint8_t read_key;

    if(!GPIO_INPUT_GET(keys->single_key[0]->gpio_id)) 
    {
        read_key |= PRESS_KEY1;
    }

    if(!GPIO_INPUT_GET(keys->single_key[1]->gpio_id)) 
    {
        read_key |= PRESS_KEY2;
    }

    return read_key;
}


/*******************************************************************************
* Function Name  : key_state_read
* Description    : Read the KEY value
* Input          : None
* Output         : None
* Return         : uint8_t KEY value
* Attention      : None
*******************************************************************************/
static uint8_t ICACHE_FLASH_ATTR key_state_read(keys_typedef_t * keys)
{
    static uint8_t Key_Check;
    static uint8_t Key_State;
    static uint16_t Key_LongCheck;
    static uint8_t Key_Prev = 0;     //保存上一次按键

    uint8_t Key_press;
    uint8_t Key_return = 0;

    //累加按键时间
    key_count_time++;
        
    //KeyCT 1MS+1  按键消抖20MS
    if(key_count_time >= (20 / keys->key_timer_ms)) 
    {
        key_count_time = 0; 
        Key_Check = 1;
    }
    
    if(Key_Check == 1)
    {
        Key_Check = 0;
        
        //获取当前按键触发值
        Key_press = key_value_read(keys); 
        
        switch (Key_State)
        {
            //"首次捕捉按键"状态
            case 0:
                if(Key_press != 0)
                {
                    Key_Prev = Key_press;
                    Key_State = 1;
                }
    
                break;
                
                //"捕捉到有效按键"状态
            case 1:
                if(Key_press == Key_Prev)
                {
                    Key_State = 2;
                    Key_return= Key_Prev | KEY_DOWN;
                }
                else
                {
                    //按键抬起,是抖动,不响应按键
                    Key_State = 0;
                }
                break;
                
                //"捕捉长按键"状态
            case 2:
    
                if(Key_press != Key_Prev)
                {
                    Key_State = 0;
                    Key_LongCheck = 0;
                    Key_return = Key_Prev | KEY_UP;
                    return Key_return;
                }
    
                if(Key_press == Key_Prev)
                {
                    Key_LongCheck++;
                    if(Key_LongCheck >= 100)    //长按2S
                    {
                        Key_LongCheck = 0;
                        Key_State = 3;
                        Key_return= Key_press |  KEY_LONG;
                        return Key_return;
                    }
                }
                break;
                
                //"还原初始"状态    
            case 3:
                if(Key_press != Key_Prev)
                {
                    Key_State = 0;
                }
                break;
        }
    }

    return  NO_KEY;
}

void gokit_key_handle(keys_typedef_t * keys)
{
    uint8_t key_value = 0;

    key_value = key_state_read(keys); 

    //Callback judgment
    if(key_value & KEY_UP)
    {
        if(key_value & PRESS_KEY1)
        {
            //key1 callback function of short press
            if(keys->single_key[0]->short_press) 
            {
                keys->single_key[0]->short_press(); 
            }
        }

        if(key_value & PRESS_KEY2)
        {
            //key2 callback function of short press
            if(keys->single_key[1]->short_press) 
            {
                keys->single_key[1]->short_press(); 
            }
        }
    }

    if(key_value & KEY_LONG)
    {
        if(key_value & PRESS_KEY1)
        {
            //key1 callback function of long press
            if(keys->single_key[0]->long_press) 
            {
                keys->single_key[0]->long_press(); 
            }
        }

        if(key_value & PRESS_KEY2)
        {
            //key2 callback function of long press
            if(keys->single_key[1]->long_press) 
            {
                keys->single_key[1]->long_press(); 
            }
        }
    }
}

key_typedef_t * ICACHE_FLASH_ATTR key_init_one(uint8 gpio_id, uint32 gpio_name, uint8 gpio_func, gokit_key_function long_press, gokit_key_function short_press)
{
    key_typedef_t * single_key = (key_typedef_t *)os_zalloc(sizeof(key_typedef_t));

    single_key->gpio_id = gpio_id;
    single_key->gpio_name = gpio_name;
    single_key->gpio_func = gpio_func;
    single_key->long_press = long_press;
    single_key->short_press = short_press;

    return single_key;
}

void key_para_init(keys_typedef_t * keys)
{
    uint8 tem_i; 
    
    //init key timer 
    os_timer_disarm(&keys->key_10ms); 
    os_timer_setfn(&keys->key_10ms, (os_timer_func_t *)gokit_key_handle, keys); 
    
    //GPIO configured as a high level input mode
    for(tem_i = 0; tem_i < keys->key_num; tem_i++) 
    {
        PIN_FUNC_SELECT(keys->single_key[tem_i]->gpio_name, keys->single_key[tem_i]->gpio_func); 
        GPIO_OUTPUT_SET(GPIO_ID_PIN(keys->single_key[tem_i]->gpio_id), 1); 
        PIN_PULLUP_EN(keys->single_key[tem_i]->gpio_name); 
        GPIO_DIS_OUTPUT(GPIO_ID_PIN(keys->single_key[tem_i]->gpio_id)); 
        
        GAgent_Printf(GAGENT_DEBUG, "key_gpio%d_init \r\n", keys->key_num + 1); 
    }
    
    //key timer start
    os_timer_arm(&keys->key_10ms, keys->key_timer_ms, 1); 
}

void key_sensortest(void)
{
    /* Test LOG model */
//  single_key[0] = key_init_one(KEY_0_IO_NUM, KEY_0_IO_MUX, KEY_0_IO_FUNC,
//                                  key1_long_press, key1_short_press);
//  single_key[1] = key_init_one(KEY_1_IO_NUM, KEY_1_IO_MUX, KEY_1_IO_FUNC,
//                                  key2_long_press, key2_short_press);
//  keys.key_num = GPIO_KEY_NUM;
//  keys.key_timer_ms = 10;
//  keys.single_key = single_key;
//  key_para_init(&keys);
}

