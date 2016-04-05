#include "gagent.h"
#include "lan.h"
#include "mqttlib.h"
#include "utils.h"
#include "platform.h"

void ICACHE_FLASH_ATTR
Lan_setClientTimeOut(pgcontext pgc, int32 channel)
{
    uint16 GAgentStatus=0;
    GAgentStatus = pgc->rtinfo.GAgentStatus;
    if( (GAgentStatus&WIFI_MODE_AP)== WIFI_MODE_AP )
    {
        pgc->ls.tcpClient[channel].timeout = LAN_CLIENT_MAXLIVETIME*5;
    }
    if( (GAgentStatus&WIFI_MODE_STATION) == WIFI_MODE_STATION )
    {
        pgc->ls.tcpClient[channel].timeout = LAN_CLIENT_MAXLIVETIME;
    }
}

int32 ICACHE_FLASH_ATTR
Lan_AddTcpNewClient(pgcontext pgc,uint8 *ip, int port)
{
    int32 i,j;


    for(i = 0; i < LAN_TCPCLIENT_MAX; i++)
    {
        if(pgc->ls.tcpClient[i].fd == INVALID_SOCKET)
        {
            pgc->ls.tcpClient[i].fd_isset = 1;
            pgc->ls.tcpClient[i].fd = 1;
            pgc->ls.tcpClient[i].remote_port = port;

            os_memcpy(pgc->ls.tcpClient[i].remote_ip, ip, 4);
            pgc->ls.tcpClient[i].remote_port = port;
            Lan_setClientTimeOut(pgc, i);
            return RET_SUCCESS;
        }
    }
//    GAgent_Printf(GAGENT_DEBUG, "[LAN]tcp client over %d channel, denied!", LAN_TCPCLIENT_MAX);
//    espconn_disconnect(espconn);
    return RET_FAILED;
}
int32 ICACHE_FLASH_ATTR
Lan_tcpClientDataHandle(pgcontext pgc, uint32 channel,ppacket prxBuf,/* ppacket ptxBuf,*/ int32 buflen)
{
    Lan_setClientTimeOut(pgc, channel);
    return Lan_dispatchTCPData(pgc, prxBuf,/* ptxBuf,*/ channel);
}

/****************************************************************
        FunctionName        :   Lan_checkAuthorization.
        Description         :      check Authorization.
        Add by Nik.chen     --2015-04-18
****************************************************************/
static uint32 ICACHE_FLASH_ATTR
Lan_checkAuthorization( pgcontext pgc,  int clientIndex)
{

    if((LAN_CLIENT_LOGIN_FAIL == pgc->ls.tcpClient[clientIndex].isLogin)
        &&(INVALID_SOCKET != pgc->ls.tcpClient[clientIndex].fd))
    {
        os_memcpy(pgc->ls.tcpServer->proto.tcp->remote_ip,pgc->ls.tcpClient[clientIndex].remote_ip,4);
        pgc->ls.tcpServer->proto.tcp->remote_port = pgc->ls.tcpClient[clientIndex].remote_port;
        espconn_disconnect(pgc->ls.tcpServer);
        pgc->ls.tcpClient[clientIndex].fd = INVALID_SOCKET;
        os_memset(pgc->ls.tcpClient[clientIndex].remote_ip,0,4);
        pgc->ls.tcpClient[clientIndex].remote_port = 0;

        GAgent_Printf(GAGENT_WARNING, "Illegal tcp client login!!! clientid[%d] ",clientIndex);
            return 0;
     }

    return 1;
}

/****************************************************************
        FunctionName        :   Lan_handleLogin.
        Description         :      Lan Tcp logining.
        Add by Will.zhou     --2015-03-10
****************************************************************/
static void ICACHE_FLASH_ATTR
Lan_handleLogin( pgcontext pgc, ppacket src, int clientIndex)
{
    uint8 isLogin;
    uint8 *pbuf;

    /* verify passcode */
    if( !os_memcmp((src->phead + 10), pgc->gc.wifipasscode, os_strlen( pgc->gc.wifipasscode)) )
    {
        /* login success */
        isLogin = LAN_CLIENT_LOGIN_SUCCESS;
        pgc->ls.tcpClientNums++;
        GAgent_SetWiFiStatus( pgc,WIFI_CLIENT_ON,1 );

        GAgent_Printf(GAGENT_INFO, "LAN login success! clientid[%d] ",clientIndex);
    }
    else
    {
        isLogin = LAN_CLIENT_LOGIN_FAIL;
        GAgent_Printf(GAGENT_WARNING,"LAN login fail. your passcode:%s",src->phead + 10);
        GAgent_Printf(GAGENT_INFO, "expected passcode:%s", pgc->gc.wifipasscode);
    }

    resetPacket( src );
    pbuf = src->phead;
    /* protocol version */
     pbuf[0] = 0x00;
     pbuf[1] = 0x00;
     pbuf[2] = 0x00;
     pbuf[3] = 0x03;

     /* len */
     pbuf[4] = 0x04;

     /* flag */
     pbuf[5] = 0x00;

     /* cmd */
     pbuf[6] = 0x00;
     pbuf[7] = 0x09;

     /* login result */
     pbuf[8] = isLogin;

     pgc->ls.tcpClient[clientIndex].isLogin = isLogin;

     os_memcpy(pgc->ls.tcpServer->proto.tcp->remote_ip,pgc->ls.tcpClient[clientIndex].remote_ip,4);
     pgc->ls.tcpServer->proto.tcp->remote_port = pgc->ls.tcpClient[clientIndex].remote_port;
     espconn_sent( pgc->ls.tcpServer, pbuf, 9 );

     if(LAN_CLIENT_LOGIN_FAIL == isLogin)
     {
        os_memcpy(pgc->ls.tcpServer->proto.tcp->remote_ip,pgc->ls.tcpClient[clientIndex].remote_ip,4);
        pgc->ls.tcpServer->proto.tcp->remote_port = pgc->ls.tcpClient[clientIndex].remote_port;
        espconn_disconnect( pgc->ls.tcpServer);
        pgc->ls.tcpClient[clientIndex].fd = INVALID_SOCKET;
        os_memset(pgc->ls.tcpClient[clientIndex].remote_ip,0,4);
        pgc->ls.tcpClient[clientIndex].remote_port = 0;

     }

}

/****************************************************************
        FunctionName        :   Lan_handlePasscode.
        Description         :   reponsing passcode to client for Binding.
        Add by Will.zhou     --2015-03-10
****************************************************************/
static void ICACHE_FLASH_ATTR
Lan_handlePasscode( pgcontext pgc, ppacket src, int clientIndex)
{
    int i;
    int ret;
    int32 fd;
    uint16 passcodeLen;
    uint8 *pbuf;

    resetPacket(src);
    pbuf = src->phead;
    fd = pgc->ls.tcpClient[clientIndex].fd;

    /* protocol version */
    pbuf[0] = 0x00;
    pbuf[1] = 0x00;
    pbuf[2] = 0x00;
    pbuf[3] = 0x03;

    /* len */
    pbuf[4] = 0x0f;

    /* flag */
    pbuf[5] = 0x00;

    /* cmd */
    pbuf[6] = 0x00;
    pbuf[7] = 0x07;

    /* passcode len */
    passcodeLen = os_strlen( pgc->gc.wifipasscode );
    *(uint16 *)(pbuf + 8) =  HTONS(passcodeLen);

    /* passcode */
    for(i=0;i < passcodeLen;i++)
    {
        pbuf[10+i] = pgc->gc.wifipasscode[i];
    }

    if((pgc->rtinfo.GAgentStatus & WIFI_MODE_BINDING) == WIFI_MODE_BINDING)//enable bind
    {
        os_memcpy(pgc->ls.tcpServer->proto.tcp->remote_ip,pgc->ls.tcpClient[clientIndex].remote_ip,4);
        pgc->ls.tcpServer->proto.tcp->remote_port = pgc->ls.tcpClient[clientIndex].remote_port;
        ret = espconn_sent(pgc->ls.tcpServer, pbuf, 20);
        GAgent_Printf(GAGENT_INFO,"Send passcode(%s) to client[%d][send data len:%d] ", pgc->gc.wifipasscode, fd, ret);
    }
    else
    {
        /* passcode len */
        pbuf[8] = 0x00;
        pbuf[9] = 0x00;

        os_memcpy(pgc->ls.tcpServer->proto.tcp->remote_ip,pgc->ls.tcpClient[clientIndex].remote_ip,4);
        pgc->ls.tcpServer->proto.tcp->remote_port = pgc->ls.tcpClient[clientIndex].remote_port;
        ret = espconn_sent( pgc->ls.tcpServer, pbuf, 10 );

        GAgent_Printf(GAGENT_ERROR,"timeout,client[%d]cannot bind,close socket", fd);
        espconn_disconnect(pgc->ls.tcpServer);
        pgc->ls.tcpClient[clientIndex].fd = INVALID_SOCKET;
        os_memset(pgc->ls.tcpClient[clientIndex].remote_ip,0,4);
        pgc->ls.tcpClient[clientIndex].remote_port = 0;
    }

    return;
}
void ICACHE_FLASH_ATTR
Lan_GetWifiHotspots( pgcontext pgc,ppacket pTxBuf,int32 clientIndex )
{
    volatile varc hostvarlen;
    uint8 *pbuf=NULL;
    uint8 ssidlen=0;
    int32 ret=0;
    int32 pos=0,payloadLen=0,wifihostspotslen=0;
    int32 fd=0,i=0;
    int32 bytesOfLen=0,len=0;

    fd = pgc->ls.tcpClient[clientIndex].fd;
    resetPacket( pTxBuf );
    pbuf = pTxBuf->phead;

    if(0==pgc->rtinfo.aplist.ApNum)
    {
        wifihostspotslen =0;
    }
    else
    {

        for( i=0;i<pgc->rtinfo.aplist.ApNum;i++ )
        {
            ssidlen = os_strlen((const int8 *)(pgc->rtinfo.aplist.ApList[i].ssid) );
            //ssidlen
            wifihostspotslen+=2;
            //ssid
            wifihostspotslen+=ssidlen;
            //power
            wifihostspotslen++;
        }
    }
    //varlen = flag(1b)+cmd(2b)+wifihotspotsLen
    payloadLen = 1+2+wifihostspotslen;
    hostvarlen = Tran2varc( payloadLen );

    /* protocol version */
    pbuf[0] = 0x00;
    pbuf[1] = 0x00;
    pbuf[2] = 0x00;
    pbuf[3] = 0x03;
    /* len */
    for( i=0;i<hostvarlen.varcbty;i++ )
    {
        pbuf[4+i] = hostvarlen.var[i];
    }
    pos=4+hostvarlen.varcbty;
    /* flag */
    pbuf[pos] = 0x00;
    pos++;
    /* cmd */
    pbuf[pos] = 0x00;
    pbuf[pos+1] = 0x0D;
    pos+=2;
    if(0==pgc->rtinfo.aplist.ApNum)
    {
        os_memcpy(pgc->ls.tcpServer->proto.tcp->remote_ip,pgc->ls.tcpClient[clientIndex].remote_ip,4);
        pgc->ls.tcpServer->proto.tcp->remote_port = pgc->ls.tcpClient[clientIndex].remote_port;
        ret = espconn_sent( pgc->ls.tcpServer, pbuf, pos );
    }
    else
    {
        for( i=0;i<pgc->rtinfo.aplist.ApNum;i++ )
        {
            ssidlen = os_strlen((const int8 *)(pgc->rtinfo.aplist.ApList[i].ssid) );
            pbuf[pos+1] = ssidlen;
            pos+=2;
            os_strcpy( (int8*)(pbuf+pos),(const int8 *)(pgc->rtinfo.aplist.ApList[i].ssid));
            pos+=ssidlen;
            pbuf[pos] = pgc->rtinfo.aplist.ApList[i].ApPower;
            pos++;
        }
        os_memcpy(pgc->ls.tcpServer->proto.tcp->remote_ip,pgc->ls.tcpClient[clientIndex].remote_ip,4);
        pgc->ls.tcpServer->proto.tcp->remote_port = pgc->ls.tcpClient[clientIndex].remote_port;
        ret = espconn_sent( pgc->ls.tcpServer, pbuf, pos );
    }

    GAgent_Printf( GAGENT_INFO,"LEN=%d",pos );
    GAgent_Printf( GAGENT_INFO,"--------------------------------------------\r\n");
    for( i=0;i<pos;i++ )
    {
        GAgent_Printf(  GAGENT_DUMP," %02X",pbuf[i] );
    }
    GAgent_Printf( GAGENT_INFO,"--------------------------------------------");
    bytesOfLen = mqtt_num_rem_len_bytes( pbuf+ 3);
    len = mqtt_parse_rem_len( pbuf+ 3 );
    GAgent_Printf( GAGENT_INFO," bytesOfLen=%d  len=%d",bytesOfLen,len );
    msleep();
    //close( fd );
    pgc->ls.tcpClient[clientIndex].fd = INVALID_SOCKET;
    pgc->ls.tcpClient[clientIndex].isLogin = LAN_CLIENT_LOGIN_FAIL;
    pgc->ls.tcpClient[clientIndex].timeout = 0;

    if(pgc->ls.tcpClientNums > 0)
    {
        pgc->ls.tcpClientNums--;
    }
}
void ICACHE_FLASH_ATTR
GAgent_Lan_SendDevInfo(pgcontext pgc,ppacket pTxBuf,int32 clientIndex)
{
    varc sendvarc;
    uint32 dataLen;
    uint16 wifi_firmware_ver_len;
    uint16 tmp;
    uint16 produckt_key_len = 32;
    int i;
    int offset=0;

    /* protocol */
    *(uint32 *)pTxBuf->phead = HTONL(GAGENT_PROTOCOL_VERSION);
    offset += LAN_PROTOCOL_HEAD_LEN;
    /* varLen */
    /* flag(1B) | cmd(2B) | p0(116B)  */
    dataLen = 1+2+84;
    sendvarc = Tran2varc(dataLen);
    for(i=0;i<sendvarc.varcbty;i++)
    {
        pTxBuf->phead[offset] = sendvarc.var[i];
        offset++;
    }
    /* flag */
    pTxBuf->phead[offset] = 0x00;
    offset +=1;
    /* cmd */
    pTxBuf->phead[offset] = 0x00;
    pTxBuf->phead[offset+1] = 0x14;
    offset +=2;
    /* wifi_hard_ver */
    os_strcpy( (char *)pTxBuf->phead+offset,WIFI_HARDVER);
    offset +=8;
    /* wifi_soft_ver */
    os_strcpy( (char *)pTxBuf->phead+offset,WIFI_SOFTVAR);
    offset +=8;
    /* mcu_hard_ver */
    for(i=0;i<8;i++)
        pTxBuf->phead[offset+i]=pgc->mcu.hard_ver[i];
    offset +=8;
    /* mcu_soft_ver */
    for(i=0;i<8;i++)
        pTxBuf->phead[offset+i]=pgc->mcu.soft_ver[i];
    offset +=8;
    /* protocol version of payload */
    for(i=0;i<8;i++)
        pTxBuf->phead[offset+i]=pgc->mcu.p0_ver[i];
    offset +=8;
    /* wifi_firmware_id */
    for(i=0;i<8;i++)
         pTxBuf->phead[offset+i]=0;
    offset +=8;
    /* wifi_firmware_ver_len */
    wifi_firmware_ver_len = 0;
    if(wifi_firmware_ver_len > FIRMWARE_LEN_MAX)
    {
        wifi_firmware_ver_len = FIRMWARE_LEN_MAX;
        pgc->gc.FirmwareVer[FIRMWARE_LEN_MAX - 1] = 0;
    }
    //*(uint16 *)(pTxBuf->phead + offset) = HTONS(wifi_firmware_ver_len);
    tmp = HTONS(wifi_firmware_ver_len);
    os_memcpy((pTxBuf->phead + offset), &tmp, 2);

    offset +=2;
    /* wifi_firmware_ver */
    for(i = 0;i < wifi_firmware_ver_len;i++)
        pTxBuf->phead[offset+i]=pgc->gc.FirmwareVerLen[i];
    offset +=wifi_firmware_ver_len;
    /* produckt_key_len */
//    *(uint16 *)(pTxBuf->phead + offset) = HTONS(32);
    tmp = HTONS(produckt_key_len);
    os_memcpy((pTxBuf->phead + offset), &tmp, 2);

    offset +=2;
    /* produckt_key */
    for(i=0;i<32;i++)
         pTxBuf->phead[offset+i]=pgc->mcu.product_key[i];
    offset +=32;
    if(pgc->ls.tcpClient[clientIndex].fd > 0 )
    {
        os_memcpy(pgc->ls.tcpServer->proto.tcp->remote_ip,pgc->ls.tcpClient[clientIndex].remote_ip,4);
        pgc->ls.tcpServer->proto.tcp->remote_port = pgc->ls.tcpClient[clientIndex].remote_port;
        espconn_sent( pgc->ls.tcpServer, pTxBuf->phead, offset );
    }
}

/****************************************************************
        FunctionName        :   Lan_AckHeartbeak.
        Description         :      Gagent response client heartbeat
        Add by Will.zhou     --2015-03-10
****************************************************************/
static void ICACHE_FLASH_ATTR
Lan_AckHeartbeak( pgcontext pgc, ppacket src, int clientIndex )
{
    int32 fd;
    uint8 *pbuf;

    resetPacket(src);
    pbuf = src->phead;
    fd = pgc->ls.tcpClient[clientIndex].fd;

    /* protocol version */
    pbuf[0] = 0x00;
    pbuf[1] = 0x00;
    pbuf[2] = 0x00;
    pbuf[3] = 0x03;

    /* len */
    pbuf[4] = 0x03;

    /* flag */
    pbuf[5] = 0x00;

    /* cmd */
    pbuf[6] = 0x00;
    pbuf[7] = 0x16;

    os_memcpy(pgc->ls.tcpServer->proto.tcp->remote_ip,pgc->ls.tcpClient[clientIndex].remote_ip,4);
    pgc->ls.tcpServer->proto.tcp->remote_port = pgc->ls.tcpClient[clientIndex].remote_port;
    espconn_sent( pgc->ls.tcpServer, pbuf, 8 );
}
/****************************************************************
        FunctionName        :   Local_Ack2TcpClient.
        Description         :   Gagent response test ack to tcp client
        Add by Frank Liu        --2015-05-05
****************************************************************/
void ICACHE_FLASH_ATTR
Local_Ack2TcpClient(pgcontext pgc, uint32 channel)
{
    int8 tmpBuf[8] = {0,};
    int8 flag = 0;
    int16 cmdWord = GAGENT_LAN_REPLY_TEST;
    os_memset(tmpBuf,0,sizeof(tmpBuf));

    if(pgc->ls.tcpClient[channel].fd > 0)
    {
        //protocolver
        *(uint32 *)tmpBuf = HTONL(GAGENT_PROTOCOL_VERSION);
        tmpBuf[4] = sizeof(flag) + sizeof(cmdWord);
        //flag
        tmpBuf[5] = flag;
        //cmdword
        *(uint16 *)(tmpBuf + 6) = HTONS(cmdWord);

        os_memcpy(pgc->ls.tcpServer->proto.tcp->remote_ip,pgc->ls.tcpClient[channel].remote_ip,4);
        pgc->ls.tcpServer->proto.tcp->remote_port = pgc->ls.tcpClient[channel].remote_port;

        espconn_sent(pgc->ls.tcpServer,tmpBuf,sizeof(tmpBuf));
    }
}

/****************************************************************
        FunctionName        :   Lan_dispatchTCPData.
        Description         :   parse and dispatch tcp cmd message.
        Add by Will.zhou     --2015-03-10
****************************************************************/
int32 ICACHE_FLASH_ATTR
Lan_dispatchTCPData(pgcontext pgc, ppacket prxBuf,/* ppacket ptxBuf,*/ int32 clientIndex)
{int i;
    int ret = 0;
    int32 sn;
    uint16 cmd;
    uint8 bytesOfLen;

    bytesOfLen = mqtt_num_rem_len_bytes(prxBuf->phead + 3);
    if(bytesOfLen<1 || bytesOfLen>4)
    {
         return 0;
    }

    cmd = prxBuf->phead[LAN_PROTOCOL_HEAD_LEN + LAN_PROTOCOL_FLAG_LEN + bytesOfLen]<<8 |
        prxBuf->phead[LAN_PROTOCOL_HEAD_LEN + LAN_PROTOCOL_FLAG_LEN + bytesOfLen + 1];
    if((cmd != GAGENT_LAN_CMD_BINDING) && (cmd != GAGENT_LAN_CMD_LOGIN)
        && (cmd != GAGENT_LAN_CMD_INFO)&& (cmd != GAGENT_LAN_CMD_HOSTPOTS)
        && (cmd != GAGENT_LAN_CMD_TEST) )
    {
        ret = Lan_checkAuthorization(pgc, clientIndex);
        if(0 == ret)
        {
            return ret;
        }
    }

    switch (cmd)
    {
        case GAGENT_LAN_CMD_BINDING:
            Lan_handlePasscode( pgc, prxBuf, clientIndex );
            break;
        case GAGENT_LAN_CMD_LOGIN:
            Lan_handleLogin( pgc, prxBuf,clientIndex );
            break;
        case GAGENT_LAN_CMD_CTL_93:
            prxBuf->type = SetPacketType( prxBuf->type,LAN_TCP_DATA_IN,1 );
            ParsePacket(prxBuf);
            if((prxBuf->pend - prxBuf->ppayload) > 0)
            {
                ret = prxBuf->pend - prxBuf->ppayload;
                sn = *(int32 *)(prxBuf->phead + LAN_PROTOCOL_HEAD_LEN + LAN_PROTOCOL_FLAG_LEN
                            + bytesOfLen + LAN_PROTOCOL_CMD_LEN);
                sn = HTONL(sn);
                Lan_SetClientAttrs(pgc, clientIndex, cmd, sn);
            }
            else
            {
                ret = 0;
            }
            break;
        case GAGENT_LAN_CMD_TRANSMIT:
            prxBuf->type = SetPacketType( prxBuf->type,LAN_TCP_DATA_IN,1 );
            prxBuf->type = SetPacketType( prxBuf->type,CLOUD_DATA_IN,0 );
            prxBuf->type = SetPacketType( prxBuf->type,LOCAL_DATA_IN,0 );
            ParsePacket(prxBuf);
            if((prxBuf->pend - prxBuf->ppayload) > 0)
            {
                ret = prxBuf->pend - prxBuf->ppayload;
                sn = 0;
                Lan_SetClientAttrs(pgc, clientIndex, cmd, sn);
            }
            else
            {
               ret = 0;
            }
            break;
        case GAGENT_LAN_CMD_HOSTPOTS:
            Lan_GetWifiHotspots( pgc,prxBuf,clientIndex );
            break;
        case GAGENT_LAN_CMD_LOG:
            break;
        case GAGENT_LAN_CMD_INFO:
            resetPacket(prxBuf);
            GAgent_Lan_SendDevInfo(pgc, prxBuf, clientIndex);
            break;
        case GAGENT_LAN_CMD_TICK:
            resetPacket(prxBuf);
            GAgent_Printf(GAGENT_INFO,"LAN TCP heartbeat...");
            Lan_AckHeartbeak(pgc, prxBuf, clientIndex);
            break;
        case GAGENT_LAN_CMD_TEST:
            Local_Ack2TcpClient( pgc,clientIndex);
            GAgent_ExitTest( pgc );
            break;
        default:
            break;
    }
    return ret;
}

/****************************************************************
        FunctionName        :   LAN_tcpClientInit.
        Description         :      init tcp clients.
        Add by Will.zhou     --2015-03-10
****************************************************************/
int32 ICACHE_FLASH_ATTR
LAN_tcpClientInit(pgcontext pgc)
{
    int32 i;

    for (i = 0; i < LAN_TCPCLIENT_MAX; i++)
    {
        os_memset(&(pgc->ls.tcpClient[i]), 0x0, sizeof(pgc->ls.tcpClient[i]));
        pgc->ls.tcpClient[i].fd = -1;
        pgc->ls.tcpClient[i].isLogin = LAN_CLIENT_LOGIN_FAIL;
        pgc->ls.tcpClient[i].timeout = 0;
        pgc->ls.tcpClient[i].remote_port = 0;
        os_memset(pgc->ls.tcpClient[i].remote_ip,0,sizeof(pgc->ls.tcpClient[i].remote_ip));
        pgc->ls.tcpClient[i].fd_isset = -1;
    }

    return  RET_SUCCESS;
}
/****************************************************************
        FunctionName        :   Lan_CreateTCPServer.
        Description         :      create tcp server.
        Add by Will.zhou     --2015-03-10
****************************************************************/
void ICACHE_FLASH_ATTR
Lan_CreateTCPServer(int32 *pFd, uint16 tcp_port)
{
    GAgent_CreateTcpServer( tcp_port );
}

