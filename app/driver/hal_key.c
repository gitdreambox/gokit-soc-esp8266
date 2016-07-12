
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
#include "mem.h"

uint32 keyCountTime; 

/*******************************************************************************
* Function Name  : keyValueRead
* Description    : Read the KEY state
* Input          : None
* Output         : None
* Return         : uint8_t KEY state
* Attention      : None
*******************************************************************************/
static ICACHE_FLASH_ATTR uint8_t keyValueRead(keys_typedef_t * keys)
{
    uint8_t read_key = 0;

    if(!GPIO_INPUT_GET(keys->singleKey[0]->gpio_id))
    {
        read_key |= PRESS_KEY1;
    }

    if(!GPIO_INPUT_GET(keys->singleKey[1]->gpio_id))
    {
        read_key |= PRESS_KEY2;
    }

    return read_key;
}


/*******************************************************************************
* Function Name  : keyStateRead
* Description    : Read the KEY value
* Input          : None
* Output         : None
* Return         : uint8_t KEY value
* Attention      : None
*******************************************************************************/
static uint8_t ICACHE_FLASH_ATTR keyStateRead(keys_typedef_t * keys)
{
    static uint8_t Key_Check = 0;
    static uint8_t Key_State = 0;
    static uint16_t Key_LongCheck = 0;
    static uint8_t Key_Prev = 0;     //保存上一次按键

    uint8_t Key_press = 0;
    uint8_t Key_return = 0;

    //累加按键时间
    keyCountTime++;
        
    //KeyCT 1MS+1  按键消抖30MS
    if(keyCountTime >= (30 / keys->key_timer_ms)) 
    {
        keyCountTime = 0; 
        Key_Check = 1;
    }
    
    if(Key_Check == 1)
    {
        Key_Check = 0;
        
        //获取当前按键触发值
        Key_press = keyValueRead(keys); 
        
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
                    if(Key_LongCheck >= 100)    //长按3S (消抖30MS * 100)
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

void ICACHE_FLASH_ATTR gokitKeyHandle(keys_typedef_t * keys)
{
    uint8_t key_value = 0;

    key_value = keyStateRead(keys); 

    //Callback judgment
    if(key_value & KEY_UP)
    {
        if(key_value & PRESS_KEY1)
        {
            //key1 callback function of short press
            if(keys->singleKey[0]->short_press) 
            {
                keys->singleKey[0]->short_press(); 
            }
        }

        if(key_value & PRESS_KEY2)
        {
            //key2 callback function of short press
            if(keys->singleKey[1]->short_press) 
            {
                keys->singleKey[1]->short_press(); 
            }
        }
    }

    if(key_value & KEY_LONG)
    {
        if(key_value & PRESS_KEY1)
        {
            //key1 callback function of long press
            if(keys->singleKey[0]->long_press) 
            {
                keys->singleKey[0]->long_press(); 
            }
        }

        if(key_value & PRESS_KEY2)
        {
            //key2 callback function of long press
            if(keys->singleKey[1]->long_press) 
            {
                keys->singleKey[1]->long_press(); 
            }
        }
    }
}

key_typedef_t * ICACHE_FLASH_ATTR keyInitOne(uint8 gpio_id, uint32 gpio_name, uint8 gpio_func, gokit_key_function long_press, gokit_key_function short_press)
{
    key_typedef_t * singleKey = (key_typedef_t *)os_zalloc(sizeof(key_typedef_t));

    singleKey->gpio_id = gpio_id;
    singleKey->gpio_name = gpio_name;
    singleKey->gpio_func = gpio_func;
    singleKey->long_press = long_press;
    singleKey->short_press = short_press;

    return singleKey;
}

void ICACHE_FLASH_ATTR keyParaInit(keys_typedef_t * keys)
{
    uint8 tem_i = 0; 
    
    //init key timer 
    os_timer_disarm(&keys->key_10ms); 
    os_timer_setfn(&keys->key_10ms, (os_timer_func_t *)gokitKeyHandle, keys); 
    
    //GPIO configured as a high level input mode
    for(tem_i = 0; tem_i < keys->key_num; tem_i++) 
    {
        PIN_FUNC_SELECT(keys->singleKey[tem_i]->gpio_name, keys->singleKey[tem_i]->gpio_func); 
        GPIO_OUTPUT_SET(GPIO_ID_PIN(keys->singleKey[tem_i]->gpio_id), 1); 
        PIN_PULLUP_EN(keys->singleKey[tem_i]->gpio_name); 
        GPIO_DIS_OUTPUT(GPIO_ID_PIN(keys->singleKey[tem_i]->gpio_id)); 
        
        os_printf("gpio_name %d \r\n", keys->singleKey[tem_i]->gpio_id); 
    }
    
    //key timer start
    os_timer_arm(&keys->key_10ms, keys->key_timer_ms, 1); 
}

void ICACHE_FLASH_ATTR keySensorTest(void)
{
    /* Test LOG model */
//  singleKey[0] = keyInitOne(KEY_0_IO_NUM, KEY_0_IO_MUX, KEY_0_IO_FUNC,
//                                  key1LongPress, key1ShortPress);
//  singleKey[1] = keyInitOne(KEY_1_IO_NUM, KEY_1_IO_MUX, KEY_1_IO_FUNC,
//                                  key2LongPress, key2ShortPress);
//  keys.key_num = GPIO_KEY_NUM;
//  keys.key_timer_ms = 10;
//  keys.singleKey = singleKey;
//  keyParaInit(&keys);
}

