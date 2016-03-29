#include "gagent.h"
#include "lan.h"
#include "cloud.h"
#include "iof_arch.h"
pgcontext pgContextData=NULL;

void GAgent_NewVar( pgcontext *pgc );

/****************************************************************
Function    :   GAgent_Init
Description :   GAgent init
pgc         :   global staruc pointer.
return      :   NULL
Add by Alex.lin     --2015-03-27
****************************************************************/
void ICACHE_FLASH_ATTR
GAgent_Init( pgcontext *pgc )
{
    GAgent_NewVar( pgc );
    GAgent_DevInit( *pgc );
    /* -1: no log */
    GAgent_loglevelSet(/*-1*//*GAGENT_DUMP*//*GAGENT_INFO *//*GAGENT_WARNING*/GAGENT_DEBUG); 

    GAgent_VarInit( pgc );
    GAgent_LocalInit( *pgc );
    GAgent_LANInit(*pgc);
    GAgent_WiFiInit( *pgc );

    GAgent_Printf( GAGENT_CRITICAL,"GAgent Start...");
}
/************************************* ***************************
Function    :   GAgent_NewVar
Description :   malloc New Var
pgc         :   global staruc pointer.
return      :   NULL
Add by Alex.lin     --2015-03-27
****************************************************************/
void ICACHE_FLASH_ATTR
GAgent_NewVar( pgcontext *pgc )
{
    *pgc = (pgcontext)os_malloc( sizeof( gcontext ));
    while(NULL == *pgc)
    {
        *pgc = (pgcontext)os_malloc( sizeof( gcontext ));
//        sleep(1);
    }
    os_memset(*pgc,0,sizeof(gcontext) );

    return ;
}

/****************************************************************
Function    :   GAgent_VarInit
Description :   init global var and malloc memory
pgc         :   global staruc pointer.
return      :   NULL
Add by Alex.lin     --2015-03-27
****************************************************************/
void ICACHE_FLASH_ATTR
GAgent_VarInit( pgcontext *pgc )
{
    int totalCap = BUF_LEN + BUF_HEADLEN;
    int bufCap = BUF_LEN;
    (*pgc)->rtinfo.firstStartUp = 1;

    //txbuf
    (*pgc)->rtinfo.Txbuf = (ppacket)os_malloc( sizeof(packet) );
    (*pgc)->rtinfo.Txbuf->allbuf = (uint8 *)os_malloc( totalCap );
    while( (*pgc)->rtinfo.Txbuf->allbuf==NULL )
    {
        (*pgc)->rtinfo.Txbuf->allbuf = (uint8 *)os_malloc( totalCap );
        //sleep(1);
    }
    os_memset( (*pgc)->rtinfo.Txbuf->allbuf,0,totalCap );
    (*pgc)->rtinfo.Txbuf->totalcap = totalCap;
    (*pgc)->rtinfo.Txbuf->bufcap = bufCap;
    resetPacket( (*pgc)->rtinfo.Txbuf );

    //udpbuf
    (*pgc)->rtinfo.Rxbuf = (ppacket)os_malloc( sizeof(packet) );
    (*pgc)->rtinfo.Rxbuf->allbuf = (uint8 *)os_malloc( totalCap );
    while( (*pgc)->rtinfo.Rxbuf->allbuf==NULL )
    {
        (*pgc)->rtinfo.Rxbuf->allbuf = (uint8 *)os_malloc( totalCap );
        //sleep(1);
    }
    os_memset( (*pgc)->rtinfo.Rxbuf->allbuf,0,totalCap );
    (*pgc)->rtinfo.Rxbuf->totalcap = totalCap;
    (*pgc)->rtinfo.Rxbuf->bufcap = bufCap;
    resetPacket( (*pgc)->rtinfo.Rxbuf );

    //uartbuf
    (*pgc)->rtinfo.UartRxbuf = (ppacket)os_malloc( sizeof(packet) );
    (*pgc)->rtinfo.UartRxbuf->allbuf = (uint8 *)os_malloc( totalCap );
    while( (*pgc)->rtinfo.UartRxbuf->allbuf==NULL )
    {
        (*pgc)->rtinfo.UartRxbuf->allbuf = (uint8 *)os_malloc( totalCap );
        //sleep(1);
    }
    os_memset( (*pgc)->rtinfo.UartRxbuf->allbuf,0,totalCap );
    (*pgc)->rtinfo.UartRxbuf->totalcap = totalCap;
    (*pgc)->rtinfo.UartRxbuf->bufcap = bufCap;
    resetPacket( (*pgc)->rtinfo.UartRxbuf );

     //tcpbuf
    (*pgc)->rtinfo.TcpRxbuf = (ppacket)os_malloc( sizeof(packet) );
    (*pgc)->rtinfo.TcpRxbuf->allbuf = (uint8 *)os_malloc( totalCap );
    while( (*pgc)->rtinfo.TcpRxbuf->allbuf==NULL )
    {
        (*pgc)->rtinfo.TcpRxbuf->allbuf = (uint8 *)os_malloc( totalCap );
        //sleep(1);
    }
    os_memset( (*pgc)->rtinfo.TcpRxbuf->allbuf,0,totalCap );
    (*pgc)->rtinfo.TcpRxbuf->totalcap = totalCap;
    (*pgc)->rtinfo.TcpRxbuf->bufcap = bufCap;
    resetPacket( (*pgc)->rtinfo.TcpRxbuf );

    //httpbuf
    (*pgc)->rtinfo.HttpRxbuf = (ppacket)os_malloc( sizeof(packet) );
    (*pgc)->rtinfo.HttpRxbuf->allbuf = (uint8 *)os_malloc( totalCap+512 );
    while( (*pgc)->rtinfo.HttpRxbuf->allbuf==NULL )
    {
        (*pgc)->rtinfo.HttpRxbuf->allbuf = (uint8 *)os_malloc( totalCap+512 );
        //sleep(1);
    }
    os_memset( (*pgc)->rtinfo.HttpRxbuf->allbuf,0,totalCap+512 );
    (*pgc)->rtinfo.HttpRxbuf->totalcap = totalCap+512;
    (*pgc)->rtinfo.HttpRxbuf->bufcap = bufCap+512;
    resetPacket( (*pgc)->rtinfo.HttpRxbuf);

    //mqttbuf
    (*pgc)->rtinfo.MqttRxbuf = (ppacket)os_malloc( sizeof(packet) );
    (*pgc)->rtinfo.MqttRxbuf->allbuf = (uint8 *)os_malloc( totalCap );
    while( (*pgc)->rtinfo.MqttRxbuf->allbuf==NULL )
    {
        (*pgc)->rtinfo.MqttRxbuf->allbuf = (uint8 *)os_malloc( totalCap );
        //sleep(1);
    }
    os_memset( (*pgc)->rtinfo.MqttRxbuf->allbuf,0,totalCap );
    (*pgc)->rtinfo.MqttRxbuf->totalcap = totalCap;
    (*pgc)->rtinfo.MqttRxbuf->bufcap = bufCap;
    resetPacket( (*pgc)->rtinfo.MqttRxbuf );

    //pRxBuf
    (*pgc)->rtinfo.pRxBuf = (int8 *)os_malloc( BUF_LEN );
    while( (*pgc)->rtinfo.pRxBuf==NULL )
    {
        (*pgc)->rtinfo.pRxBuf = (int8 *)os_malloc( BUF_LEN );
        //sleep(1);
    }


    /* get config data form flash */
    GAgent_DevGetConfigData( &(*pgc)->gc );
    (*pgc)->rtinfo.waninfo.CloudStatus=CLOUD_INIT;

    /* get mac address */
    GAgent_DevGetMacAddress((*pgc)->minfo.szmac);
    os_memcpy( (*pgc)->minfo.ap_name,AP_NAME,os_strlen(AP_NAME));
    os_memcpy( (*pgc)->minfo.ap_name+os_strlen(AP_NAME),(*pgc)->minfo.szmac+8,4);

    (*pgc)->minfo.ap_name[os_strlen(AP_NAME)+4]= '\0';

    if((*pgc)->gc.magicNumber != GAGENT_MAGIC_NUM)
    {
        GAgent_Printf( GAGENT_INFO,"GAGENT_MAGIC_NUM different ...");
        os_memset(&((*pgc)->gc), 0, sizeof(GAGENT_CONFIG_S));
        (*pgc)->gc.magicNumber = GAGENT_MAGIC_NUM;

        GAgent_DevCheckWifiStatus( WIFI_MODE_ONBOARDING,1 );
    }
    else
    {
        if( os_strlen( (*pgc)->gc.DID )!=(DID_LEN-2) )
            os_memset( ((*pgc)->gc.DID ),0,DID_LEN );

        if( os_strlen( (*pgc)->gc.old_did )!=(DID_LEN-2))
            os_memset( ((*pgc)->gc.old_did ),0,DID_LEN );

        if( os_strlen( ((*pgc)->gc.wifipasscode) ) != PASSCODE_LEN )
        {
            os_memset( ((*pgc)->gc.wifipasscode ),0,PASSCODE_MAXLEN + 1);
            make_rand( (*pgc)->gc.wifipasscode );
        }
        if( os_strlen( ((*pgc)->gc.old_wifipasscode) )!=PASSCODE_LEN || os_strlen( ((*pgc)->gc.old_did) )!= (DID_LEN-2) )
        {
            os_memset( ((*pgc)->gc.old_wifipasscode ),0,PASSCODE_LEN );
            os_memset( ((*pgc)->gc.old_did ),0,DID_LEN );
        }

        if( os_strlen( ((*pgc)->gc.old_productkey) )!=(PK_LEN) )
            os_memset( (*pgc)->gc.old_productkey,0,PK_LEN + 1 );

        if( os_strlen( (*pgc)->gc.m2m_ip)>IP_LEN_MAX || os_strlen( (*pgc)->gc.m2m_ip)<IP_LEN_MIN )
            os_memset( (*pgc)->gc.m2m_ip,0,IP_LEN_MAX + 1 );

        if( os_strlen( (*pgc)->gc.GServer_ip)>IP_LEN_MAX || os_strlen( (*pgc)->gc.GServer_ip)<IP_LEN_MIN )
            os_memset( (*pgc)->gc.GServer_ip,0,IP_LEN_MAX + 1 );

        if( os_strlen( (*pgc)->gc.cloud3info.cloud3Name )>CLOUD3NAME )
            os_memset( (*pgc)->gc.cloud3info.cloud3Name,0,CLOUD3NAME );
    }
    (*pgc)->rtinfo.waninfo.ReConnectMqttTime = GAGENT_MQTT_TIMEOUT;
    (*pgc)->rtinfo.waninfo.ReConnectHttpTime = GAGENT_HTTP_TIMEOUT;
    (*pgc)->rtinfo.waninfo.send2HttpLastTime = GAgent_GetDevTime_S();
    (*pgc)->rtinfo.waninfo.firstConnectHttpTime = GAgent_GetDevTime_S();
    (*pgc)->rtinfo.waninfo.httpCloudPingTime = 0;
    (*pgc)->rtinfo.waninfo.RefreshIPLastTime = 0;
    (*pgc)->rtinfo.waninfo.RefreshIPTime = 0;
    (*pgc)->rtinfo.wifiLastScanTime = GAGENT_STA_SCANTIME;
    (*pgc)->rtinfo.webconfigflag = RET_FAILED;
    (*pgc)->rtinfo.file.using = 0;
    (*pgc)->rtinfo.OTATypeflag = OTATYPE_WIFI;
    (*pgc)->rtinfo.onlinePushflag = 0;
    (*pgc)->rtinfo.bigdataUploadflag = 0;
    (*pgc)->rtinfo.reqFirewareLenflag = 1;
    (*pgc)->rtinfo.m2mDnsflag = RET_FAILED;
    (*pgc)->mcu.mcu_upgradeflag = 1;
    (*pgc)->mcu.uiBaund = 9600;

    Cloud_ClearClientAttrs(*pgc, &((*pgc)->rtinfo.waninfo.srcAttrs));
    GAgent_DevSaveConfigData( &((*pgc)->gc) );
}

void ICACHE_FLASH_ATTR
GAgent_dumpInfo( pgcontext pgc )
{
    GAgent_Printf(GAGENT_DEBUG,"Product Soft Version: %s. Hard Version: %s", WIFI_SOFTVAR,WIFI_HARDVER);
    GAgent_Printf(GAGENT_DEBUG,"GAgent Compiled Time: %s, %s.\r\n",__DATE__, __TIME__);
    GAgent_Printf(GAGENT_DEBUG,"GAgent mac :%s",pgc->minfo.szmac );
    GAgent_Printf(GAGENT_DEBUG,"GAgent passcode :%s len=%d",pgc->gc.wifipasscode,os_strlen( pgc->gc.wifipasscode ) );
    GAgent_Printf(GAGENT_DEBUG,"GAgent did :%s len:%d",pgc->gc.DID,os_strlen(pgc->gc.DID) );
    GAgent_Printf(GAGENT_DEBUG,"GAgent old did :%s len:%d",pgc->gc.old_did,os_strlen(pgc->gc.old_did) );
    GAgent_Printf(GAGENT_DEBUG,"GAgent old pk :%s len:%d",pgc->gc.old_productkey,os_strlen(pgc->gc.old_productkey) );
    GAgent_Printf(GAGENT_DEBUG,"GAgent AP name:%s",pgc->minfo.ap_name );
    //GAgent_Printf(GAGENT_DEBUG,"GAgent 3rd cloud :%s",pgc->gc.cloud3info.cloud3Name );
    GAgent_Printf(GAGENT_DEBUG,"GAgent M2M IP :%d.%d.%d.%d",
        pgc->gc.m2m_ip[0],pgc->gc.m2m_ip[1],pgc->gc.m2m_ip[2],pgc->gc.m2m_ip[3] );
    GAgent_Printf(GAGENT_DEBUG,"GAgent GService IP :%d.%d.%d.%d",pgc->gc.GServer_ip[0],
        pgc->gc.GServer_ip[1],pgc->gc.GServer_ip[2],pgc->gc.GServer_ip[3]);
    return ;
}
/****************************** running status ******************************/
/*
    flag=1 set GAgentStatus
    flag=0 reset GAgentStatus
*/
void ICACHE_FLASH_ATTR
GAgent_SetWiFiStatus( pgcontext pgc,uint16 GAgentStatus,int8 flag )
{
    if(flag==1)
    {
        pgc->rtinfo.GAgentStatus |= GAgentStatus;
    }
    else
    {
        pgc->rtinfo.GAgentStatus &=~ GAgentStatus;
    }
    return ;
}

void ICACHE_FLASH_ATTR
GAgent_SetCloudConfigStatus( pgcontext pgc,int16 cloudstauts )
{
    pgc->rtinfo.waninfo.CloudStatus = cloudstauts;
    /*g_globalvar.waninfo.CloudStatus = cloudstauts;*/
    return ;
}

void ICACHE_FLASH_ATTR
GAgent_SetCloudServerStatus( pgcontext pgc,int16 serverstatus )
{
    pgc->rtinfo.waninfo.mqttstatus = serverstatus;
    /*g_globalvar.waninfo.mqttstatus = serverstatus;*/
    return ;
}

/****************************************************************
*       functionName    :   GAgent_SetGServerIP
*       description     :   set the  Gserver ip into configdata
*       Input           :   gserver ip string like "192.168.1.1"
*       return          :   =0 set Gserver ip ok
*                       :   other fail
*       add by Alex.lin     --2015-03-02
****************************************************************/
int8 ICACHE_FLASH_ATTR
GAgent_SetGServerIP( pgcontext pgc,int8 *szIP )
{
    /*strcpy( g_stGAgentConfigData.GServer_ip,szIP );*/
    os_strcpy( pgc->gc.GServer_ip,szIP );
    GAgent_DevSaveConfigData( &(pgc->gc) );
    return 0;
}
/****************************************************************
*       functionName    :   GAgent_SetGServerSocket
*       description     :   set the  Gserver socket val
*       pgc             :   global struct.
*       socketid        :   the socketid will set into GServer socket
*       return          :   =0 set ok
*                       :   other fail
*       add by Alex.lin     --2015-03-02
****************************************************************/
int8 ICACHE_FLASH_ATTR
GAgent_SetGServerSocket( pgcontext pgc,int32 socketid )
{
    pgc->rtinfo.waninfo.http_socketid = socketid;
    /*g_globalvar.waninfo.http_socketid = socketid;*/
    return 0;
}
/****************************** config status ******************************/
uint8 ICACHE_FLASH_ATTR
GAgent_SetDeviceID( pgcontext pgc,int8 *p_szDeviceID )
{
    if( p_szDeviceID != NULL )
    {
        os_strcpy( pgc->gc.DID,p_szDeviceID );
    }
    else
    {
        os_memset( pgc->gc.DID,0,DID_LEN );
    }
    GAgent_DevSaveConfigData( &(pgc->gc) );
    return 0;
}
/****************************************************************
*       FunctionName      :     GAgent_SetOldDeviceID
*       Description       :     reset the old did and passcode
*       flag              :     0 reset to NULL
*                               1 set the new did and passcode
*                                 to old info.
*      Add by Alex.lin      --2015-03-02
****************************************************************/
int8 ICACHE_FLASH_ATTR
GAgent_SetOldDeviceID( pgcontext pgc,int8* p_szDeviceID,int8* p_szPassCode,int8 flag )
{
    /*
    memset( g_stGAgentConfigData.old_did,0,24 );
    memset( g_stGAgentConfigData.old_wifipasscode,0,16 );
    */
    os_memset( pgc->gc.old_did,0,DID_LEN );
    os_memset( pgc->gc.old_wifipasscode,0,PASSCODE_LEN );
    if( 1 == flag )
    {
        os_strcpy( pgc->gc.old_did,p_szDeviceID );
        os_strcpy( pgc->gc.old_wifipasscode,p_szPassCode );
    }
    GAgent_DevSaveConfigData( &(pgc->gc) );
    return 0;
}

/*
    return 0 : don't need to disable did.
           1 : need to disable did
*/
int8 ICACHE_FLASH_ATTR
GAgent_IsNeedDisableDID( pgcontext pgc )
{
    uint32 didLen=0,passcodeLen=0;
    didLen = os_strlen( pgc->gc.old_did );
    passcodeLen = os_strlen( pgc->gc.old_wifipasscode );
    if( (0==didLen)|| ( 22<didLen) || (passcodeLen==0) || (passcodeLen>16) ) /* invalid did length or passcode length */
    {
        os_memset( pgc->gc.old_did,0,DID_LEN );
        os_memset( pgc->gc.old_wifipasscode,0,PASSCODE_LEN );
        GAgent_DevSaveConfigData( &(pgc->gc) );
        return 0;
    }
    return 1;
}
void ICACHE_FLASH_ATTR
GAgent_loglevelSet( uint16 level )
{
    pgContextData->rtinfo.loglevel = level;
}
int8 ICACHE_FLASH_ATTR
GAgent_loglevelenable( uint16 level )
{
    if( level > pgContextData->rtinfo.loglevel )
        return 1;
    else
        return 0;
}
/****************************************************************
*       FunctionName      :     GAgent_RefreshIPTick
*       Description       :     update ip tick,if gethostbyname
                                ok will set time to one hour
*      Add by Alex.lin      --2015-04-23
****************************************************************/
void ICACHE_FLASH_ATTR
GAgent_RefreshIPTick( pgcontext pgc,uint32 dTime_s )
{
    uint32 cTime=0;
    int8 tmpip[32] = {0},failed=0,ret=0;

    if( ((pgc->rtinfo.GAgentStatus)&WIFI_MODE_TEST) == WIFI_MODE_TEST )
    {
        GAgent_Printf( GAGENT_INFO,"In WIFI_MODE_TEST...");
        return ;
    }
    if( ((pgc->rtinfo.GAgentStatus)&WIFI_STATION_CONNECTED)!=WIFI_STATION_CONNECTED )
    {
        pgc->rtinfo.waninfo.RefreshIPTime =  1;
        GAgent_Printf( GAGENT_INFO," not in WIFI_STATION_CONNECTED ");
        return ;
    }
    pgc->rtinfo.waninfo.RefreshIPLastTime+=dTime_s;
    if( (pgc->rtinfo.waninfo.RefreshIPLastTime) >= (pgc->rtinfo.waninfo.RefreshIPTime) )
    {
        GAgent_Printf( GAGENT_DEBUG,"GAgentStatus:%04x",(pgc->rtinfo.GAgentStatus) );
        GAgent_Printf( GAGENT_DEBUG,"RefreshIPTime=%d ms,lsst:%d,ctimd %d",(pgc->rtinfo.waninfo.RefreshIPTime),(pgc->rtinfo.waninfo.RefreshIPLastTime) ,cTime);
        GAgent_Printf( GAGENT_DEBUG,"RefreshIPTime=%d s",(pgc->rtinfo.waninfo.RefreshIPTime) );
        pgc->rtinfo.waninfo.RefreshIPLastTime = 0;

        if( ((pgc->rtinfo.GAgentStatus)&WIFI_STATION_CONNECTED) != WIFI_STATION_CONNECTED )
        {
            pgc->rtinfo.waninfo.RefreshIPTime =  1;
        }
        ret = GAgent_GetHostByName(HTTP_SERVER, tmpip,1);

        if( ret!=0 && ret!=-5 )
        {
            GAgent_Printf( GAGENT_ERROR,"http dns failed ret = %d\n",ret );
            failed=1;
        }
        else //do it in cb
        {
//            GAgent_Printf( GAGENT_DEBUG,"HTTP_SERVER %s : %s",HTTP_SERVER,tmpip);
//            if(os_strcmp( pgc->gc.GServer_ip, tmpip) != 0)
//            {
//                GAgent_Printf( GAGENT_DEBUG,"Save GService ip into flash!");
//                os_strcpy(pgc->gc.GServer_ip, tmpip );
//                GAgent_DevSaveConfigData( &(pgc->gc) );
//            }
        }

        if(os_strlen(pgc->minfo.m2m_SERVER) == 0)
        {
           GAgent_Printf(GAGENT_DEBUG," m2m server is NULL!");
            //not yet m2m host
            failed = 1;
        }
        else
        {
            ret = GAgent_GetHostByName( pgc->minfo.m2m_SERVER,tmpip,2 );
            if( ret!=0 && ret!=-5)
            {
                GAgent_Printf( GAGENT_DEBUG,"m2m dns failed!" );
                failed = 1;
            }
            else //do it in cb
            {
//                GAgent_Printf( GAGENT_DEBUG,"got %s : %s",pgc->minfo.m2m_SERVER,tmpip);
//                if( 0!=os_strcmp( pgc->gc.m2m_ip,tmpip) )
//                {
//                    os_strcpy( pgc->gc.m2m_ip,tmpip );
//                    GAgent_DevSaveConfigData( &(pgc->gc) );
//                    GAgent_DevGetConfigData( &(pgc->gc) );
//                }
            }

         }

        if( 1 == failed )
        {
            if( ((pgc->rtinfo.GAgentStatus)&WIFI_MODE_TEST) == WIFI_MODE_TEST )
            {
                pgc->rtinfo.waninfo.RefreshIPTime =  1*10;
            }
            else
            {
                pgc->rtinfo.waninfo.RefreshIPTime = 1;
            }
        }
        else
        {
            pgc->rtinfo.waninfo.RefreshIPTime = ONE_HOUR;
        }

    }
}

/******************************************************
 *      FUNCTION        :   update info
 *      new_pk          :   new productkey
 *   Add by Alex lin  --2014-12-19
 *
 ********************************************************/
void ICACHE_FLASH_ATTR
GAgent_UpdateInfo( pgcontext pgc,uint8 *new_pk )
{
    GAgent_Printf(GAGENT_DEBUG,"a new productkey is :%s.",new_pk);
    /*the necessary information to disable devices*/
    os_memset( pgc->gc.old_did,0,DID_LEN);
    os_memset( pgc->gc.old_wifipasscode,0,PASSCODE_MAXLEN + 1);
    /*存到old字段用于注销设备*/
    os_memcpy( pgc->gc.old_did,pgc->gc.DID,DID_LEN);
    os_memcpy( pgc->gc.old_wifipasscode,pgc->gc.wifipasscode,PASSCODE_MAXLEN + 1);

    os_memset( pgc->gc.old_productkey,0,PK_LEN + 1);
    os_memcpy( pgc->gc.old_productkey,new_pk,PK_LEN + 1);
    pgc->gc.old_productkey[PK_LEN] = '\0';

    /*neet to reset info */
    os_memset( pgc->gc.FirmwareVer,0,FIRMWARE_LEN_MAX + 1);
    os_memset( pgc->gc.FirmwareVerLen,0,2);
    os_memset( &(pgc->gc.cloud3info),0,sizeof(pgc->gc.cloud3info));
    os_memset( pgc->gc.DID,0,DID_LEN );

   /*生成新的wifipasscode*/
    make_rand(pgc->gc.wifipasscode);
    GAgent_DevSaveConfigData( &(pgc->gc) );
}
/******************************************************
 *      FUNCTION        :   uGAgent_Config
 *      typed           :   1:AP MODE 2:Airlink
 *   Add by Alex lin  --2014-12-19
 *
 ********************************************************/
void ICACHE_FLASH_ATTR
GAgent_Config( uint8 typed,pgcontext pgc )
{
    GAgent_SetWiFiStatus( pgc,WIFI_MODE_TEST,0 );
//    pgc->rtinfo.waninfo.CloudStatus = CLOUD_INIT;
//    pgc->rtinfo.waninfo.RefreshIPTime = 0;


    switch( typed )
    {
        //AP MODE
        case 1:
             GAgent_Printf( GAGENT_DEBUG,"file:%s function:%s line:%d ",__FILE__,__FUNCTION__,__LINE__ );
             GAgent_DevCheckWifiStatus( WIFI_MODE_ONBOARDING,1 );
        break;
        //Airlink
        case 2:
        {
            pgc->rtinfo.isAirlinkConfig = 1;
            GAgent_DevCheckWifiStatus( WIFI_MODE_ONBOARDING,1 );
            int8 timeout;
            uint16 tempWiFiStatus=0;
            timeout = 60 * 1000;

            tempWiFiStatus = pgc->rtinfo.GAgentStatus;
            pgc->gc.flag  &=~ XPG_CFG_FLAG_CONFIG;
            GAgent_OpenAirlink( timeout );
            GAgent_Printf( GAGENT_INFO,"OpenAirlink...");
            os_timer_disarm(&(pgc->rtinfo.AirLinkTimer));
            os_timer_setfn(&(pgc->rtinfo.AirLinkTimer), (os_timer_func_t *)AirLinkResult, pgContextData);
            os_timer_arm(&(pgc->rtinfo.AirLinkTimer), 60000, 0);
//            while( timeout )
//            {
//                timeout--;
//                msleep(1);
//                if( (pgc->gc.flag & XPG_CFG_FLAG_CONFIG) ==XPG_CFG_FLAG_CONFIG )
//                {
//                    GAgent_Printf( GAGENT_INFO,"AirLink result ssid:%s key:%s",pgc->gc.wifi_ssid,pgc->gc.wifi_key );
//                    tempWiFiStatus |=WIFI_MODE_STATION;
//                    pgc->gc.flag |= XPG_CFG_FLAG_CONNECTED;
//                    pgc->ls.onboardingBroadCastTime = SEND_UDP_DATA_TIMES;
//                    GAgent_DevSaveConfigData( &(pgc->gc) );
//                    tempWiFiStatus |= GAgent_DRVWiFi_StationCustomModeStart( pgc->gc.wifi_ssid,pgc->gc.wifi_key,tempWiFiStatus );
//                    GAgent_WiFiInit( pgc );
//                      CreateUDPBroadCastServer( pgc );
//                    break;
//                }
//                GAgent_DevLED_Green( (timeout%2) );
//            }
//            GAgent_DevCheckWifiStatus( WIFI_MODE_ONBOARDING,0 );
//            if( timeout<=0 )
//            {
//                    GAgent_DevCheckWifiStatus( WIFI_MODE_ONBOARDING,1 );
//                    GAgent_Printf( GAGENT_INFO,"AirLink Timeout ...");
//                    GAgent_Printf( GAGENT_INFO,"Into SoftAp Config...");
//            }

        break;
        }
        default :
        break;
    }
}
uint8 ICACHE_FLASH_ATTR
GAgent_EnterTest( pgcontext pgc )
{
    pgc->rtinfo.scanWifiFlag = 0;
    os_memset( pgc->gc.GServer_ip,0,IP_LEN_MAX+1);
    os_memset( pgc->gc.m2m_ip,0,IP_LEN_MAX+1);

    GAgent_DevSaveConfigData( &(pgc->gc) );
    GAgent_SetWiFiStatus( pgc,WIFI_MODE_TEST,1 );
    GAgent_DRVWiFiStartScan();
    return 0;
}
uint8 ICACHE_FLASH_ATTR
GAgent_ExitTest( pgcontext pgc )
{
    pgc->rtinfo.scanWifiFlag = 0;
    GAgent_DRVWiFi_StationDisconnect();
    GAgent_SetWiFiStatus( pgc,WIFI_MODE_TEST,0 );
    GAgent_DRVWiFiStopScan( );
    GAgent_EnterNullMode( pgc );
    return 0;
}
/****************************************************************
return      :       RET_SUCCESS  success
            :       RET_FAILED   fail
****************************************************************/
int32 ICACHE_FLASH_ATTR
GAgent_Cloud_OTAByUrl( pgcontext pgc,int8 *downloadUrl,OTATYPE otatype )
{
    int32 ret;
    if( OTATYPE_WIFI == otatype )
    {
        GAgent_Printf( GAGENT_CRITICAL,"start WIFI OTA...\n");
        ret = GAgent_WIFIOTAByUrl( pgc, downloadUrl );

        return RET_SUCCESS;
    }
    else
    {
         GAgent_Printf( GAGENT_CRITICAL,"start MCU OTA...\n");
         return GAgent_MCUOTAByUrl( pgc, downloadUrl );
    }
}
/****************************************************************
FunctionName        :   GAgent_GetStaWiFiLevel
Description         :   return the wifi level in Sta mode.
wifiRSSI            :   the wifi signal strength(0-100)
return              :   0-7(0:min,7:max)
Add by Alex.lin     --2015-05-26
****************************************************************/
int8 ICACHE_FLASH_ATTR
GAgent_GetStaWiFiLevel( int8 wifiRSSI )
{
    if( wifiRSSI==WIFI_LEVEL_0)
        return 0;
    if( (wifiRSSI>WIFI_LEVEL_0)&&(wifiRSSI<WIFI_LEVEL_1))
        return 1;
    if( (wifiRSSI>=WIFI_LEVEL_1)&&(wifiRSSI<WIFI_LEVEL_2))
        return 2;
    if( (wifiRSSI>=WIFI_LEVEL_2)&&(wifiRSSI<WIFI_LEVEL_3))
        return 3;
    if( (wifiRSSI>=WIFI_LEVEL_3)&&(wifiRSSI<WIFI_LEVEL_4))
        return 4;
    if( (wifiRSSI>=WIFI_LEVEL_4)&&(wifiRSSI<WIFI_LEVEL_5))
        return 5;
    if( (wifiRSSI>=WIFI_LEVEL_5)&&(wifiRSSI<WIFI_LEVEL_6))
        return 6;
    if( wifiRSSI>=WIFI_LEVEL_6 )
        return 7;
    return RET_FAILED;
}

void ICACHE_FLASH_ATTR
GAgent_Tick( pgcontext pgc )
{
    if(RET_SUCCESS != pgc->rtinfo.getInfoflag)
    {
        return;
    }
    GAgent_DevTick();
    GAgent_CloudTick( pgc,1 );
    GAgent_LocalTick( pgc,1 );
    GAgent_LanTick( pgc,1 );
    GAgent_WiFiEventTick( pgc,1 );
    GAgent_RefreshIPTick( pgc,1 );
    GAgent_BigDataTick( pgc );
}

