/**
********************************************************
*
* @file      hal_key.c
* @author    Gizwtis
* @version   V2.3
* @date      2015-07-06
*
* @brief     机智云 只为智能硬件而生
*            Gizwits Smart Cloud  for Smart Products
*            链接|增值|开放|中立|安全|自有|自由|生态
*            www.gizwits.com
*
*********************************************************/
#include "driver/hal_key.h"
#include "gokit.h"

uint32 KeyCountTime; 

/*******************************************************************************
* Function Name  : key_value_read
* Description    : Read the KEY state
* Input          : None
* Output         : None
* Return         : uint8_t KEY state
* Attention		 : None
*******************************************************************************/
uint8_t ICACHE_FLASH_ATTR
key_value_read(void)
{
    uint8_t ReadKey;

    #ifdef KEY1_EANBLE
    if(!GET_KEY1)
    {
        ReadKey |= PRESS_KEY1;
    }
    #endif

    if(!GET_KEY2)
    {
        ReadKey |= PRESS_KEY2;
    }

    return ReadKey;
}


/*******************************************************************************
* Function Name  : key_state_read
* Description    : Read the KEY value
* Input          : None
* Output         : None
* Return         : uint8_t KEY value
* Attention		 : None
*******************************************************************************/
uint8_t ICACHE_FLASH_ATTR
key_state_read(void)
{
    static uint8_t Key_Check;
    static uint8_t Key_State;
    static uint16_t Key_LongCheck;
    static uint8_t Key_Prev    = 0;     //保存上一次按键

    uint8_t Key_press;
    uint8_t Key_return = 0;

    //累加按键时间
    KeyCountTime++;
        
    //KeyCT 1MS+1  按键消抖10MS
    if(KeyCountTime >= DEBOUNCE_TIMEOUT) 
    {
        KeyCountTime = 0; 
        Key_Check = 1;
    }
    
    if(Key_Check == 1)
    {
        Key_Check = 0;
        
        //获取当前按键触发值
        Key_press = key_value_read(); 
        
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
                else 																					//按键抬起,是抖动,不响应按键
                {
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

/*******************************************************************************
* Function Name  : key_gpio_init
* Description    : Configure GPIO Pin
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void ICACHE_FLASH_ATTR
key_gpio_init(void)
{
    /* Migrate your driver code */

    //key1
    #ifdef KEY1_EANBLE
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_KEY1_PIN), 1);
    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
    GPIO_DIS_OUTPUT(GPIO_ID_PIN(GPIO_KEY1_PIN));
    #endif

    //key2
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
    GPIO_DIS_OUTPUT(GPIO_ID_PIN(GPIO_KEY2_PIN));

}

