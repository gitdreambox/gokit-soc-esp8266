#include "gagent.h"
#include "lan.h"
#include "lanudp.h"
//#include "3rdlan.h"
#include "platform.h"

extern struct espconn udpbroespconn;
/****************************************************************
*       FunctionName    :   combination_broadcast_packet
*       Description     :   combination broadcast packet data.
*       return          :   0 success other error.
*       Add by Frank Liu   --2015-04-14
****************************************************************/
int32 ICACHE_FLASH_ATTR
combination_broadcast_packet(pgcontext pgc,u8* Udp_Broadcast,uint16 cmdWord)
{
    int32 i;
    int16 tmpDidLen = 0;
    int16 tmpMacLen = 0;
    int16 tmpPkLen = 0;
    int16 tmpFirmwareverLen = 0;
    int16 tmpMcuattrLen = 0;
    int8 strMacByte[3] = {0};
    int pos;
    if(NULL == pgc || NULL == Udp_Broadcast)
    {
        return RET_FAILED;
    }

    tmpDidLen = os_strlen(pgc->gc.DID);
    if(tmpDidLen > DID_LEN)
    {
       tmpDidLen = DID_LEN - 1;
    }

    tmpPkLen = os_strlen((char *)pgc->mcu.product_key);
    if(tmpPkLen > PK_LEN)
    {
        tmpPkLen = PK_LEN;
    }

    tmpFirmwareverLen=os_strlen(pgc->gc.FirmwareVer);
    if(tmpFirmwareverLen > FIRMWARELEN)
    {
        tmpFirmwareverLen = FIRMWARELEN;
    }

    tmpMcuattrLen = os_strlen((char *)pgc->mcu.mcu_attr);
    tmpMacLen = 6;

    if((tmpDidLen > (DID_LEN - 1)) || (tmpFirmwareverLen > FIRMWARELEN) \
        || (tmpMcuattrLen > LAN_PROTOCOL_MCU_ATTR_LEN) || (tmpPkLen > PK_LEN))
    {
        return RET_FAILED;
    }

    //protocolver
    *(uint32 *)Udp_Broadcast = HTONL(GAGENT_PROTOCOL_VERSION);

    //cmdword
    if((GAGENT_LAN_CMD_STARTUP_BROADCAST == cmdWord) || (GAGENT_LAN_CMD_REPLY_BROADCAST == cmdWord))
    {
        //varlen =flag(1b)+cmd(2b)+didlen(2b)+did(didLen)+maclen(2b)+mac+firwareverLen(2b)+firwarever+2+productkeyLen+mcu_attr
        Udp_Broadcast[4] =LAN_PROTOCOL_FLAG_LEN+LAN_PROTOCOL_CMD_LEN+sizeof(tmpDidLen)+strlen(pgc->gc.DID) \
        +sizeof(tmpMacLen)+tmpMacLen+sizeof(tmpFirmwareverLen)+strlen(pgc->gc.FirmwareVer)+sizeof(tmpPkLen)+strlen((char *)pgc->mcu.product_key)+LAN_PROTOCOL_MCU_ATTR_LEN;

        //flag
        Udp_Broadcast[5] = 0x00;

        //cmdword
        *(uint16 *)(Udp_Broadcast + 6) = HTONS(cmdWord);

        pos = 8;
        //didlen
        Udp_Broadcast[pos]=0x00;
        pos++;
        Udp_Broadcast[pos]=tmpDidLen;
        pos++;
        //did
        for(i=0;i<tmpDidLen;i++)
        {
            Udp_Broadcast[pos+i]=pgc->gc.DID[i];
        }
        pos += tmpDidLen;

        //maclen
        Udp_Broadcast[pos]=0x00;
        pos++;
        Udp_Broadcast[pos]=tmpMacLen;//macLEN
        pos++;
        //mac
        strMacByte[2] = 0;
        for(i=0;i<tmpMacLen;i++)
        {
            strMacByte[0] = pgc->minfo.szmac[i*2];
            strMacByte[1] = pgc->minfo.szmac[i*2 + 1];
            Udp_Broadcast[pos+i] = strtoul(strMacByte, NULL, 16);
        }

        pos += tmpMacLen;
        //firmwarelen
        Udp_Broadcast[pos]=0x00;
        pos++;
        Udp_Broadcast[pos]=os_strlen(pgc->gc.FirmwareVer);//firmwareVerLen
        pos++;

        //firmware
        os_memcpy( &Udp_Broadcast[pos],pgc->gc.FirmwareVer,tmpFirmwareverLen);
        pos += tmpFirmwareverLen;

        //productkeylen
        Udp_Broadcast[pos]=0x00;
        pos++;
        Udp_Broadcast[pos]=os_strlen((char *)pgc->mcu.product_key);
        pos++;
        //productkey
        os_memcpy(&Udp_Broadcast[pos],pgc->mcu.product_key,tmpPkLen);
        pos += tmpPkLen;

        //mcu attr
        os_memcpy(&Udp_Broadcast[pos],pgc->mcu.mcu_attr,LAN_PROTOCOL_MCU_ATTR_LEN);
        pos += LAN_PROTOCOL_MCU_ATTR_LEN;
    }
    else if(GAGENT_LAN_CMD_AIR_BROADCAST == cmdWord)
    {
        //varlen =flag(1b)+cmd(2b)+didlen(2b)+did(didLen)+maclen(2b)+mac+firwareverLen(2b)+firwarever+2+productkeyLen
        Udp_Broadcast[4] =LAN_PROTOCOL_FLAG_LEN+LAN_PROTOCOL_CMD_LEN+sizeof(tmpDidLen)+os_strlen(pgc->gc.DID) \
        +sizeof(tmpMacLen)+tmpMacLen+sizeof(tmpPkLen)+os_strlen((char *)pgc->mcu.product_key);

        //flag
        Udp_Broadcast[5] = 0x00;

        //cmdword
        *(uint16 *)(Udp_Broadcast + 6) = HTONS(cmdWord);

        pos = 8;
        //maclen
        Udp_Broadcast[pos]=0x00;
        pos++;
        Udp_Broadcast[pos]=tmpMacLen;//macLEN
        pos++;
        //mac
        strMacByte[2] = 0;
        for(i=0;i<tmpMacLen;i++)
        {
            strMacByte[0] = pgc->minfo.szmac[i*2];
            strMacByte[1] = pgc->minfo.szmac[i*2 + 1];
            Udp_Broadcast[pos+i] = strtoul(strMacByte, NULL, 16);
        }

        pos += tmpMacLen;

        //productkeylen
        Udp_Broadcast[pos]=0x00;
        pos++;
        Udp_Broadcast[pos]=os_strlen((char *)pgc->mcu.product_key);
        pos++;
        //productkey
        os_memcpy(&Udp_Broadcast[pos],pgc->mcu.product_key,tmpPkLen);
        pos += tmpPkLen;

        //didlen
        Udp_Broadcast[pos]=0x00;
        pos++;
        Udp_Broadcast[pos]=tmpDidLen;
        pos++;

        //did
        for(i=0;i<tmpDidLen;i++)
        {
            Udp_Broadcast[pos + i]=pgc->gc.DID[i];
        }
        pos += tmpDidLen;
    }
    else
    {
        return RET_FAILED;
    }

    return RET_SUCCESS;
}


/****************************************************************
*       FunctionName    :   send_broadCastPacket
*       Description     :   Send BroadCast data.
*       Add by Frank Liu   --2015-04-14
****************************************************************/
void ICACHE_FLASH_ATTR
send_broadCastPacket(pgcontext pgc,uint8* ptxBuf,uint16 cmdWord)
{
    int32 ret = 0;
    int32 len = 0;

    resetPacket(pgc->rtinfo.Rxbuf);

    if(NULL == ptxBuf)
    {
        return;
    }

    ret = combination_broadcast_packet(pgc,ptxBuf,cmdWord);
    if((RET_SUCCESS == ret) && (INVALID_SOCKET != pgc->ls.udpBroadCastServerFd))
    {
        len = ptxBuf[4] + LAN_PROTOCOL_HEAD_LEN + 1;
        ret = espconn_sent(&udpbroespconn,ptxBuf,len);
        GAgent_Printf( GAGENT_INFO,"Send BroadCast datalen=%d cmd=%04X ret :%d",len,cmdWord,ret );
    }

    return ;
}

static void ICACHE_FLASH_ATTR
Lan_Upload_AllApp(pgcontext pgc, ppacket pTxBuf)
{
    uint8 i = 0;

    for(i = 0; i < LAN_TCPCLIENT_MAX; i++)
    {
        if(pgc->ls.tcpClient[i].fd > 0 &&
            LAN_CLIENT_LOGIN_SUCCESS == pgc->ls.tcpClient[i].isLogin)
        {
            os_memcpy(pgc->ls.tcpServer->proto.tcp->remote_ip,pgc->ls.tcpClient[i].remote_ip,4);
            pgc->ls.tcpServer->proto.tcp->remote_port = pgc->ls.tcpClient[i].remote_port;
            espconn_sent( pgc->ls.tcpServer, pTxBuf->phead, pTxBuf->pend - pTxBuf->phead );
        }
    }
}

void ICACHE_FLASH_ATTR Lan_sendTcpData(pgcontext pgc, int32 fd, uint16 cmd, int32 sn, ppacket pTxBuf)
{
    varc sendvarc;
    uint32 dataLen;
    int i;
    int offset = 0;

    /* protocol(4B) | varlen(xB) | flag(1B) | cmd(2B) | p0(xB)  */
    dataLen = pTxBuf->pend - pTxBuf->ppayload + LAN_PROTOCOL_CMD_LEN + LAN_PROTOCOL_FLAG_LEN;
    /* while cmd == 0x93/94,sn follow with cmd */
    if(GAGENT_LAN_CMD_CTL_93 == cmd || GAGENT_LAN_CMD_CTLACK_94 == cmd)
    {
        dataLen += LAN_PROTOCOL_SN_LEN;
    }
    sendvarc = Tran2varc(dataLen);
    pTxBuf->phead = pTxBuf->ppayload - sendvarc.varcbty -
            LAN_PROTOCOL_HEAD_LEN - LAN_PROTOCOL_CMD_LEN - LAN_PROTOCOL_FLAG_LEN;

    /* while cmd == 0x93/94,sn follow with cmd */
    if(GAGENT_LAN_CMD_CTL_93 == cmd || GAGENT_LAN_CMD_CTLACK_94 == cmd)
    {
        pTxBuf->phead -= LAN_PROTOCOL_SN_LEN;
    }
    offset = 0;
    /* protocol */
    pTxBuf->phead[0] = 0x00;
    pTxBuf->phead[1] = 0x00;
    pTxBuf->phead[2] = 0x00;
    pTxBuf->phead[3] = 0x03;
    offset += LAN_PROTOCOL_HEAD_LEN;
    /* varLen */
    for(i=0;i<sendvarc.varcbty;i++)
    {
        pTxBuf->phead[offset] = sendvarc.var[i];
        offset++;
    }
    /* flag */
    pTxBuf->phead[offset] = 0x00;
    offset += 1;

    /* cmd */
    pTxBuf->phead[offset] = (cmd>>8)&0xff;
    pTxBuf->phead[offset+1] = cmd&0xff;
    offset += LAN_PROTOCOL_CMD_LEN;
    switch(cmd)
    {
        case GAGENT_LAN_CMD_CTL_93:
            sn = 0;
            *(int32 *)(pTxBuf->phead + offset) = HTONL(sn);
            offset += LAN_PROTOCOL_SN_LEN;
            fd = -1;
            break;
        case GAGENT_LAN_CMD_CTLACK_94:
            *(int32 *)(pTxBuf->phead + offset) = HTONL(sn);
            offset += LAN_PROTOCOL_SN_LEN;
            break;
        case GAGENT_LAN_CMD_TRANSMIT_91:
            break;
        default:
            /* not support ,return directly */
            return ;
    }

    if(fd >= 0)
    {
        os_memcpy(pgc->ls.tcpServer->proto.tcp->remote_ip, pgc->rtinfo.stChannelAttrs.lanClient.remote_ip, 4);
        pgc->ls.tcpServer->proto.tcp->remote_port = pgc->rtinfo.stChannelAttrs.lanClient.remote_port;
        espconn_sent( pgc->ls.tcpServer, pTxBuf->phead, pTxBuf->pend - pTxBuf->phead );
    }
    else
    {
        Lan_Upload_AllApp(pgc, pTxBuf);
    }

    return ;
}

void ICACHE_FLASH_ATTR
Lan_SetClientAttrs(pgcontext pgc, int32 clientIndex, uint16 cmd, int32 sn)
{
    pgc->ls.srcAttrs.sn = sn;
    pgc->ls.srcAttrs.fd = pgc->ls.tcpClient[clientIndex].fd;
    pgc->ls.srcAttrs.remote_port = pgc->ls.tcpClient[clientIndex].remote_port;
    os_memcpy(pgc->ls.srcAttrs.remote_ip, pgc->ls.tcpClient[clientIndex].remote_ip,4);
    pgc->ls.srcAttrs.cmd = cmd;

}

void ICACHE_FLASH_ATTR
Lan_ClearClientAttrs(pgcontext pgc, stLanAttrs_t *client)
{
    if(NULL != client)
    {
        client->sn = 0;
        client->fd = INVALID_SOCKET;
        client->remote_port = 0;
        //client->remote_ip[4]={0};
        client->cmd = 0;
    }
}

/****************************************************************
        FunctionName    :   GAgent_Lan_SendTcpData
        Description     :   send buf data to TCP client.
        return          :   void
        Add by Will.zhou     --2015-03-17
****************************************************************/
void ICACHE_FLASH_ATTR
GAgent_Lan_SendTcpData(pgcontext pgc,ppacket pTxBuf)
{
    int32 fd;
    uint16 cmd;
    int32 sn;

    fd = pgc->rtinfo.stChannelAttrs.lanClient.fd;
    cmd = pgc->rtinfo.stChannelAttrs.lanClient.cmd;
    sn = pgc->rtinfo.stChannelAttrs.lanClient.sn;

    Lan_sendTcpData(pgc, fd, cmd, sn, pTxBuf);

    return ;
}
/****************************************************************
        FunctionName        :   CreateUDPBroadCastServer.
        Description         :   create udp broadcast server.
        Add by Frank Liu     --2015-04-22
****************************************************************/
void ICACHE_FLASH_ATTR
CreateUDPBroadCastServer(pgcontext pgc)
{

    if(NULL == pgc)
    {
        return;
    }
    pgc->ls.onboardingBroadCastTime = SEND_UDP_DATA_TIMES;

    Lan_CreateUDPBroadCastServer(&(pgc->ls.udpBroadCastServerFd),LAN_UDP_BROADCAST_SERVER_PORT);
}
/****************************************************************
        FunctionName        :   DestroyUDPBroadCastServer.
        Description         :   destroy udp broadcast server.
        Add by Frank Liu     --2015-04-22
****************************************************************/
void ICACHE_FLASH_ATTR DestroyUDPBroadCastServer(pgcontext pgc)
{
    if(NULL == pgc)
    {
        return;
    }
    if(INVALID_SOCKET != pgc->ls.udpBroadCastServerFd)
    {
        espconn_disconnect(pgc->ls.udpBroadcast);
        pgc->ls.udpBroadCastServerFd = INVALID_SOCKET;
    }
}
/****************************************************************
        FunctionName        :   GAgent_LanTick.
        Description         :   check clients whether it is timeout.
        Add by Will.zhou     --2015-03-10
****************************************************************/
void ICACHE_FLASH_ATTR
GAgent_LanTick( pgcontext pgc,uint32 dTime_s )
{
    int32 i;
    uint16 GAgentStatus = 0;
    uint32 GAgentConStatus = 0;
    uint8 *ptxBuf = NULL;
    static uint8 count = 0;

    if( RET_SUCCESS == pgc->rtinfo.webconfigflag )
    {
        count++;
        if( 3 == count )
        {
            count = 0;
            pgc->rtinfo.webconfigflag = RET_FAILED;
            GAgent_Printf( GAGENT_INFO,"webconfig ssid:%s key:%s",pgContextData->gc.wifi_ssid,pgContextData->gc.wifi_key );
            espconn_disconnect(pgContextData->ls.webTcpServer);
            pgContextData->ls.tcpWebConfigFd = INVALID_SOCKET;
            GAgent_DRVWiFi_APModeStop( pgContextData );
            GAgent_DevReset( );
            GAgent_DevCheckWifiStatus( WIFI_MODE_ONBOARDING,0 );
            GAgent_DRVWiFi_StationCustomModeStart(pgContextData->gc.wifi_ssid, pgContextData->gc.wifi_key, WIFI_MODE_STATION);
        }
    }

    if(pgc->mcu.passcodeTimeout > 0 &&
        ((pgc->rtinfo.GAgentStatus & WIFI_MODE_BINDING) == WIFI_MODE_BINDING))
    {
        GAgent_Printf(GAGENT_DEBUG,"enable bind time remain %d s",pgc->mcu.passcodeTimeout);
        pgc->mcu.passcodeTimeout--;
        if(0 == pgc->mcu.passcodeTimeout)
        {
            pgc->mcu.passcodeTimeout = -1;
            GAgent_SetWiFiStatus( pgc,WIFI_MODE_BINDING,0);
        }
    }

    for(i = 0; i < LAN_TCPCLIENT_MAX; i++)
    {
        if(pgc->ls.tcpClient[i].fd > 0)
        {
            if( pgc->ls.tcpClient[i].timeout <= dTime_s )
            {
                os_memcpy(pgc->ls.tcpServer->proto.tcp->remote_ip,pgc->ls.tcpClient[i].remote_ip,4);
                pgc->ls.tcpServer->proto.tcp->remote_port = pgc->ls.tcpClient[i].remote_port;
                espconn_disconnect( pgc->ls.tcpServer);
                pgc->ls.tcpClient[i].fd = INVALID_SOCKET;
                os_memset(pgc->ls.tcpClient[i].remote_ip,0,4);
                pgc->ls.tcpClient[i].remote_port = 0;

                if( LAN_CLIENT_LOGIN_SUCCESS == pgc->ls.tcpClient[i].isLogin)
                {
                     if(pgc->ls.tcpClientNums > 0)
                         pgc->ls.tcpClientNums--;

                     if(0 == (pgc->ls.tcpClientNums + pgc->rtinfo.waninfo.wanclient_num))
                     {
                       GAgent_SetWiFiStatus( pgc,WIFI_CLIENT_ON,0 );
                     }
                }
            }
            else
            {
                pgc->ls.tcpClient[i].timeout -= dTime_s;
            }
        }
    }

    GAgentStatus = pgc->rtinfo.GAgentStatus;
    GAgentConStatus = pgc->gc.flag;

    if((GAgentStatus&WIFI_STATION_CONNECTED) == WIFI_STATION_CONNECTED)
    {
        if(pgc->ls.onboardingBroadCastTime > 0)
        {
            if( pgc->ls.udpBroadCastServerFd<=0 )
            {
                if( pgc->ls.udp3rdCloudFd>0 )
                {
                    //close( pgc->ls.udp3rdCloudFd );
                    pgc->ls.udp3rdCloudFd = INVALID_SOCKET;
                }
                CreateUDPBroadCastServer( pgc );
            }
            pgc->ls.onboardingBroadCastTime--;
            resetPacket(pgc->rtinfo.Rxbuf);
            ptxBuf = pgc->rtinfo.Rxbuf->phead;
            GAgent_Printf( GAGENT_INFO,"onboardingBroadCastTime:%d",pgc->ls.onboardingBroadCastTime );
            if(pgc->rtinfo.firstStartUp)
            {
                GAgent_Printf( GAGENT_INFO,"UDP BC firstStartUp...");
                send_broadCastPacket(pgc,ptxBuf,GAGENT_LAN_CMD_STARTUP_BROADCAST);
                if( 0==(pgc->ls.onboardingBroadCastTime) )
                {
                    GAgent_Printf( GAGENT_INFO,"hello ..." );
                }
            }
            if((GAgentConStatus & XPG_CFG_FLAG_CONFIG) == XPG_CFG_FLAG_CONFIG)
            {
                GAgent_Printf( GAGENT_INFO,"UDP BC Config success...");
                send_broadCastPacket(pgc,ptxBuf,GAGENT_LAN_CMD_AIR_BROADCAST);
            }

            if( 0==(pgc->ls.onboardingBroadCastTime) )
            {
                pgc->rtinfo.firstStartUp = 0;
                GAgentConStatus &= (~XPG_CFG_FLAG_CONFIG);
                pgc->gc.flag = GAgentConStatus;
                DestroyUDPBroadCastServer(pgc);
                if( 1==pgc->rtinfo.waninfo.Cloud3Flag )
                {
                    if( 0==os_strcmp(pgc->gc.cloud3info.cloud3Name,"jd" ) )
                    {
                        if( pgc->ls.udp3rdCloudFd <=0 )
                        {
                            GAgent_Printf( GAGENT_DEBUG,"GAgent will Open JD discover protocol.");
                            GAgent_Printf( GAGENT_DEBUG,"3rdCloud Name:%s",pgc->gc.cloud3info.cloud3Name);
                            GAgent_Printf( GAGENT_DEBUG,"3rdCloud UUID:%s",pgc->gc.cloud3info.jdinfo.product_uuid );
                            //pgc->ls.udp3rdCloudFd = Socket_CreateUDPServer_JD( LAN_UDP_SERVER_PORT_JD );
                        }
                    }
                }
                GAgent_DevSaveConfigData( &(pgc->gc) );
            }
        }
    }
}

uint32 ICACHE_FLASH_ATTR
GAgent_Lan_Handle(pgcontext pgc, ppacket prxBuf,int32 len)
{
    int i =0;
    int fd =0;
    int ret =0;

//    GAgent_DoTcpWebConfig( pgc );
//    Lan_udpDataHandle(pgc, prxBuf, len);
//    Lan_TcpServerHandler(pgc);
//    GAgent3rdLan_Handle( pgc );
    for(i = 0;i < LAN_TCPCLIENT_MAX; i++)
    {
        fd = pgc->ls.tcpClient[i].fd;
        if(fd <= 0)
            continue;
        if(1 == pgc->ls.tcpClient[i].fd_isset)
        {
            pgc->ls.tcpClient[i].fd_isset = 0;
            ret = Lan_tcpClientDataHandle(pgc, i, prxBuf, /* ptxBuf,*/ len);
            if(ret > 0)
            {
                dealPacket(pgc, prxBuf);
//                Lan_ClearClientAttrs(pgc, &pgc->ls.srcAttrs);
//                clearChannelAttrs(pgc);
            }
        }
    }
    return 0;
}

/****************************************************************
        FunctionName        :   GAgent_LANInit.
        Description         :   init clients socket and create tcp/udp server.
        Add by Will.zhou     --2015-03-10
****************************************************************/
void ICACHE_FLASH_ATTR
GAgent_LANInit(pgcontext pgc)
{
    LAN_tcpClientInit(pgc);
    Lan_ClearClientAttrs(pgc, &pgc->ls.srcAttrs);
    LAN_InitSocket(pgc);
//    GAgent_SetWiFiStatus( pgc,WIFI_MODE_BINDING,1 );  //enable Bind
    pgc->ls.broResourceNum = 0;//enable public broadcast resource
    GAgent_SetWiFiStatus( pgc,WIFI_CLIENT_ON,0 );
}

/****************************************************************
        FunctionName        :   LAN_InitSocket.
        Description         :      create tcp/udp server.
        Add by Will.zhou     --2015-03-10
****************************************************************/
int32 ICACHE_FLASH_ATTR
LAN_InitSocket(pgcontext pgc)
{
    pgc->ls.tcpServerFd = INVALID_SOCKET;
    pgc->ls.udpServerFd = INVALID_SOCKET;
    pgc->ls.udpBroadCastServerFd = INVALID_SOCKET;
    pgc->ls.tcpClientNums = 0;
    pgc->ls.onboardingBroadCastTime = SEND_UDP_DATA_TIMES;
    pgc->ls.startupBroadCastTime = SEND_UDP_DATA_TIMES;
    pgc->ls.tcpWebConfigFd = INVALID_SOCKET;

    Lan_CreateTCPServer(&(pgc->ls.tcpServerFd), GAGENT_TCP_SERVER_PORT);
    Lan_CreateUDPServer(&(pgc->ls.udpServerFd), LAN_UDP_SERVER_PORT );
    CreateUDPBroadCastServer(pgc);//startup broadcast
    return 0;
}


