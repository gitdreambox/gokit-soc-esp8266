#include "gagent.h"
#include "mqttxpg.h"
#include "mqttbase.h"
#include "mqttxpg.h"
#include "mqttlib.h"
#include "cloud.h"

static mqtt_broker_handle_t g_stMQTTBroker;

//int32 MQTT_readPacket( int32 socketid,ppacket pbuf,int32 bufferLen )
//{
//    int32 bytes_rcvd;
//    uint8_t *pData;
//    int32 messageLen;
//    int32 varLen;
//    int32 packet_length;

//    memset(pbuf->phead, 0, bufferLen);
//    pData = pbuf->phead;
//    bytes_rcvd = recv(socketid, pData , bufferLen, 0);
//    if((bytes_rcvd) <= 0)
//    {
//        MQTTclose_socket( &g_stMQTTBroker );
//        return -1;
//    }
//    //pData = packetBuffer + 0;
//    pbuf->pend=pbuf->phead+bytes_rcvd;
//
//    messageLen = mqtt_parse_rem_len(pData);
//    varLen = mqtt_num_rem_len_bytes(pData);
//    if(varLen<1 || varLen>4)
//    {
//         return -3;
//    }
//    packet_length = varLen + messageLen + 1;

//    if (bytes_rcvd < packet_length)
//    {
//        GAgent_Printf(GAGENT_INFO, " packet length too long %s:%d ", __FUNCTION__, __LINE__);
//        return -3;
//    }
//
//    return bytes_rcvd;
//}
//int32 MQTTclose_socket(mqtt_broker_handle_t* broker)
//{
//    int32 fd = broker->socketid;
//    close(fd);
//    broker->socketid = -1;
//    return 0;
//}

/**************************************************
 *
 *
 *      return :0
 *
 ***************************************************/
static int32 ICACHE_FLASH_ATTR
init_subTopic( mqtt_broker_handle_t* broker ,pgcontext pgc,char *Sub_TopicBuf,int32 Mqtt_Subflag)
{
    int32 productKeyLen=0,DIdLen=0;

    DIdLen = os_strlen( pgc->gc.DID );
    switch(Mqtt_Subflag)
    {
    case 1:
        //4.6
        productKeyLen = os_strlen( (const int8 *)pgc->mcu.product_key );
        os_memcpy( Sub_TopicBuf,"ser2cli_noti/",os_strlen("ser2cli_noti/") );
        os_memcpy( Sub_TopicBuf+os_strlen("ser2cli_noti/"),pgc->mcu.product_key,productKeyLen );
        Sub_TopicBuf[os_strlen("ser2cli_noti/")+productKeyLen] = '\0';
        break;
    case 2:
        // 4.7 4.9
        os_memcpy( Sub_TopicBuf,"ser2cli_res/",os_strlen("ser2cli_res/") );
        os_memcpy( Sub_TopicBuf+os_strlen("ser2cli_res/"),pgc->gc.DID,DIdLen );
        Sub_TopicBuf[os_strlen("ser2cli_res/")+DIdLen] = '\0';
        break;
    case 3:
        // 4.13
        os_memcpy(Sub_TopicBuf,"app2dev/",os_strlen("app2dev/") );
        os_memcpy(Sub_TopicBuf+os_strlen("app2dev/"),pgc->gc.DID,DIdLen );
        Sub_TopicBuf[os_strlen("app2dev/")+DIdLen] = '/';
        Sub_TopicBuf[os_strlen("app2dev/")+DIdLen+1] = '#';
        Sub_TopicBuf[os_strlen("app2dev/")+DIdLen+2] = '\0';
        break;
    default:
        break;
    }

    return 0;
}
int32 ICACHE_FLASH_ATTR
Mqtt_DoSubTopic( pgcontext pgc,int16 mqttstatus )
{
    int8 ret =0;
    ret = Mqtt_SubLoginTopic( &g_stMQTTBroker,pgc,mqttstatus );
    return ret;
}
/*******************************************************
 *
 * 返回成功订阅Topic的个数
 *
 ********************************************************/
int32 ICACHE_FLASH_ATTR
Mqtt_SubLoginTopic( mqtt_broker_handle_t *LoginBroker,pgcontext pgc,int16 mqttstatus )
{
    char Sub_TopicBuf[100];
    char Topic[100];

    os_memset(Sub_TopicBuf,0,100);
    os_memset(Topic,0,100);

    switch(mqttstatus)
    {
    case MQTT_STATUS_REQ_LOGINTOPIC1:
        init_subTopic(LoginBroker,pgc,Sub_TopicBuf,1);
        if(mqtt_subscribe( LoginBroker,Sub_TopicBuf,&(pgc->rtinfo.waninfo.mqttMsgsubid) )==1)
        {
            os_sprintf(Topic,"LOGIN sub topic1 is:%s",Sub_TopicBuf);
            GAgent_Printf(GAGENT_INFO,Topic);
            GAgent_Printf(GAGENT_INFO,"MQTT_STATUS_LOGINTOPIC1");
        }
        break;
    case MQTT_STATUS_REQ_LOGINTOPIC2:
        init_subTopic(LoginBroker,pgc,Sub_TopicBuf,2);
        if(mqtt_subscribe( LoginBroker,Sub_TopicBuf,&(pgc->rtinfo.waninfo.mqttMsgsubid) )==1)
        {
            os_sprintf(Topic,"LOGIN T2 sub topic is:%s",Sub_TopicBuf);
            GAgent_Printf(GAGENT_INFO,Topic);
            GAgent_Printf(GAGENT_INFO,"MQTT_STATUS_LOGINTOPIC2");
        }
        break;
    case MQTT_STATUS_REQ_LOGINTOPIC3:
        init_subTopic(LoginBroker,pgc,Sub_TopicBuf,3);
        if(mqtt_subscribe( LoginBroker,Sub_TopicBuf,&(pgc->rtinfo.waninfo.mqttMsgsubid) )==1)
        {
            os_sprintf(Topic,"LOGIN T3 sub topic is:%s",Sub_TopicBuf);
            GAgent_Printf(GAGENT_INFO,Topic);
            GAgent_Printf(GAGENT_INFO,"MQTT_STATUS_LOGINTOPIC3");
        }
        break;
    default:
        break;
    }

    return 0;
}
int32 ICACHE_FLASH_ATTR
Mqtt_SendPiece(pgcontext pgc, ppacket pp)
{int i;
    int len = 0;
    int ret = 0;
    if( ((pgc->rtinfo.GAgentStatus)&WIFI_CLOUD_CONNECTED)  != WIFI_CLOUD_CONNECTED )
    {
        GAgent_Printf( GAGENT_INFO,"GAgent not in WIFI_CLOUD_CONNECTED,can't send data to cloud ");
        return RET_FAILED;
    }
    len = g_stMQTTBroker.mqttsend(g_stMQTTBroker.socketid, pp->ppayload, pp->pend - pp->ppayload);
    return len;
}

/***********************************************************
 *
 *   return :    0 success ,1 error
 *
 *************************************************************/
int32 ICACHE_FLASH_ATTR
Mqtt_SendConnetPacket( mqtt_broker_handle_t *pstBroketHandle, int32 socketid,const int8* username,const int8* password )
{
    int32 ret;

    if( (username==NULL) || (password==NULL) )// 匿名登录 GAgent V4 will not runing this code.
    {
        return 1;
    }
    else // 用户登录
    {
        mqtt_init( pstBroketHandle,username );
        mqtt_init_auth(pstBroketHandle,username,password );
    }
    mqtt_set_alive(pstBroketHandle, CLOUD_MQTT_SET_ALIVE);
    pstBroketHandle->socketid = socketid;
    pstBroketHandle->mqttsend = send_packet;

    ret = mqtt_connect(pstBroketHandle);
    if (ret != 0)
    {
        GAgent_Printf(GAGENT_WARNING,"mqtt send connect packet is failed with:%d", ret);
        Cloud_M2M_InitSocket( pgContextData->rtinfo.waninfo.m2m_socketid ,pgContextData->gc.m2m_ip ,pgContextData->minfo.m2m_Port,0 );
        return 1;
    }
    GAgent_Printf(GAGENT_INFO, "Mqtt_SendConnetPacket OK, write:%d", ret);
    return 0;
}

int32 ICACHE_FLASH_ATTR
Mqtt_Login2Server(  int32 socketid,const uint8 *username,const uint8 *password )
{
    if( Mqtt_SendConnetPacket( &g_stMQTTBroker,socketid,(const int8*)username,(const int8*)password ) == 0)
    {
        GAgent_Printf(GAGENT_INFO," Mqtt_SendConnetPacket OK!");
        return 0;
    }
    GAgent_Printf(GAGENT_INFO," Mqtt_SendConnetPacket NO!");

    return 1;
}

void ICACHE_FLASH_ATTR
MQTT_HeartbeatTime( void )
{
    mqtt_ping(&g_stMQTTBroker);
}

int32 ICACHE_FLASH_ATTR
Mqtt_IntoRunning( pgcontext pgc )
{
    mqtt_ping(&g_stMQTTBroker);
    Mqtt_ReqOnlineClient(pgc);
    return 0;
}
 //发送日志
 //目前使用全局变量交换数据
 //pmbroker:MQTT Broker, lm:Log Manager
void ICACHE_FLASH_ATTR
SendLog( /*mqtt_broker_handle_t *pmbroker, LogMan *lm*/ )
{

    return;
}

void ICACHE_FLASH_ATTR
Log2Cloud( pgcontext pgc )
{
    //SendLog(&g_stMQTTBroker, pgc->logman/*g_Xpg_GlobalVar.logman*/);
    return;
}
int32 ICACHE_FLASH_ATTR
MQTT_SendData( pgcontext pgc, int8 *szDID, ppacket pbuf, int32 buflen )
{
    uint8 *sendpack=NULL;
    int32 i=0,sendpacklen=0,headlen=0;
    int16 varlen=0;
    int32 totallen = 0;
    volatile varc sendvarc;
    int8 msgtopic[64]= {0};
    int32 didlen=0;
    uint16 cmd = pgc->rtinfo.stChannelAttrs.cloudClient.cmd;
    int32 sn = pgc->rtinfo.stChannelAttrs.cloudClient.sn;
    int8 *clienid = pgc->rtinfo.stChannelAttrs.cloudClient.phoneClientId;
    uint8 pos = 0;
    int32 clienIdLen;

    didlen = strlen(szDID);
    if(didlen!=22)
    {
        return -1;
    }

    if( ((pgc->rtinfo.GAgentStatus)&WIFI_CLOUD_CONNECTED)  != WIFI_CLOUD_CONNECTED )
    {
        GAgent_Printf( GAGENT_INFO,"GAgent not in WIFI_CLOUD_CONNECTED,can't send data to cloud ");
        return -1;
    }

    varlen = 1 + 2;
    if(0x0093 == cmd || 0x0094 == cmd)
    {
        varlen += sizeof(sn);
    }

    /* protocolVer(4B)+varLen(1~4B)+/ flag(1B)+cmd(2B)+P0 / */
    if(pgc->rtinfo.file.using == 1)
    {
        totallen = varlen + pgc->rtinfo.file.totalsize;
    }
    else
    {
        totallen = varlen + buflen;
    }
    varlen += buflen;
    sendvarc = Tran2varc(totallen);
    sendpacklen = 4 + sendvarc.varcbty + varlen;
    headlen = sendpacklen - buflen;

    sendpack = ( (pbuf->ppayload)-headlen );
    //protocolVer
    sendpack[0] = 0x00;
    sendpack[1] = 0x00;
    sendpack[2] = 0x00;
    sendpack[3] = 0x03;
    //varLen
    for(i=0;i<sendvarc.varcbty;i++)
    {
        sendpack[4+i] = sendvarc.var[i];
    }
     //flag
    sendpack[4+sendvarc.varcbty] = 0x00;
    //CMD
    *(uint16 *)&sendpack[4+sendvarc.varcbty+1] = HTONS(cmd);
    if(0x0093 == cmd || 0x0094 == cmd)
    {
        *(int32 *)&sendpack[4+sendvarc.varcbty+1 + 2] = HTONL(sn);
    }

    memcpy( msgtopic,"dev2app/",strlen("dev2app/"));
    pos += strlen("dev2app/");
    memcpy( msgtopic+strlen("dev2app/"),szDID,didlen );
    pos += didlen;
    if(0x0094 == cmd)
    {
        clienIdLen = strlen( (const int8 *)clienid);
        if(clienIdLen > 0)
        {
            msgtopic[pos] = '/';
            pos++;
            memcpy( msgtopic+pos,clienid,clienIdLen );
            pos+=clienIdLen;
        }
    }
    msgtopic[pos] = '\0';
    sendpack -= pos;
    memcpy(sendpack, msgtopic, pos);
    GAgent_Printf(GAGENT_DUMP,"------SEND TO Cloud ------\r\n");
    for(i=0;i<sendpacklen;i++)
        GAgent_Printf(GAGENT_DUMP," %02X",sendpack[i] );
    totallen += 4;
    totallen += sendvarc.varcbty;
    pbuf->phead = sendpack;
    if(pgc->rtinfo.file.using == 1)
    {
        PubMsg( &g_stMQTTBroker, pbuf, 2, totallen);
    }
    else
    {
        PubMsg( &g_stMQTTBroker,pbuf, 0, 0);
    }

    return 0;
}

/********************************************************************
 *
 *  FUNCTION   : Mqtt send request packbag to server ask online client
 *             add by alex.lin --2014-12-11
 ********************************************************************/
void ICACHE_FLASH_ATTR
Mqtt_ReqOnlineClient(pgcontext pgc)
{
     char req_buf[6] = {0x00,0x00,0x00,0x03,0x02,0x0f};
    /* added */
    ppacket pp = pgc->rtinfo.Txbuf;
    uint8 *p = NULL;
    resetPacket(pp);
    p = pp->phead;
    memcpy(p, "cli2ser_req", strlen("cli2ser_req"));
    p += strlen("cli2ser");
    pp->ppayload = p;

    memcpy(p, req_buf, sizeof(req_buf));
    p += sizeof(req_buf);
    pp->pend = p;
    PubMsg(&g_stMQTTBroker, pp, 0, 0);
    /* end */
    /* PubMsg( &g_stMQTTBroker,"cli2ser_req",req_buf,6,0, 0); */
}

/********************************************************************
 *
 *  FUNCTION   : Mqtt send request packbag to server ask online client
 *               res.
 *   buf       : mqtt msg payload.
 *   return    : none.
 *             add by alex.lin --2014-12-11
 ********************************************************************/
void ICACHE_FLASH_ATTR
Mqtt_ResOnlineClient( pgcontext pgc,char *buf,int32 buflen)
{
    u16 *pWanclient_num;
    u16 wanclient_num=0;
    u16 lanclient_num=0;

    pWanclient_num = (u16*)&buf[6];
    wanclient_num = HTONS( *pWanclient_num );
    pgc->rtinfo.waninfo.wanclient_num = wanclient_num;
    lanclient_num = pgc->ls.tcpClientNums;

    if( 0 != wanclient_num)
    {
        GAgent_SetWiFiStatus( pgc,WIFI_CLIENT_ON,1 );
    }
    else if(0 == (wanclient_num + lanclient_num))
    {
        GAgent_SetWiFiStatus( pgc,WIFI_CLIENT_ON,0 );
    }

    GAgent_Printf(GAGENT_INFO,"wanclient_num = %d",wanclient_num );
    return ;
}
void ICACHE_FLASH_ATTR
Mqtt_Ack2Cloud( pgcontext pgc,uint8 *pPhoneClient,uint8* szDID,uint8 *pData,uint32 datalen )
{
    uint8 msgtopic[60]={0};
    uint8 pos=0;
    ppacket pp = pgc->rtinfo.Txbuf;
    uint8 *p = NULL;

    os_memcpy( msgtopic+pos,"dev2app/",os_strlen("dev2app/"));
    pos+=os_strlen( "dev2app/" );
    os_memcpy( msgtopic+pos,szDID,os_strlen((const int8 *)szDID) );
    pos+=os_strlen((const int8*)szDID);
    msgtopic[pos] = '/';
    pos++;
    os_memcpy( msgtopic+pos,pPhoneClient,os_strlen( (const int8 *)pPhoneClient) );
    pos+=os_strlen( (const int8*)pPhoneClient );
    msgtopic[pos] = '\0';
    GAgent_Printf( GAGENT_DEBUG,"msgtopic:%s  :len=%d",msgtopic,os_strlen( (const int8 *)msgtopic) );

    resetPacket(pp);
    p = pp->phead;
    memcpy(p, msgtopic, pos);
    p += pos;
    pp->ppayload = p;
    memcpy(p, pData, datalen);
    p += datalen;
    pp->pend = p;
    PubMsg(&g_stMQTTBroker, pp, 0, 0);
    //TODO ack buf.
    /* PubMsg( &g_stMQTTBroker,( const int8*)msgtopic,( int8* )pData,datalen,0, 0); */
    return ;
}
int32 ICACHE_FLASH_ATTR
Mqtt_DispatchPublishPacket( pgcontext pgc,u8 *packetBuffer,int32 packetLen )
{
    u8 firmwareType;
    u8 topic[128];
    int32 topiclen;
    u8 *pHiP0Data;
    int32 HiP0DataLen;
    int32 i;
    u8 varlen=0;
    u8  clientid[PHONECLIENTID + 1];
    //int32 clientidlen = 0;
    u8 *pTemp;
    u16 cmd;
    int32 sn;
    int32 u32Sn;
    int32 buf_pos;
    u16 *pcmd=NULL;

    topiclen = mqtt_parse_pub_topic(packetBuffer, topic);
    //HiP0DataLen = packetLen - topiclen;
    topic[topiclen] = '\0';

    HiP0DataLen = mqtt_parse_publish_msg(packetBuffer, &pHiP0Data);


    if(os_strncmp((const int8*)topic,"app2dev/",os_strlen("app2dev/"))==0)
    {

        varlen = mqtt_num_rem_len_bytes( pHiP0Data+3 );
        if(varlen<1 || varlen>4)
        {
            return 0;
        }

        pcmd = (u16*)&pHiP0Data[4+varlen+1];
        cmd = HTONS( *pcmd );
        pTemp = &topic[os_strlen("app2dev/")];
        i = 0;
        while (*pTemp != '/')
        {
            i++;
            pTemp++;
        }

        pTemp ++; /* 跳过\/ */
        i=0;
        while (*pTemp != '\0' && i <= PHONECLIENTID)
        {
            clientid[i] = *pTemp;
            i++;
            pTemp++;
        }
        if(i > PHONECLIENTID)
        {
            /* should handle invalid phone client id.don't ack the cmd */
            i = PHONECLIENTID;
        }
        clientid[i]= '\0';
        os_strcpy( pgc->rtinfo.waninfo.phoneClientId ,(const int8*)clientid );
        pgc->rtinfo.waninfo.srcAttrs.cmd = cmd;
        os_memcpy( packetBuffer,pHiP0Data,HiP0DataLen );
        GAgent_Printf( GAGENT_INFO,"Cloud CMD =%04X",cmd );
        if( cmd==0x0093 )
        {
            buf_pos = 4+varlen+1+sizeof(cmd);
            os_memcpy(&u32Sn,pHiP0Data+buf_pos,4);
            //sn = *(int32 *)&pHiP0Data[4+varlen+1 + sizeof(cmd)];
            sn = HTONL(sn);
            Cloud_SetClientAttrs(pgc, clientid, cmd, sn);
        }
        else if( cmd == 0x0090 )
        {
            sn = 0;
            Cloud_SetClientAttrs(pgc, clientid, cmd, sn);
            Lan_ClearClientAttrs(pgc, &pgc->ls.srcAttrs);
        }
        return HiP0DataLen;
    }
    // 订阅最新固件响应
    else if(os_strncmp((const int8*)topic,"ser2cli_res/",os_strlen("ser2cli_res/"))==0)
    {

        pcmd = (u16*)&pHiP0Data[4];
        cmd = HTONS( *pcmd );
        // pHiP0Data消息体的指针
        // HiP0DataLen消息体的长度 packetBuffer
        switch(cmd)
        {
            /* V4.1 Don't use this cmd */
            case 0x020e:
                break;
            // wan client on line numbers res.
            case 0x0210:
                Mqtt_ResOnlineClient( pgc, (int8*)pHiP0Data, HiP0DataLen);
            break;
            case 0x0211:
                GAgent_Printf( GAGENT_DEBUG,"M2M cmd to check OTA!!! ");
                firmwareType = pHiP0Data[6];
                pgc->rtinfo.onlinePushflag = 1;
                if(firmwareType >= OTATYPE_INVALID || 0 == firmwareType)
                {
                    GAgent_Printf( GAGENT_WARNING,"invalid firmware type!");
                }
                pgc->rtinfo.OTATypeflag = firmwareType;
                if( 1 == firmwareType )
                {
                    GAgent_Printf(GAGENT_CRITICAL,"M2M push WIFI OTA!\n");
                }
                else
                {
                    GAgent_Printf(GAGENT_CRITICAL,"M2M push MCU OTA!\n");
                }
                pgc->rtinfo.waninfo.send2HttpLastTime = 0;
                GAgent_SetCloudConfigStatus( pgc, CLOUD_RES_GET_SOFTVER);
                if(pgContextData->rtinfo.waninfo.http_socketid > 0)
                {
                    Cloud_ReqGetSoftver( pgc,pgc->rtinfo.OTATypeflag);
                }
                else
                {
                    Cloud_InitSocket( pgContextData->rtinfo.waninfo.http_socketid,pgContextData->gc.GServer_ip,80,0 );
                }
                //system_os_post( 1, SIG_CLOUD_HTTP, NULL );
            break;
            default:
            break;
        }
        return 0;
    }
    return 0;
}
