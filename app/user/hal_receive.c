#include "gagent.h"
#include "hal_receive.h"

#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"
#include "osapi.h"
#include "driver/uart_register.h"
#include "mem.h"
#include "os_type.h"

uint8 *hal_RxBuffer=NULL;
/*
 pos_start:pos of get
 pos_current:pos of put
 */
static uint32 pos_start = 0;//read
static uint32 pos_current = 0;//write

/*
 * 串口通讯协议
 * | head(0xffff) | len(2B) | cmd(2B) | SN(1B) | flag(2B) | payload(xB) | checksum(1B) |
 *     0xffff     cmd~checksum                                            len~payload
 *     0xff-->LOCAL_HAL_REC_SYNCHEAD1
 *         ff->LOCAL_HAL_REC_SYNCHEAD2
 *                  len_1-->LOCAL_HAL_REC_DATALEN1
 *                  len_2-->LOCAL_HAL_REC_DATALEN2
 *                          | ------------     LOCAL_HAL_REC_DATA     -----------------|
 *                | ------- halRecKeyWord set to 1 while rec first byte 0xff ----------|
 */
/****************LOCAL MACRO AND VAR**********************************/
// for gu8LocalHalStatus
    #define         LOCAL_HAL_REC_SYNCHEAD1     1
    #define         LOCAL_HAL_REC_SYNCHEAD2     2
    #define         LOCAL_HAL_REC_DATALEN1      3
    #define         LOCAL_HAL_REC_DATALEN2      4
    #define         LOCAL_HAL_REC_DATA          5
static uint8 gu8LocalHalStatus = LOCAL_HAL_REC_SYNCHEAD1;
/* 见上协议，len字段 */
static uint16 gu16LocalPacketDataLen;
/* 一包数据的气味位置偏移 */
static uint32 guiLocalPacketStart;
/* 非包头部分，数据0xff应该变换为0xff 55,不影响校验和和len。因此，非包头部分，收到0xff后，该标志位被置1 */
static uint8 halRecKeyWord = 0;

static uint8 ICACHE_FLASH_ATTR __halbuf_read(uint32 offset)
{
    return hal_RxBuffer[offset & HAL_BUF_MASK];
}

static void ICACHE_FLASH_ATTR __halbuf_write(uint32 offset, uint8 value)
{
    hal_RxBuffer[offset & HAL_BUF_MASK] = value;
}
/****************************************************************
FunctionName    :   get_data_len
Description     :   get data length from "from " to "to"
return          :   the length of data.
Add by Alex.lin and Johnson     --2015-04-07
****************************************************************/
static int32 ICACHE_FLASH_ATTR
get_data_len( int32 from, int32 to )
{
    to = to & HAL_BUF_MASK;
    from = from & HAL_BUF_MASK;

    if(to >= from)
        return to  - from;
    else
        return HAL_BUF_SIZE - from + to;
}

/****************************************************************
FunctionName    :   get_available_buf_space
Description     :   get buf of availabel size of buf
pos_current     :   current position
pos_start       :   start position
return          :   available size
Add by Alex.lin and Johnson     --2015-04-07
****************************************************************/
int32 ICACHE_FLASH_ATTR
get_available_buf_space(int32 pos_current, int32 pos_start)
{
    pos_current = pos_current & HAL_BUF_MASK;
    pos_start = pos_start & HAL_BUF_MASK;

    if(pos_current >= pos_start)
    {
        return HAL_BUF_SIZE - pos_current;
    }
    else
    {
        return pos_start - pos_current;
    }
}

/****************************************************************
FunctionName    :   move_data_backward
Description     :   move data backward
buf             :   the pointer of hal_buf
pos_start       :   the pos of start of move.
pos_end         :   the pos of end of move.
move_len        :   the  length of data need to move.
return          :   NULL
Add by Alex.lin and Johnson     --2015-04-07
****************************************************************/
 //向前（左）移动数据，被移动的数据为从p_start_pos开始，到p_end_pos，移动长度为move_len
void ICACHE_FLASH_ATTR
move_data_backward( uint8 *buf, int32 pos_start, int32 pos_end, int32 move_len)
{
    int32 k;
    int32 pos_new_start;
    int32 move_data_len = get_data_len(pos_start, pos_end);

    pos_new_start = pos_start + (-1)*move_len;

    for(k=0; k < move_data_len; k++)
    {
        __halbuf_write(pos_new_start + k, __halbuf_read(pos_start + k));
    }
}

/****************************************************************
FunctionName    :   GAgent_Local_ExtractOnePacket
Description     :   extract one packet from local cycle buf, and
                    put data into buf.Will change pos_start
buf             :   dest buf
return          :   >0 the local packet data length.
                    <0 don't have one whole packet data
****************************************************************/
int32 ICACHE_FLASH_ATTR
GAgent_Local_ExtractOnePacket(uint8 *buf)
{
    uint8 data;
    uint32 i = 0;

    while((pos_start & HAL_BUF_MASK) != (pos_current & HAL_BUF_MASK))
    {
        data = __halbuf_read(pos_start);

        if(LOCAL_HAL_REC_SYNCHEAD1 == gu8LocalHalStatus)
        {
            if(MCU_HDR_FF == data)
            {
                gu8LocalHalStatus = LOCAL_HAL_REC_SYNCHEAD2;

                guiLocalPacketStart = pos_start;
            }
        }
        else if(LOCAL_HAL_REC_SYNCHEAD2 == gu8LocalHalStatus)
        {
            if(MCU_HDR_FF == data)
            {
                gu8LocalHalStatus = LOCAL_HAL_REC_DATALEN1;
            }
            else
            {
                gu8LocalHalStatus = LOCAL_HAL_REC_SYNCHEAD1;
            }
        }
        else
        {
            if(halRecKeyWord)
            {
                /* 前面接收到0xff */
                halRecKeyWord = 0;
                if(0x55 == data)
                {
                    data = 0xff;
                    move_data_backward(hal_RxBuffer, pos_start + 1, pos_current, 1);
                    pos_current--;
                    pos_start--;

                }
                else if(MCU_HDR_FF == data)
                {
                    /* 新的一包数据，前面数据丢弃 */
                    gu8LocalHalStatus = LOCAL_HAL_REC_DATALEN1;
                    guiLocalPacketStart = pos_start - 1;
                    pos_start++;
                    continue;
                }
                else
                {
                    if(LOCAL_HAL_REC_DATALEN1 == gu8LocalHalStatus)
                    {
                        /* 说明前面接收到的0xff和包头0xffff是连在一起的，以最近的0xffff作为包头起始，
                         * 当前字节作为len字节进行解析
                         */
                        guiLocalPacketStart = pos_start - 2;
                    }
                    else
                    {
                        gu8LocalHalStatus = LOCAL_HAL_REC_SYNCHEAD1;
                        pos_start++;
                        continue;
                    }
                }

            }
            else
            {
                if(MCU_HDR_FF == data)
                {
                    halRecKeyWord = 1;
                    pos_start++;
                    continue;
                }
            }

            if(LOCAL_HAL_REC_DATALEN1 == gu8LocalHalStatus)
            {
                gu16LocalPacketDataLen = data;
                gu16LocalPacketDataLen = (gu16LocalPacketDataLen << 8) & 0xff00;
                gu8LocalHalStatus = LOCAL_HAL_REC_DATALEN2;
            }
            else if(LOCAL_HAL_REC_DATALEN2 == gu8LocalHalStatus)
            {
                gu16LocalPacketDataLen += data;
                gu8LocalHalStatus = LOCAL_HAL_REC_DATA;

                if(0 == gu16LocalPacketDataLen)
                {
                    /* invalid packet */
                    gu8LocalHalStatus = LOCAL_HAL_REC_SYNCHEAD1;
                }
            }
            else
            {
                /* Rec data */
                gu16LocalPacketDataLen--;
                if(0 == gu16LocalPacketDataLen)
                {
                    /* 接收到完整一包数据，拷贝到应用层缓冲区 */
                    pos_start++;
                    i = 0;
                    while(guiLocalPacketStart != pos_start)
                    {
                        buf[i] = __halbuf_read(guiLocalPacketStart++);\
                        i++;
                    }
                    return i;
                }
            }

        }

        pos_start++;
    }

    return RET_FAILED;
}

int32 ICACHE_FLASH_ATTR
GAgent_Local_RecAll(pgcontext pgc)
{
    int32 fd;
    uint32 idx=0;
    uint32 i;
    uint32 read_count = 0;
    int32 available_len =0;
    uint8 d_tmp = 0;

//    uint8 fifo_len = (READ_PERI_REG(UART_STATUS(UART0))>>UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
//    for(idx=0;idx<fifo_len;idx++)//???մ???????
//    {
//        d_tmp = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
//        read_count = fifo_len; //?ܹ???Ҫ???ܵ????ݳ???
//        hal_RxBuffer[(pos_current & HAL_BUF_MASK) + idx] = d_tmp;
//    }
//    WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR);
//    uart_rx_intr_enable(UART0);
//    pos_current = (pos_current & HAL_BUF_MASK) + read_count;

    available_len = get_available_buf_space( pos_current, pos_start );
    read_count = pgc->rtinfo.uart_recv_len;
//    GAgent_Printf(GAGENT_DEBUG,"uart recv count = %d\n",read_count);
//    for(i=0; i<read_count; i++)
//    {
//        os_printf("%02x ",pgc->rtinfo.pRxBuf[i]);
//    }
    for(idx=0;idx<read_count;idx++)//???մ???????
    {
        hal_RxBuffer[(pos_current+idx) & HAL_BUF_MASK] = pgc->rtinfo.pRxBuf[idx];
    }
    pos_current += read_count;
    memset(pgc->rtinfo.pRxBuf,0,1024);
    return read_count;
}

void ICACHE_FLASH_ATTR
hal_ReceiveInit(  )
{
    hal_RxBuffer = (uint8*)os_malloc( HAL_BUF_SIZE );
    while( hal_RxBuffer==NULL )
    {
        hal_RxBuffer = (uint8*)os_malloc( HAL_BUF_SIZE );
        //sleep(1);
    }
}