#include "gagent.h"
#include "http.h"
#include "mqttxpg.h"
#include "cloud.h"
//#include "3rdcloud.h"
#include "utils.h"
#include "iof_arch.h"
#include "netevent.h"

/*
return 0 OTA SUCCESS
*/
int32 ICACHE_FLASH_ATTR
GAgent_MCUOTAByUrl( pgcontext pgc,int8 *downloadUrl )
{
    int32 ret = 0;
    int32 http_socketid = -1;
    uint8 OTA_IP[32]={0};
    int8 *url = NULL;
    int8 *host = NULL;
    if( RET_FAILED == Http_GetHost( downloadUrl,&host,&url ) )
    {
        return RET_FAILED;
    }

    GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_MCU_OTA );
    if(pgc->rtinfo.waninfo.http_socketid > 0)
    {
        ret = Http_ReqGetFirmware( url,host,http_socketid );
        if(RET_SUCCESS == ret)
        {
            GAgent_Printf(GAGENT_DEBUG,"MCU Http_ReqGetFirmware success!\n");
        }
        else
        {
            GAgent_Printf(GAGENT_DEBUG,"MCU Http_ReqGetFirmware failed!\n");
        }
    }
    else
    {
        //Cloud_InitSocket( pgc->rtinfo.waninfo.http_socketid,pgc->gc.GServer_ip,80,0 );
        //Http_ReqGetFirmware( url,host,http_socketid );
        GAgent_Printf(GAGENT_WARNING,"Http socket closed!");
        ret = RET_FAILED;
    }

    return ret;
}
uint32 ICACHE_FLASH_ATTR
GAgent_ReqServerTime(pgcontext pgc)
{
    uint32 ret;
    if((pgc->rtinfo.GAgentStatus&WIFI_STATION_CONNECTED) !=  WIFI_STATION_CONNECTED)
    {
        return (uint32)RET_FAILED;
    }

    ret = Cloud_ReqProvision( pgc );
    pgc->rtinfo.waninfo.send2HttpLastTime = GAgent_GetDevTime_S();
    if(0 != ret)
    {
        GAgent_Printf(GAGENT_WARNING,"Provision fail!\n");
        return (uint32)RET_FAILED;
    }
    return (uint32)RET_SUCCESS;
}
uint32 ICACHE_FLASH_ATTR
GAgent_Get_Gserver_Time( uint32 *clock, uint8 *Http_recevieBuf, int32 respondCode )
{
    int8 *p_start = NULL;
    int8 *p_end =NULL;
    int8 stime[20]={0};
    uint32 time;

    if( 200 != respondCode )
    {
        return (uint32)RET_FAILED;
    }
    p_start = os_strstr((char *)Http_recevieBuf, "server_ts=");
    if( p_start==NULL )
        return (uint32)RET_FAILED;
    p_start = p_start+os_strlen("server_ts=");
    p_end = os_strstr( p_start,"&" );
    if( p_end == NULL )
    {
        p_end = os_strstr( p_start,"\r" );
    }
    os_memcpy(stime,p_start,( p_end-p_start));
    time = atoi(stime);
    *clock = time;
    return (uint32)RET_SUCCESS;
}
/****************************************************************
        FunctionName    :   GAgent_Cloud_SendData
        Description     :   send buf data to M2M server.
        return          :   0-ok
                            other fail.
        Add by Alex.lin     --2015-03-17
****************************************************************/
uint32 ICACHE_FLASH_ATTR
GAgent_Cloud_SendData( pgcontext pgc,ppacket pbuf,int32 buflen )
{
    int8 ret = 0;
    uint16 cmd;

    stCloudAttrs_t *client = &pgc->rtinfo.stChannelAttrs.cloudClient;
    cmd = client->cmd;
    if( isPacketTypeSet( pbuf->type,CLOUD_DATA_OUT ) == 1)
    {
        ret = MQTT_SendData( pgc, pgc->gc.DID, pbuf,buflen );
        GAgent_Printf(GAGENT_INFO,"Send date to cloud :len =%d ,ret =%d",buflen,ret );
        pbuf->type = SetPacketType( pbuf->type,CLOUD_DATA_OUT,0 );
    }
    return ret;
}
/****************************************************************
        Function    :   Cloud_ReqRegister
        description :   sent register data to cloud
        Input       :   NULL;
        return      :   0-send register data ok.
                        other fail.
        add by Alex.lin     --2015-03-02
****************************************************************/
uint32 ICACHE_FLASH_ATTR
Cloud_ReqRegister( pgcontext pgc )
{
    uint32 socket = 0;
    int8 ret = 0;

    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;
    pGlobalVar=pgc;
    pConfigData = &(pgc->gc);
//    Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
//    socket = pGlobalVar->rtinfo.waninfo.http_socketid ;

//    if( socket<=0 )
//    {
//        return (uint32)RET_FAILED;
//    }

    ret = Http_POST( socket, HTTP_SERVER,pConfigData->wifipasscode,(char *)pGlobalVar->minfo.szmac,
                        (char *)pGlobalVar->mcu.product_key );
    if( RET_SUCCESS!=ret )
    {
        return (uint32)RET_FAILED;
    }
    else
    {
        return (uint32)RET_SUCCESS;
    }
}
/*
    will get the device id.
*/
int32 ICACHE_FLASH_ATTR
Cloud_ResRegister( uint8 *cloudConfiRxbuf,int32 buflen,int8 *pDID,int32 respondCode )
{
    int32 ret=0;

    if( 201 != respondCode)
        return RET_FAILED;
    ret = Http_Response_DID( cloudConfiRxbuf,pDID );
    if( RET_SUCCESS==ret )
    {
        return RET_SUCCESS;
    }
    else
        return RET_FAILED;
}

uint32 ICACHE_FLASH_ATTR
Cloud_ReqGetSoftver( pgcontext pgc,enum OTATYPE_T type )
{
    int32 socket = 0;
    int8 *hver, *sver;
    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;

    pGlobalVar=pgc;
    pConfigData = &(pgc->gc);
//    pGlobalVar->rtinfo.waninfo.http_socketid = Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
//    socket = pGlobalVar->rtinfo.waninfo.http_socketid;
//
//    if( socket<=0 )
//        return (uint32)RET_FAILED;

    GAgent_Printf(GAGENT_DEBUG, "http socket connect OK with:%d", socket);
    switch( type )
    {
        case OTATYPE_WIFI:
                hver = WIFI_HARDVER;
                hver[8] = '\0';
                sver = WIFI_SOFTVAR;
            break;
        case OTATYPE_MCU:
                hver = (char *)(pGlobalVar->mcu.hard_ver);
                sver = (char *)pGlobalVar->mcu.soft_ver;
            break;
        default:
            GAgent_Printf( GAGENT_WARNING,"GAgent OTA type is invalid! ");
            return (uint32)RET_FAILED;
    }
    CheckFirmwareUpgrade( HTTP_SERVER,pConfigData->DID,type,pConfigData->wifipasscode,hver,sver,socket );
    pgc->rtinfo.waninfo.send2HttpLastTime = GAgent_GetDevTime_S();
    return (uint32)RET_SUCCESS;
}
/****************************************************************
*       FunctionName    :   Cloud_ResGetSoftver.
*       Description     :   get firmwarm download url and software version.
*       buf             :   data form cloud after req Softver.
*       download_url    :   new firmwarm download url
*       fwver           :   new software version.
*       respondCode     :   http respond code.
*       reutn           :   0 success other error.
*       Add by Alex.lin   --2015-03-03
****************************************************************/
int32 ICACHE_FLASH_ATTR
Cloud_ResGetSoftver( int8 *downloadurl, int8 *fwver, uint8 *cloudConfiRxbuf,int32 respondCode )
{
    int32 ret=0;

    if( 200 != respondCode )
        return RET_FAILED;
    ret = Http_GetSoftver_Url( downloadurl, fwver, cloudConfiRxbuf );
    if( RET_SUCCESS != ret )
    {
        return RET_FAILED;
    }
    return RET_SUCCESS;
}

/****************************************************************
*       FunctionName    :   Cloud_ReqProvision
*       Description     :   send provision req to host.
*       host            :   GServer host,like "api.gizwits.com"
*       return          :   0 success other error.
*       Add by Alex.lin   --2015-03-03
****************************************************************/
uint32 ICACHE_FLASH_ATTR
Cloud_ReqProvision( pgcontext pgc )
{
    int32 socket = 0;
    int8 ret = 0;
    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;

    pGlobalVar=pgc;
    pConfigData = &(pgc->gc);

//    pGlobalVar->rtinfo.waninfo.http_socketid = Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
//    socket = pGlobalVar->rtinfo.waninfo.http_socketid;
//    if( socket<=0 )
//        return (uint32)RET_FAILED;
//
    ret = Http_GET( HTTP_SERVER,pConfigData->DID,socket );
    return ret;
}
/****************************************************************
*       FunctionName    :   Cloud_ResProvision.
*       Description     :   data form server after provision.
*       szm2mhost       :   m2m server like: "m2m.gizwits.com"
*       port            :   m2m port .
*       respondCode     :   http respond code.
*       return          :   0 success other fail.
****************************************************************/
uint32 ICACHE_FLASH_ATTR
Cloud_ResProvision( int8 *szdomain,int32 *port,uint8 *cloudConfiRxbuf,int32 respondCode )
{
    int32 ret = 0;
    if( 200 != respondCode )
    {
        return (uint32)RET_FAILED;
    }
    ret = Http_getdomain_port( cloudConfiRxbuf,szdomain,port );
    return ret;
}
/****************************************************************
*       FunctionName    :   Cloud_isNeedOTA
*       sFV             :   soft version
*       return          :   -1 do not need to OTA
*                           0 need to OTA.
*       Add by Alex.lin   --2015-03-03
****************************************************************/
uint32 ICACHE_FLASH_ATTR
Cloud_isNeedOTA( pgcontext pgc, int type, int8 *sFV )
{
    int32 result=0;
    switch( type )
    {
        case OTATYPE_WIFI:
            result = strcmp( WIFI_SOFTVAR,sFV );
            if( result < 0 )
                return RET_SUCCESS;
            break;
        case OTATYPE_MCU:
            result = strcmp((char *)pgc->mcu.soft_ver,sFV );
            if( result < 0 )
                return RET_SUCCESS;
            break;
        default:
            return (uint32)RET_FAILED;
    }
    return (uint32)RET_FAILED;
}
/****************************************************************
        Function    :   Cloud_ReqConnect
        Description :   send req m2m connect packet.
        username    :   username.
        password    :   username.
        return      :   0: send req connect packet ok
                        other req connect fail.
        Add by Alex.lin     --2015-03-09
****************************************************************/
uint32 ICACHE_FLASH_ATTR
Cloud_ReqConnect( pgcontext pgc,const int8 *username,const int8 *password )
{
    int8 ret = 0;
    int32 socket = 0;
    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;
    int32 nameLen=0,passwordLen=0;

    pGlobalVar=pgc;
    pConfigData = &(pgc->gc);

    nameLen = os_strlen( username );
    passwordLen = os_strlen( password );

    if( nameLen<=0 || nameLen>22 ) /* invalid name */
    {
        GAgent_Printf( GAGENT_WARNING," can't req to connect to m2m invalid name length !");
        return 1;
    }
    if( passwordLen<=0 || passwordLen>16 )/* invalid password */
    {
        GAgent_Printf( GAGENT_WARNING," can't req to connect to m2m invalid password length !");
        return 1;
    }
    GAgent_Printf( GAGENT_INFO,"Connect to server domain:%s port:%d",pGlobalVar->minfo.m2m_SERVER,pGlobalVar->minfo.m2m_Port );


//    pGlobalVar->rtinfo.waninfo.m2m_socketid = Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.m2m_socketid ,pConfigData->m2m_ip ,
//                                                         pGlobalVar->minfo.m2m_Port,0 );
//    socket = pGlobalVar->rtinfo.waninfo.m2m_socketid;

//    if( socket<=0 )
//    {
//        GAgent_Printf(GAGENT_WARNING,"m2m socket :%d",socket);
//        return (uint32)RET_FAILED;
//    }
    GAgent_Printf(GAGENT_DEBUG,"Cloud_InitSocket OK!");

    ret = Mqtt_Login2Server( socket,(const uint8*)username,(const uint8*)password );
    return ret;
}
/****************************************************************
        Function    :   Cloud_ResConnect
        Description :   handle packet form mqtt req connect
        buf         :   data form mqtt.
        return      :   0: req connect ok
                        other req connect fail.
        Add by Alex.lin     --2015-03-09
****************************************************************/
uint32 ICACHE_FLASH_ATTR
Cloud_ResConnect( uint8* buf )
{
    if(NULL == buf)
        return (uint32)RET_FAILED;

    if( buf[3] == 0x00 )
    {
        if( (buf[0]!=0) && (buf[1] !=0) )
        {
        return RET_SUCCESS;
        }
        else
        {
            GAgent_Printf( GAGENT_ERROR,"%s %s %d",__FILE__,__FUNCTION__,__LINE__ );
            GAgent_Printf( GAGENT_ERROR,"MQTT Connect res  fail ret =%d!!",buf[3] );
            return (uint32)RET_FAILED;
        }
    }
    GAgent_Printf( GAGENT_ERROR,"res connect fail with %d ",buf[3] );
    return buf[3];
}
uint32 Cloud_ReqSubTopic( pgcontext pgc,uint16 mqttstatus )
{
  int32 ret=0;
  ret = Mqtt_DoSubTopic( pgc,mqttstatus);
  return ret;
}
/****************************************************************
        Function        :   Cloud_ResSubTopic
        Description     :   check sub topic respond.
        buf             :   data form mqtt.
        msgsubId        :   sub topic messages id
        return          :   0 sub topic ok.
                            other fail.
        Add by Alex.lin     --2015-03-09
****************************************************************/
uint32 ICACHE_FLASH_ATTR
Cloud_ResSubTopic( const uint8* buf,int8 msgsubId )
{
    uint16 recmsgId=0;
     if(NULL == buf)
        return (uint32)RET_FAILED;
    recmsgId = mqtt_parse_msg_id( buf );
    if( recmsgId!=msgsubId )
        return (uint32)RET_FAILED;
    else
        return (uint32)RET_SUCCESS;
}
uint32 Cloud_ReqDisable( pgcontext pgc )
{
    int32 ret = 0;
    int32 socket = 0;
    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;

    pGlobalVar=pgc;
    pConfigData = &(pgc->gc);

//    pGlobalVar->rtinfo.waninfo.http_socketid = Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
//    socket = pGlobalVar->rtinfo.waninfo.http_socketid;

//    if( socket<=0 )
//        return (uint32)RET_FAILED;

    ret = Http_Delete( socket,HTTP_SERVER,pConfigData->old_did,pConfigData->old_wifipasscode );
    return 0;
}
uint32 ICACHE_FLASH_ATTR
Cloud_ResDisable( int32 respondCode )
{
    if( 200 != respondCode )
        return 1;
    return 0;
}

//uint32 Cloud_JD_Post_ReqFeed_Key( pgcontext pgc )
//{
//    int32 ret = 0;
//    int32 socket = 0;
//    pgcontext pGlobalVar=NULL;
//    pgconfig pConfigData=NULL;
//
//    pGlobalVar=pgc;
//    pConfigData = &(pgc->gc);
//
//    pGlobalVar->rtinfo.waninfo.http_socketid = Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
//    socket = pGlobalVar->rtinfo.waninfo.http_socketid;
//
//    if( socket<=0 )
//        return (uint32)RET_FAILED;
//
//    ret = Http_JD_Post_Feed_Key_req( socket,pConfigData->cloud3info.jdinfo.feed_id,pConfigData->cloud3info.jdinfo.access_key,
//                                     pConfigData->DID,HTTP_SERVER );
//    pConfigData->cloud3info.jdinfo.ischanged=0;
//    GAgent_DevSaveConfigData( pConfigData );
//    return 0;
//}

//uint32 Cloud_JD_Post_ResFeed_Key( pgcontext pgc,int32 respondCode )
//{
//    int32 ret=0;
//    pgconfig pConfigData=NULL;

//    pConfigData = &(pgc->gc);
//    if( 200 != respondCode )
//     return 1;

//    if( 1 == pConfigData->cloud3info.jdinfo.ischanged )
//    {
//        GAgent_Printf(GAGENT_WARNING,"jd info is changed need to post again.");
//        pConfigData->cloud3info.jdinfo.tobeuploaded=1;
//        ret=1;
//    }
//    else
//    {
//        GAgent_Printf(GAGENT_DEBUG,"jd info post ok.");
//        pConfigData->cloud3info.jdinfo.tobeuploaded=0;
//        ret=0;
//    }
//    GAgent_DevSaveConfigData( pConfigData );
//    return ret;
//}
//uint32 cloud_querymcuota(pgcontext pgc)
//{
//    /* 在以下条件下，发送失败。发送失败后，不进行重试 */
//    /* 1. 云端未处于运行状态 */
//    /* 2. http未空闲 */
//    /* 3.  */
//    int32 ret = 0;
//    int32 socket = 0;

//    pgc->rtinfo.waninfo.http_socketid = Cloud_InitSocket( pgc->rtinfo.waninfo.http_socketid,pgc->gc.GServer_ip,80,0 );
//    socket = pgc->rtinfo.waninfo.http_socketid;

//    if( socket<=0 )
//        return (uint32)RET_FAILED;

//    /* ret = Http_JD_Post_Feed_Key_req( socket,pConfigData->cloud3info.jdinfo.feed_id,pConfigData->cloud3info.jdinfo.access_key, */
//                                     /* pConfigData->DID,HTTP_SERVER ); */
//    /* pConfigData->cloud3info.jdinfo.ischanged=0; */
//    /* GAgent_DevSaveConfigData( pConfigData ); */
//    resetPacket(pgc->rtinfo.Txbuf);
//    ret = http_querymcuota(socket, pgc->rtinfo.Txbuf->phead, HTTP_SERVER, pgc->tmcu.pk, pgc->tmcu.did, pgc->tmcu.hv, pgc->tmcu.sv, 0x02);
//    GAgent_SetCloudConfigStatus( pgc,CLOUD_QUERY_MCU_OTA );
//    pgc->rtinfo.waninfo.send2HttpLastTime = GAgent_GetDevTime_S();
//    return 0;

//}

//uint32 test_trans_mcuota_cmd(pgcontext pgc)
//{
//    trans_sendotaresult(pgc, 0x01);
//    trans_sendotaresult(pgc, 0x00);
//    trans_sendotadownloadresult(pgc, TRANSCTION_OTA_FILE_DOWNLOAD_SUCCESS);
//    trans_sendotadownloadresult(pgc, TRANSCTION_OTA_FILE_DOWNLOAD_FAILED);
//    return 0;
//}

//uint32 test_cloud_querymcuota(pgcontext pgc)
//{
//    memset(&pgc->tmcu, 0x00, sizeof(trans_mcuotainfo));
//    memcpy(pgc->tmcu.pk, "b4953ca374724b66b3686fbfb9d7c57a", strlen("6f3074fe43894547a4f1314bd7e3ae0b"));
//    memcpy(pgc->tmcu.hv, "00000001", 8);
//    memcpy(pgc->tmcu.sv, "01000001", 8);
//    pgc->tmcu.check = 0x01;
//    /* test_trans_mcuota_cmd(pgc); */
//    cloud_querymcuota(pgc);
//}

void ICACHE_FLASH_ATTR
Cloud_SetClientAttrs(pgcontext pgc, uint8 *clientid, uint16 cmd, int32 sn)
{
    if(NULL != clientid)
    {
        os_strcpy(pgc->rtinfo.waninfo.srcAttrs.phoneClientId, (int8 *)clientid);
    }
    pgc->rtinfo.waninfo.srcAttrs.cmd = cmd;
    pgc->rtinfo.waninfo.srcAttrs.sn = sn;
}

void ICACHE_FLASH_ATTR
Cloud_ClearClientAttrs(pgcontext pgc, stCloudAttrs_t *client)
{
    os_memset((char *)client, 0, sizeof(stCloudAttrs_t));
}

/****************************************************************
*       functionname    :   Cloud_ReadGServerConfigData
*       description     :   read data form gserver.
*       socket          :   gserver socket.
*       buf             :   data pointer form gserver
*       buflen          :   want to read data length
        return          :   >0 data form gserver
                            other error.
        Add by Alex.lin     --2015-03-03
****************************************************************/
//int32 Cloud_ReadGServerConfigData( pgcontext pgc ,int32 socket,uint8 *buf,int32 buflen )
//{
//    int32 ret =0;
//    ret = Http_ReadSocket( socket,buf,buflen );
//    if( ret <0 )
//    {
//        GAgent_Printf( GAGENT_WARNING,"Cloud_ReadGServerConfigData fail close the socket:%d",socket );
//        close( socket );
//        socket = INVALID_SOCKET;
//        GAgent_SetGServerSocket( pgc,socket );
//        return RET_FAILED;
//    }
//    return ret;
//}


/****************************************************************
        FunctionName        :   GAgent_CloudTick.
        Description         :   GAgent Send cloud heartbeat to cloud
                                when mqttstatus is MQTT_STATUS_RUNNING

        Add by Alex.lin     --2015-03-10
****************************************************************/
void ICACHE_FLASH_ATTR
GAgent_CloudTick( pgcontext pgc,uint32 dTime_s )
{
    static uint32 count = 0;
    int ret = 0;
    uint16 newStatus=0;
    newStatus = pgc->rtinfo.GAgentStatus;

    GAGENT_CheckOtaStatus(pgc);

    if( 0 != pgc->rtinfo.clock )
    {
        pgc->rtinfo.clock++;
        count++;
    }
    if( count >= ONE_HOUR)
    {
        if(CLOUD_CONFIG_OK != pgc->rtinfo.waninfo.CloudStatus)
        {
            count = ONE_HOUR-1;
        }
        else
        {
            count = 0;
            GAgent_SetCloudConfigStatus( pgc,CLOUD_REQ_GET_GSERVER_TIME );
            GAgent_Printf(GAGENT_INFO,"Request sync server time!\n");
            if(pgContextData->rtinfo.waninfo.http_socketid > 0)
            {
                ret = GAgent_ReqServerTime(pgc);
                if( RET_FAILED == ret)
                {
                    GAgent_Printf(GAGENT_WARNING,"Request sync time fial!\n");
                }
            }
            else
            {
                Cloud_InitSocket( pgc->rtinfo.waninfo.http_socketid,pgc->gc.GServer_ip,80,0 );
            }
        }
    }

    if( WIFI_CLOUD_CONNECTED!=(newStatus&WIFI_CLOUD_CONNECTED) )
        return ;

    pgc->rtinfo.waninfo.send2MqttLastTime +=dTime_s;
    if( pgc->rtinfo.waninfo.send2MqttLastTime >= CLOUD_HEARTBEAT &&
        pgc->rtinfo.file.using == 0 )
    {
        pgc->rtinfo.waninfo.send2MqttLastTime  = 0;
        if( pgc->rtinfo.waninfo.cloudPingTime > 2 )
        {

            ERRORCODE
            pgc->rtinfo.waninfo.cloudPingTime=0;
            pgc->rtinfo.waninfo.wanclient_num = 0;
            pgc->rtinfo.waninfo.ReConnectMqttTime = 0;

            //newStatus &=~ WIFI_CLOUD_CONNECTED;
            GAgent_SetCloudServerStatus( pgc,MQTT_STATUS_START );
            GAgent_Printf( GAGENT_INFO,"file:%s function:%s line:%d ",__FILE__,__FUNCTION__,__LINE__ );
            newStatus = GAgent_DevCheckWifiStatus( WIFI_CLOUD_CONNECTED,0 );
        }
        else
        {
            MQTT_HeartbeatTime();
            pgc->rtinfo.waninfo.cloudPingTime++;
            GAgent_Printf( GAGENT_CRITICAL,"GAgent Cloud Ping ..." );
        }
    }
}
/****************************************************************
*
*   function    :   gagent do cloud config.
*   cloudstatus :   gagent cloud status.
*   return      :   0 successful other fail.
*   Add by Alex.lin --2015-02-28
****************************************************************/
uint32 ICACHE_FLASH_ATTR
Cloud_ConfigDataHandle( pgcontext pgc /*int32 cloudstatus*/ )
{
    uint32 dTime=0;
    int32 ret =0;
    int32 i;
    int32 respondCode=0;
    int32 cloudstatus = 0;
    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;
    static int8 wifiOTARunning = 0;

    uint16 GAgentStatus = 0;
    int8 *pDeviceID=NULL;
    int8 timeoutflag = 0;
    int32 http_fd;

    uint8 *pCloudConfiRxbuf = NULL;
    //resetPacket(pgc->rtinfo.Rxbuf);
    pCloudConfiRxbuf = pgc->rtinfo.HttpRxbuf->phead;
    pConfigData = &(pgc->gc);
    pGlobalVar = pgc;
    cloudstatus = pgc->rtinfo.waninfo.CloudStatus;
    GAgentStatus = pgc->rtinfo.GAgentStatus;

    if( 1 == wifiOTARunning )
    {
        GAgent_Printf(GAGENT_DEBUG,"WIFI OTA is running!\n");
        return 1 ;
    }

    if((GAgentStatus&WIFI_STATION_CONNECTED) !=  WIFI_STATION_CONNECTED)
    {
        return 1 ;
    }

    if( CLOUD_RES_MCU_OTA == cloudstatus )
    {
        GAgent_Printf(GAGENT_DEBUG,"MCU OTA is running!\n");
        return 1 ;
    }

    if(CLOUD_CONFIG_OK == cloudstatus)
    {
        if(pGlobalVar->rtinfo.waninfo.http_socketid > 0)
        {
            espconn_disconnect(pgc->rtinfo.waninfo.phttp_fd);
            GAgent_Printf( GAGENT_CRITICAL,"http config ok ,and close the socket.");
            pGlobalVar->rtinfo.waninfo.http_socketid = INVALID_SOCKET;
        }
        pgc->rtinfo.waninfo.httpCloudPingTime = 0;
        pgc->rtinfo.waninfo.ReConnectHttpTime = GAGENT_HTTP_TIMEOUT;
        return 1;
    }

    pDeviceID = pConfigData->DID;
    http_fd = pGlobalVar->rtinfo.waninfo.http_socketid;

    if(CLOUD_INIT == cloudstatus)
    {
        if(os_strlen(pDeviceID) == (DID_LEN - 2))/*had did*/
        {
            GAgent_Printf(GAGENT_INFO,"Had did,did = %s!!!!go to Provision",pDeviceID );
            GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_PROVISION );
            Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
            //ret = Cloud_ReqProvision( pgc );
        }
        else
        {
            GAgent_Printf( GAGENT_INFO,"Need to get did!!!" );
            GAgent_SetDeviceID( pgc,NULL );/*clean did*/
            GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_GET_DID );
            Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
            //ret = Cloud_ReqRegister( pgc );
        }
        pGlobalVar->rtinfo.waninfo.send2HttpLastTime = GAgent_GetDevTime_S();
        return 0;
    }

    dTime = abs(GAgent_GetDevTime_S()- pGlobalVar->rtinfo.waninfo.send2HttpLastTime);
//    if( 1 == pgc->rtinfo.onlinePushflag )
//    {
//        dTime = pgc->rtinfo.waninfo.ReConnectHttpTime + 1;
//    }
    if( 1 == pgc->rtinfo.waninfo.httpRecvFlag ||
       ( (cloudstatus != CLOUD_CONFIG_OK) && (dTime > pgc->rtinfo.waninfo.ReConnectHttpTime))
      )
    {
        pgContextData->rtinfo.waninfo.httpRecvFlag = 0;
        GAgent_Printf(GAGENT_DEBUG,"HTTP Data from Gserver!%d", 2);
        if(dTime > pgc->rtinfo.waninfo.ReConnectHttpTime)
        {
            GAgent_Printf( GAGENT_INFO,"HTTP timeout...,dTime=%d",dTime);
            if(pGlobalVar->rtinfo.waninfo.http_socketid > 0)
            {
                //close(pGlobalVar->rtinfo.waninfo.http_socketid);
                //pGlobalVar->rtinfo.waninfo.http_socketid = INVALID_SOCKET;
            }
            respondCode = -1;
        }
        else
        {
            respondCode = Http_Response_Code( pCloudConfiRxbuf );
        }

        GAgent_Printf(GAGENT_INFO,"http read ret:%d cloudStatus : %d,response code: %d",ret,cloudstatus,respondCode );

        switch( cloudstatus )
        {
            case CLOUD_RES_GET_DID:
                ret = Cloud_ResRegister( pCloudConfiRxbuf,ret,pDeviceID,respondCode );
                if(RET_SUCCESS != ret)/* can't got the did */
                {
                    if(dTime > pgc->rtinfo.waninfo.ReConnectHttpTime)
                    {
                        timeoutflag = 1;
                        GAgent_Printf(GAGENT_ERROR,"res register fail: %s %d",__FUNCTION__,__LINE__ );
                        GAgent_Printf(GAGENT_ERROR,"go to req register Device id again.");
                        if(pgContextData->rtinfo.waninfo.http_socketid > 0)
                        {
                            Cloud_ReqRegister( pgc );
                        }
                        else
                        {
                            Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
                        }
                    }
                }
                else
                {
                    pgc->rtinfo.waninfo.ReConnectHttpTime = GAGENT_HTTP_TIMEOUT;
                    pgc->rtinfo.waninfo.httpCloudPingTime = 0;
                    pgc->rtinfo.waninfo.firstConnectHttpTime = GAgent_GetDevTime_S();
                    GAgent_SetDeviceID( pgc,pDeviceID );
                    GAgent_DevGetConfigData( &(pgc->gc) );
                    GAgent_Printf( GAGENT_INFO,"Register got did :%s len=%d",pgc->gc.DID,os_strlen(pgc->gc.DID) );
                    GAgent_Printf( GAGENT_INFO,"GAgent go to Provision!!!");
                    GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_PROVISION );
                    if(pgContextData->rtinfo.waninfo.http_socketid > 0)
                    {
                        Cloud_ReqProvision( pgc );
                    }
                    else
                    {
                        Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
                    }
                }
            break;
            case CLOUD_RES_PROVISION:
//                 pGlobalVar->rtinfo.waninfo.Cloud3Flag = Http_Get3rdCloudInfo( pConfigData->cloud3info.cloud3Name,pConfigData->cloud3info.jdinfo.product_uuid ,
//                                                                pCloudConfiRxbuf );
//                 /* have 3rd cloud info need save to falsh */
//                 if( pGlobalVar->rtinfo.waninfo.Cloud3Flag == 1 )
//                 {
//                    GAgent_Printf(GAGENT_INFO,"3rd cloud name:%s",pConfigData->cloud3info.cloud3Name );
//                    GAgent_Printf(GAGENT_INFO,"3re cloud UUID: %s",pConfigData->cloud3info.jdinfo.product_uuid);
//                    GAgent_DevSaveConfigData( pConfigData );
//                 }
                ret = Cloud_ResProvision( pGlobalVar->minfo.m2m_SERVER , &pGlobalVar->minfo.m2m_Port,pCloudConfiRxbuf,respondCode);
                GAgent_Get_Gserver_Time( &pGlobalVar->rtinfo.clock, pCloudConfiRxbuf,respondCode );
                if( ret!=0 )
                {
                    if(dTime > pgc->rtinfo.waninfo.ReConnectHttpTime)
                    {
                        timeoutflag = 1;
                        GAgent_Printf(GAGENT_WARNING,"Provision res fail ret=%d.", ret );
                        GAgent_Printf(GAGENT_WARNING,"go to provision again.");
                        if(pgContextData->rtinfo.waninfo.http_socketid > 0)
                        {
                            Cloud_ReqProvision( pgc );
                        }
                        else
                        {
                            Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
                        }
                    }
                }
                else
                {
                    pgc->rtinfo.waninfo.ReConnectHttpTime = GAGENT_HTTP_TIMEOUT;
                    pgc->rtinfo.waninfo.httpCloudPingTime = 0;
                    pgc->rtinfo.waninfo.firstConnectHttpTime = GAgent_GetDevTime_S();
                    //login to m2m.
                    //after m2m_dns
                    GAgent_SetCloudServerStatus( pgc,MQTT_STATUS_START );
                    pgc->rtinfo.waninfo.ReConnectMqttTime = 0;
                    pgc->rtinfo.waninfo.send2MqttLastTime = GAgent_GetDevTime_S();

                    GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_GET_SOFTVER );
                    pgc->rtinfo.isOtaRunning = 1;
                    if(pgContextData->rtinfo.waninfo.http_socketid > 0)
                    {
                        Cloud_ReqGetSoftver( pgc,pgc->rtinfo.OTATypeflag);
                    }
                    else
                    {
                        Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
                    }

                    GAgent_Printf(GAGENT_INFO,"Provision OK!");
                    GAgent_Printf(GAGENT_INFO,"M2M host:%s port:%d",pGlobalVar->minfo.m2m_SERVER,pGlobalVar->minfo.m2m_Port);
                    GAgent_Printf(GAGENT_INFO,"GAgent go to update OTA info and login M2M !");
                }
            break;
            case CLOUD_RES_GET_SOFTVER:
            {
                int8 *download_url = NULL;
                int8  disableDIDflag=0;

                download_url = (int8 *)os_malloc(256);
                if(NULL == download_url)
                {
                    GAgent_Printf(GAGENT_WARNING, "OTA malloc fail!go to get OTA info again...");
                    break;
                }
                ret = Cloud_ResGetSoftver( download_url ,pGlobalVar->gc.FirmwareVer ,pCloudConfiRxbuf,respondCode );
                if( RET_SUCCESS != ret )
                {
                    if(dTime > pgc->rtinfo.waninfo.ReConnectHttpTime)
                    {
                        timeoutflag = 1;
                        GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_GET_SOFTVER );
                        if(pgContextData->rtinfo.waninfo.http_socketid > 0)
                        {
                            Cloud_ReqGetSoftver( pgc,pgc->rtinfo.OTATypeflag);
                        }
                        else
                        {
                            Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
                        }
                    }
                    else
                    {
                        if( OTATYPE_MCU == pgc->rtinfo.OTATypeflag )
                        {
                            GAgent_Printf( GAGENT_WARNING,"GAgent get MCU OTA respondCode:%d",respondCode );
                            if(0 == pgc->rtinfo.onlinePushflag)
                            {
                                GAgent_Printf( GAGENT_WARNING,"go to check disaable Device!" );
                                disableDIDflag = 1;
                                pgc->rtinfo.isOtaRunning = 0;
                                system_os_post( 1, SIG_CLOUD_M2M, NULL);
                            }
                            else
                            {
                                GAgent_SetCloudConfigStatus( pgc,CLOUD_CONFIG_OK);
                            }
                        }
                        else
                        {
                            GAgent_Printf( GAGENT_WARNING,"GAgent get WIFI OTA respondCode:%d,go to check MCU OTA!",respondCode );
                            if(0 == pgc->rtinfo.onlinePushflag)
                            {
                                GAgent_Printf( GAGENT_WARNING,"go to check MCU OTA!" );
                                pgc->rtinfo.OTATypeflag = OTATYPE_MCU;
                                GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_GET_SOFTVER );
                                if(pgContextData->rtinfo.waninfo.http_socketid > 0)
                                {
                                   Cloud_ReqGetSoftver( pgc,pgc->rtinfo.OTATypeflag);
                                }
                                else
                                {
                                    Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
                                }
                            }
                            else
                            {
                                GAgent_SetCloudConfigStatus( pgc,CLOUD_CONFIG_OK);
                            }
                        }
                    }
                }
                else
                {
                    pgc->rtinfo.waninfo.ReConnectHttpTime = GAGENT_HTTP_TIMEOUT;
                    pgc->rtinfo.waninfo.httpCloudPingTime = 0;
                    pgc->rtinfo.waninfo.firstConnectHttpTime = GAgent_GetDevTime_S();
                    ret = Cloud_isNeedOTA( pgc, pgc->rtinfo.OTATypeflag, pGlobalVar->gc.FirmwareVer );
                    if( ret != RET_SUCCESS )
                    {
                        if( OTATYPE_MCU == pgc->rtinfo.OTATypeflag )
                        {
                            GAgent_Printf(GAGENT_WARNING,"MCU does not need OTA,current MCU softver = %s",pgc->mcu.soft_ver);
                            if(0 == pgc->rtinfo.onlinePushflag)
                            {
                                GAgent_Printf( GAGENT_WARNING,"go to check disaable Device!" );
                                disableDIDflag=1;
                            }
                            else
                            {
                                GAgent_SetCloudConfigStatus( pgc,CLOUD_CONFIG_OK);
                            }
                        }
                        else
                        {
                            GAgent_Printf( GAGENT_WARNING,"WIFI does not need OTA,current WIFI softver = %s",WIFI_SOFTVAR );
                            if(0 == pgc->rtinfo.onlinePushflag)
                            {
                                GAgent_Printf( GAGENT_WARNING,"go to check MCU OTA!" );
                                pgc->rtinfo.OTATypeflag = OTATYPE_MCU;
                                GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_GET_SOFTVER );
                                if(pgc->rtinfo.waninfo.http_socketid > 0)
                                {
                                    Cloud_ReqGetSoftver( pgc,pgc->rtinfo.OTATypeflag);
                                }
                                else
                                {
                                    Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
                                }
                            }
                            else
                            {
                                GAgent_SetCloudConfigStatus( pgc,CLOUD_CONFIG_OK);
                            }
                        }
                    }
                    else
                    {
                        if( RET_SUCCESS == GAgent_Cloud_OTAByUrl( pgc, download_url, pgc->rtinfo.OTATypeflag ) )
                        {
                            if( OTATYPE_MCU == pgc->rtinfo.OTATypeflag )
                            {
                                GAgent_Printf( GAGENT_CRITICAL,"MCU ota is running\n");
                                return 0;
                            }
                            else
                            {
                                GAgent_Printf( GAGENT_CRITICAL,"WIFI OTA is running!\n");
                                wifiOTARunning = 1;
                                return 0;
                                pgc->rtinfo.OTATypeflag = OTATYPE_MCU;
                                GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_GET_SOFTVER );
                                if(pgContextData->rtinfo.waninfo.http_socketid > 0)
                                {
                                    Cloud_ReqGetSoftver( pgc,pgc->rtinfo.OTATypeflag);
                                }
                                else
                                {
                                    Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
                                }
                            }
                        }
                        else
                        {
                            if( OTATYPE_MCU == pgc->rtinfo.OTATypeflag )
                            {
                                GAgent_Printf(GAGENT_WARNING,"GAgent download MCU firmware failed ,go to check disaable Device!");
                                disableDIDflag=1;
                                pgc->rtinfo.isOtaRunning = 0;
                                system_os_post( 1, SIG_CLOUD_M2M, NULL);
                            }
                            else
                            {
                                GAgent_Printf(GAGENT_WARNING,"GAgent download WIFI firmware failed!");
                                if(0 == pgc->rtinfo.onlinePushflag)
                                {
                                    GAgent_Printf(GAGENT_WARNING,"go to check MCU OTA!");
                                    pgc->rtinfo.OTATypeflag = OTATYPE_MCU;
                                    GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_GET_SOFTVER );
                                    if(pgc->rtinfo.waninfo.http_socketid > 0)
                                    {
                                        Cloud_ReqGetSoftver( pgc,pgc->rtinfo.OTATypeflag);
                                    }
                                    else
                                    {
                                        Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
                                    }
                                }
                                else
                                {
                                    GAgent_SetCloudConfigStatus( pgc,CLOUD_CONFIG_OK);
                                }
                            }
                        }
                    }
                }

                if( (1==disableDIDflag) )
                {
                    if( 1==GAgent_IsNeedDisableDID( pgc ) )
                    {
                        GAgent_Printf(GAGENT_INFO,"Need to Disable Device ID!");
                        GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_DISABLE_DID );
                        if(pgContextData->rtinfo.waninfo.http_socketid > 0)
                        {
                            GAgent_Printf(GAGENT_WARNING,"Req to Disable Device ID!");
                            Cloud_ReqDisable( pgc );
                        }
                        else
                        {
                            Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
                        }
                    }
                    else
                    {
                       GAgent_SetCloudConfigStatus ( pgc,CLOUD_CONFIG_OK );
                    }
                    pgc->rtinfo.OTATypeflag = OTATYPE_WIFI;
                }
                os_free(download_url);
            break;
            }
            case CLOUD_RES_DISABLE_DID:
                ret = Cloud_ResDisable( respondCode );
                if(ret!=0)
                {
                    if(dTime > pgc->rtinfo.waninfo.ReConnectHttpTime)
                    {
                        timeoutflag = 1;
                        GAgent_Printf(GAGENT_WARNING,"Disable Device ID Fail.");
                        if( 1==GAgent_IsNeedDisableDID( pgc ) )
                        {
                            GAgent_Printf(GAGENT_INFO,"Need to Disable Device ID again !");
                            ret = Cloud_ReqDisable( pgc );
                            GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_DISABLE_DID );
                        }
                        else
                        {
                           if( 1==pConfigData->cloud3info.jdinfo.tobeuploaded )
                           {
                                GAgent_Printf( GAGENT_INFO,"%d Neet to be uploaded jdinfo.",__LINE__);
                                GAgent_SetCloudConfigStatus ( pgc,CLOUD_RES_POST_JD_INFO );
                           }
                        }
                    }
                    else
                    {
                        if( 1==pConfigData->cloud3info.jdinfo.tobeuploaded )
                        {
                            GAgent_Printf( GAGENT_INFO,"%d Neet to be uploaded jdinfo.",__LINE__);
                            GAgent_SetCloudConfigStatus ( pgc,CLOUD_RES_POST_JD_INFO );
                        }
                        else
                        {
                           GAgent_SetCloudConfigStatus ( pgc,CLOUD_CONFIG_OK );
                        }
                    }
                }
                else
                {
                    pgc->rtinfo.waninfo.ReConnectHttpTime = GAGENT_HTTP_TIMEOUT;
                    pgc->rtinfo.waninfo.httpCloudPingTime = 0;
                    pgc->rtinfo.waninfo.firstConnectHttpTime = GAgent_GetDevTime_S();
                    GAgent_Printf(GAGENT_INFO,"Disable Device ID OK!");
                    GAgent_SetOldDeviceID( pgc,NULL,NULL,0 );

                    if( 1==pConfigData->cloud3info.jdinfo.tobeuploaded )
                    {
                        GAgent_Printf( GAGENT_INFO,"%d Neet to be uploaded jdinfo.",__LINE__);
                        GAgent_SetCloudConfigStatus ( pgc,CLOUD_RES_POST_JD_INFO );
                    }
                    else
                    {
                        GAgent_SetCloudConfigStatus ( pgc,CLOUD_CONFIG_OK );
                    }
                }
            break;
            case CLOUD_RES_POST_JD_INFO:
//                 ret = Cloud_JD_Post_ResFeed_Key( pgc,respondCode );
//                 if( ret!=0 )
//                 {
//                     GAgent_Printf( GAGENT_WARNING," Post JD info respond fail!" );
//                     //if( dTime > pgc->rtinfo.waninfo.ReConnectHttpTime )
//                     {
//                        timeoutflag = 1;
//                        GAgent_Printf( GAGENT_WARNING," Post JD info again");
//                        ret = Cloud_JD_Post_ReqFeed_Key( pgc );
//                     }
//                 }
//                 else
//                 {
//                    pgc->rtinfo.waninfo.ReConnectHttpTime = GAGENT_HTTP_TIMEOUT;
//                    pgc->rtinfo.waninfo.httpCloudPingTime = 0;
//                    pgc->rtinfo.waninfo.firstConnectHttpTime = GAgent_GetDevTime_S();
//                    GAgent_SetCloudConfigStatus( pgc,CLOUD_CONFIG_OK );
//                 }
            break;
            case CLOUD_REQ_GET_GSERVER_TIME:
                ret = GAgent_Get_Gserver_Time( &pGlobalVar->rtinfo.clock, pCloudConfiRxbuf,respondCode );
                if(ret == RET_SUCCESS)
                {
                    GAgent_Printf(GAGENT_INFO,"sync server time success!");
                }
                GAgent_SetCloudConfigStatus( pgc,CLOUD_CONFIG_OK );
            break;
            default:
                break;
        }

        if(timeoutflag)
        {
            pgc->rtinfo.waninfo.ReConnectHttpTime += (10 * ONE_SECOND);
            pgc->rtinfo.waninfo.httpCloudPingTime++;
            if(pgc->rtinfo.waninfo.httpCloudPingTime == 10)
            {
                GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_PROVISION );
                if(pgContextData->rtinfo.waninfo.http_socketid > 0)
                {
                    Cloud_ReqProvision( pgc );
                }
                else
                {
                    Cloud_InitSocket( pGlobalVar->rtinfo.waninfo.http_socketid,pConfigData->GServer_ip,80,0 );
                }
                pgc->rtinfo.waninfo.httpCloudPingTime = 0;
            }

            if((GAgent_GetDevTime_S()-pgc->rtinfo.waninfo.firstConnectHttpTime) >= 2 * ONE_HOUR)
            {
                GAgent_DevReset();
            }
        }
        pGlobalVar->rtinfo.waninfo.send2HttpLastTime = GAgent_GetDevTime_S();
    }
    return 0;
}
/****************************************************************
        FunctionName        :   Cloud_M2MDataHandle.
        Description         :   Receive cloud business data .
        xpg                 :   global context.
        Rxbuf                :   global buf struct.
        buflen              :   receive max len.
        return              :   >0 have business data,and need to
                                   handle.
                                other,no business data.
        Add by Alex.lin     --2015-03-10
****************************************************************/
int32 ICACHE_FLASH_ATTR
Cloud_M2MDataHandle(  pgcontext pgc,ppacket pbuf /*, ppacket poutBuf*/, uint32 buflen)
{
    uint32 dTime=0,ret=0,dataLen=0;
    int32 packetLen=0;
    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;
    int8 *username=NULL;
    int8 *password=NULL;
    uint8* pMqttBuf=NULL;

    int32 mqtt_fd=0;
    uint16 mqttstatus=0;
    uint8 mqttpackType=0;

    pConfigData = &(pgc->gc);
    pGlobalVar = pgc;

    mqttstatus = pGlobalVar->rtinfo.waninfo.mqttstatus;
    mqtt_fd = pGlobalVar->rtinfo.waninfo.m2m_socketid;
    username = pConfigData->DID;
    password = pConfigData->wifipasscode;

    if( os_strlen(pConfigData->m2m_ip)==0 )
    {
        GAgent_Printf( GAGENT_INFO,"M2M IP =0 IP TIME 1 %d 2%d ",pgc->rtinfo.waninfo.RefreshIPLastTime,pgc->rtinfo.waninfo.RefreshIPTime);
        return 0;
    }

    dTime = abs( GAgent_GetDevTime_S()-pgc->rtinfo.waninfo.send2MqttLastTime );
    if( MQTT_STATUS_START==mqttstatus )
    {
        if( dTime >= pgc->rtinfo.waninfo.ReConnectMqttTime )
        {
            GAgent_Printf(GAGENT_INFO,"Req to connect m2m !");
            GAgent_Printf(GAGENT_INFO,"username: %s password: %s",username,password);
            if(pgc->rtinfo.waninfo.m2m_socketid > 0)
            {
                Cloud_ReqConnect( pgc,username,password );
            }
            else
            {
                Cloud_M2M_InitSocket( pGlobalVar->rtinfo.waninfo.m2m_socketid ,pConfigData->m2m_ip ,pGlobalVar->minfo.m2m_Port,0 );
            }
            GAgent_SetCloudServerStatus( pgc,MQTT_STATUS_RES_LOGIN );
            GAgent_Printf(GAGENT_INFO,"MQTT_STATUS_START");
            pgc->rtinfo.waninfo.send2MqttLastTime = GAgent_GetDevTime_S();
            if( pgc->rtinfo.waninfo.ReConnectMqttTime < GAGENT_MQTT_TIMEOUT)
            {
                pgc->rtinfo.waninfo.ReConnectMqttTime = GAGENT_MQTT_TIMEOUT;
            }
            else
            {
                pgc->rtinfo.waninfo.ReConnectMqttTime += GAGENT_CLOUDREADD_TIME;
                if(pgc->rtinfo.waninfo.ReConnectMqttTime > GAGENT_MQTT_DEADTIME_IN_TIMEOUT)
                {
                    /* can't connect to cloud over 2 hours, reset!!! */
                    GAgent_Printf(GAGENT_CRITICAL, "disconnect to m2m server over 2 h.reset!!!\r\n");
                    msleep();
                    GAgent_DevReset();
                }
            }
        }
        return 0;
    }
    if( (1 == pgContextData->rtinfo.waninfo.m2mRecvFlag) ||
        ( mqttstatus!=MQTT_STATUS_RUNNING && dTime>(pgc->rtinfo.waninfo.ReConnectMqttTime) )
       )
    {
        pMqttBuf = pbuf->phead;
//        if( mqtt_fd>0 ? FD_ISSET( mqtt_fd,&readfd):0 )
//        {
//          GAgent_Printf(GAGENT_DEBUG,"Data form M2M!!!");
//          resetPacket( pbuf );
//          pMqttBuf = pbuf->phead;
//          packetLen = MQTT_readPacket(mqtt_fd,pbuf,GAGENT_BUF_LEN );
//          if( packetLen==-1 )
//          {
//              mqtt_fd=-1;
//              pGlobalVar->rtinfo.waninfo.m2m_socketid=-1;
//              //GAgent_SetCloudServerStatus( pgc,MQTT_STATUS_START );
//              //GAgent_SetWiFiStatus( pgc,WIFI_CLOUD_CONNECTED,0 );
//              GAgent_DevCheckWifiStatus( WIFI_CLOUD_CONNECTED,0 );
//              GAgent_Printf(GAGENT_DEBUG,"MQTT fd was closed!!");
//              GAgent_Printf(GAGENT_DEBUG,"GAgent go to MQTT_STATUS_START");
//              return RET_FAILED;
//          }
//          else if( packetLen>0 )
        if( 1 == pgContextData->rtinfo.waninfo.m2mRecvFlag )
        {
            GAgent_Printf(GAGENT_DEBUG,"Data form M2M!!!");
            pgc->rtinfo.waninfo.ReConnectMqttTime = GAGENT_MQTT_TIMEOUT;
            mqttpackType = MQTTParseMessageType( pMqttBuf );
            GAgent_Printf( GAGENT_DEBUG,"MQTT message type %d",mqttpackType );
        }
        else
        {
            return RET_FAILED;
        }

        /*create mqtt connect to m2m.*/
        if( MQTT_STATUS_RUNNING!=mqttstatus &&
            (MQTT_MSG_CONNACK==mqttpackType||MQTT_MSG_SUBACK==mqttpackType||dTime>(pgc->rtinfo.waninfo.ReConnectMqttTime) ) )
        {
            int8 timeoutFlag=0;
            switch( mqttstatus)
            {
                case MQTT_STATUS_RES_LOGIN:
                     ret = Cloud_ResConnect( pMqttBuf );
                     if( RET_SUCCESS!= ret )
                     {
                         GAgent_Printf(GAGENT_DEBUG," MQTT_STATUS_REQ_LOGIN Fail ");
                         if( dTime > (pgc->rtinfo.waninfo.ReConnectMqttTime) )
                         {
                            timeoutFlag =1;
                            GAgent_Printf(GAGENT_DEBUG,"MQTT req connetc m2m again!");
                            //Cloud_ReqConnect( pgc,username,password );
                            Cloud_M2M_InitSocket( pGlobalVar->rtinfo.waninfo.m2m_socketid ,pConfigData->m2m_ip ,pGlobalVar->minfo.m2m_Port,0 );
                         }
                     }
                     else
                     {
                         GAgent_Printf(GAGENT_DEBUG,"GAgent do req connect m2m OK !");
                         GAgent_Printf(GAGENT_DEBUG,"Go to MQTT_STATUS_REQ_LOGINTOPIC1. ");
                         Cloud_ReqSubTopic( pgc,MQTT_STATUS_REQ_LOGINTOPIC1 );
                         GAgent_SetCloudServerStatus( pgc,MQTT_STATUS_RES_LOGINTOPIC1 );
                     }
                     break;
                case MQTT_STATUS_RES_LOGINTOPIC1:
                     ret = Cloud_ResSubTopic(pMqttBuf,pgc->rtinfo.waninfo.mqttMsgsubid );
                     if( RET_SUCCESS!=ret )
                     {
                        GAgent_Printf(GAGENT_DEBUG," MQTT_STATUS_RES_LOGINTOPIC1 Fail ");
                        if( dTime > (pgc->rtinfo.waninfo.ReConnectMqttTime) )
                        {
                            timeoutFlag =1;
                            GAgent_Printf( GAGENT_DEBUG,"GAgent req sub LOGINTOPIC1 again ");
                            Cloud_ReqSubTopic( pgc,MQTT_STATUS_REQ_LOGINTOPIC1 );
                        }
                     }
                     else
                     {
                        GAgent_Printf(GAGENT_DEBUG,"Go to MQTT_STATUS_RES_LOGINTOPIC2. ");
                        Cloud_ReqSubTopic( pgc,MQTT_STATUS_REQ_LOGINTOPIC2 );
                        GAgent_SetCloudServerStatus( pgc,MQTT_STATUS_RES_LOGINTOPIC2 );
                     }
                     break;
                case MQTT_STATUS_RES_LOGINTOPIC2:
                     ret = Cloud_ResSubTopic(pMqttBuf,pgc->rtinfo.waninfo.mqttMsgsubid );
                     if( RET_SUCCESS != ret )
                     {
                        GAgent_Printf(GAGENT_DEBUG," MQTT_STATUS_RES_LOGINTOPIC2 Fail ");
                        if( dTime > (pgc->rtinfo.waninfo.ReConnectMqttTime) )
                        {
                            timeoutFlag =1;
                            GAgent_Printf( GAGENT_INFO,"GAgent req sub LOGINTOPIC2 again.");
                            Cloud_ReqSubTopic( pgc,MQTT_STATUS_REQ_LOGINTOPIC2 );
                        }
                     }
                     else
                     {
                        GAgent_Printf(GAGENT_DEBUG," Go to MQTT_STATUS_RES_LOGINTOPIC3. ");
                        Cloud_ReqSubTopic( pgc,MQTT_STATUS_REQ_LOGINTOPIC3 );
                        GAgent_SetCloudServerStatus( pgc,MQTT_STATUS_RES_LOGINTOPIC3 );
                     }
                     break;
                case MQTT_STATUS_RES_LOGINTOPIC3:
                      ret = Cloud_ResSubTopic(pMqttBuf,pgc->rtinfo.waninfo.mqttMsgsubid );
                     if(RET_SUCCESS!= ret )
                     {
                        GAgent_Printf(GAGENT_DEBUG," MQTT_STATUS_RES_LOGINTOPIC3 Fail ");
                        if( dTime > (pgc->rtinfo.waninfo.ReConnectMqttTime) )
                        {
                            timeoutFlag =1;
                            GAgent_Printf(GAGENT_DEBUG,"GAgent req sub LOGINTOPIC3 again." );
                            Cloud_ReqSubTopic( pgc,MQTT_STATUS_REQ_LOGINTOPIC3 );
                        }
                     }
                     else
                     {
                        GAgent_Printf(GAGENT_CRITICAL,"GAgent Cloud Working...");
                        GAgent_Printf(GAGENT_INFO,"system_get_free_heap_size = %d\n",system_get_free_heap_size());
                        GAgent_SetCloudServerStatus( pgc,MQTT_STATUS_RUNNING );
                        GAgent_Printf( GAGENT_INFO,"file:%s function:%s line:%d ",__FILE__,__FUNCTION__,__LINE__ );
                        GAgent_DevCheckWifiStatus( WIFI_CLOUD_CONNECTED,1 );
                        pgc->rtinfo.waninfo.send2MqttLastTime = 0;
                        pgc->rtinfo.waninfo.ReConnectMqttTime = 0;
                        //GAgent_SetWiFiStatus( pgc,WIFI_CLOUD_CONNECTED,1 );
                     }
                      break;
                default:
                     break;
            }
            if( 1==timeoutFlag )
            {
                pgc->rtinfo.waninfo.ReConnectMqttTime+=GAGENT_CLOUDREADD_TIME;
            }
            pgc->rtinfo.waninfo.send2MqttLastTime = GAgent_GetDevTime_S();
        }
        else if( /*packetLen>0 && */( mqttstatus == MQTT_STATUS_RUNNING ) )
        {
            switch( mqttpackType )
            {
                case MQTT_MSG_PINGRESP:
                    pgc->rtinfo.waninfo.cloudPingTime=0;
                    GAgent_Printf(GAGENT_CRITICAL,"GAgent Cloud Pong ... \r\n");
                break;
                case MQTT_MSG_PUBLISH:
                    dataLen = Mqtt_DispatchPublishPacket( pgc,pMqttBuf,packetLen );
                    if( dataLen>0 )
                    {
                        pbuf->type = SetPacketType( pbuf->type,CLOUD_DATA_IN,1 );
                        pbuf->type = SetPacketType( pbuf->type,LAN_TCP_DATA_IN,0 );
                        pbuf->type = SetPacketType( pbuf->type,LOCAL_DATA_IN,0 );
                        ParsePacket(  pbuf );
                        GAgent_Printf(GAGENT_INFO,"%s %d type : %04X len :%d",__FUNCTION__,__LINE__,pbuf->type,dataLen );
                    }
                break;
                default:
                    GAgent_Printf(GAGENT_WARNING," data form m2m but msg type is %d",mqttpackType );
                break;
            }
        }
    }
    return dataLen;
}

int32 ICACHE_FLASH_ATTR
GAgent_Cloud_GetPacket( pgcontext pgc,ppacket pRxbuf, int32 buflen)
{
    int32 Mret=0;
    uint16 GAgentstatus = 0;
    ppacket pbuf = pRxbuf;
    GAgentstatus = pgc->rtinfo.GAgentStatus;

    if( (GAgentstatus&WIFI_STATION_CONNECTED) != WIFI_STATION_CONNECTED)
        return -1 ;

//    Cloud_ConfigDataHandle( pgc );
    Mret = Cloud_M2MDataHandle( pgc,pbuf, buflen );
    return Mret;
}
void ICACHE_FLASH_ATTR
GAgent_Cloud_Handle( pgcontext pgc, ppacket Rxbuf,int32 length )
{
    int32 cloudDataLen = 0;

    cloudDataLen = GAgent_Cloud_GetPacket( pgc,Rxbuf ,length );
    if( cloudDataLen>0 )
    {
        dealPacket(pgc, Rxbuf);
//        Cloud_ClearClientAttrs(pgc, &pgc->rtinfo.waninfo.srcAttrs);
//        clearChannelAttrs(pgc);
    }
}
