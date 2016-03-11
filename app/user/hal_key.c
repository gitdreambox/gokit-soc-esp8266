/**
********************************************************
*
* @file      hal_key.c
* @author    Gizwtis
* @version   V2.3
* @date      2015-07-06
*
* @brief     ������ ֻΪ����Ӳ������
*            Gizwits Smart Cloud  for Smart Products
*            ����|��ֵ|����|����|��ȫ|����|����|��̬
*            www.gizwits.com
*
*********************************************************/
#include "hal_key.h"

uint32 KeyCountTime; 

/*******************************************************************************
* Function Name  : KEY_GPIO_Init
* Description    : Configure GPIO Pin
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void ICACHE_FLASH_ATTR
KEY_GPIO_Init(void)
{
    //key1
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13); 
    gpio_output_set(0, 0, 0, GPIO_ID_PIN(GPIO_KEY1_PIN)); 
    
    //key2
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12); 
    gpio_output_set(0, 0, 0, GPIO_ID_PIN(GPIO_KEY2_PIN)); 
}

/*******************************************************************************
* Function Name  : Get_Key
* Description    : Read the KEY state
* Input          : None
* Output         : None
* Return         : uint8_t KEY state
* Attention		 : None
*******************************************************************************/
uint8_t ICACHE_FLASH_ATTR
Get_Key(void)
{
    uint8_t ReadKey;

    if(!GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_KEY1_PIN))) 
    {
        ReadKey |= PRESS_KEY1;
    }
    if(!GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_KEY2_PIN))) 
    {
        ReadKey |= PRESS_KEY2;
    }
//  if(!GPIO_INPUT_GET(GPIO_KEY3_PIN))
//  {
//      ReadKey |= PRESS_KEY3;
//  }

    return ReadKey;
}


/*******************************************************************************
* Function Name  : ReadKeyValue
* Description    : Read the KEY value
* Input          : None
* Output         : None
* Return         : uint8_t KEY value
* Attention		 : None
*******************************************************************************/
uint8_t ICACHE_FLASH_ATTR
ReadKeyValue(void)
{
    static uint8_t Key_Check;
    static uint8_t Key_State;
    static uint16_t Key_LongCheck;
    static uint8_t Key_Prev    = 0;     //������һ�ΰ���

    uint8_t Key_press;
    uint8_t Key_return = 0;

    //�ۼӰ���ʱ��
    KeyCountTime++;
        
    //KeyCT 1MS+1  ��������10MS
    if(KeyCountTime >= DebounceTimout) 
    {
        KeyCountTime = 0; 
        Key_Check = 1;
    }
    
    if(Key_Check == 1)
    {
        Key_Check = 0;
        
        //��ȡ��ǰ��������ֵ
        Key_press = Get_Key();
        
        switch (Key_State)
        {
            //"�״β�׽����"״̬
            case 0:
                if(Key_press != 0)
                {
                    Key_Prev = Key_press;
                    Key_State = 1;
                }
    
                break;
                
                //"��׽����Ч����"״̬
            case 1:
                if(Key_press == Key_Prev)
                {
                    Key_State = 2;
                    Key_return= Key_Prev | KEY_DOWN;
                }
                else 																					//����̧��,�Ƕ���,����Ӧ����
                {
                    Key_State = 0;
                }
                break;
                
                //"��׽������"״̬
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
                    if(Key_LongCheck >= 100)    //����2S
                    {
                        Key_LongCheck = 0;
                        Key_State = 3;
                        Key_return= Key_press |  KEY_LONG;
                        return Key_return;
                    }
                }
                break;
                
                //"��ԭ��ʼ"״̬    
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


