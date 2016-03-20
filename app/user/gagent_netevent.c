#include "gagent.h"
#include "cloud.h"
#include "platform.h"


void ICACHE_FLASH_ATTR
GAgent_WiFiInit( pgcontext pgc )
{
    uint16 tempWiFiStatus=0;
    tempWiFiStatus = pgc->rtinfo.GAgentStatus;

    if( (pgc->gc.flag & XPG_CFG_FLAG_CONFIG_AP) == XPG_CFG_FLAG_CONFIG_AP )
    {
        GAgent_Printf( GAGENT_DEBUG," GAgent XPG_CFG_FLAG_CONFIG_AP." );
        GAgent_DevCheckWifiStatus( WIFI_MODE_ONBOARDING,1 );
    }

    if( GAgent_DRVBootConfigWiFiMode() != GAgent_DRVGetWiFiMode(pgc) )
    {
        if( ((pgc->gc.flag)&XPG_CFG_FLAG_CONNECTED) == XPG_CFG_FLAG_CONNECTED )
        {
            GAgent_Printf( GAGENT_INFO,"In Station mode");
            GAgent_Printf( GAGENT_INFO,"SSID:%s,KEY:%s",pgc->gc.wifi_ssid,pgc->gc.wifi_key );
            tempWiFiStatus |=WIFI_MODE_STATION;
            tempWiFiStatus |= GAgent_DRVWiFi_StationCustomModeStart( pgc->gc.wifi_ssid,pgc->gc.wifi_key, tempWiFiStatus );
        }
        else
        {
            GAgent_Printf( GAGENT_CRITICAL,"In AP mode");
            tempWiFiStatus |=WIFI_MODE_AP;
            os_memset( pgc->gc.wifi_ssid,0,SSID_LEN_MAX+1 );
            os_memset( pgc->gc.wifi_key,0,WIFIKEY_LEN_MAX+1 );
            GAgent_DevSaveConfigData( &(pgc->gc));
            GAgent_DevGetConfigData( &(pgc->gc));
            tempWiFiStatus |= GAgent_DRV_WiFi_SoftAPModeStart( pgc->minfo.ap_name,AP_PASSWORD,tempWiFiStatus );

        }
    }
    else
    {
        if(2 == GAgent_DRVBootConfigWiFiMode())
        {
            os_memset( pgc->gc.wifi_ssid,0,SSID_LEN_MAX+1 );
            os_memset( pgc->gc.wifi_key,0,WIFIKEY_LEN_MAX+1 );
            GAgent_DevSaveConfigData( &(pgc->gc));
            GAgent_DevGetConfigData( &(pgc->gc));
            GAgent_DRV_WiFi_SoftAPModeStart( pgc->minfo.ap_name,AP_PASSWORD,tempWiFiStatus );
        }
        if(1 == GAgent_DRVBootConfigWiFiMode())
        {
            GAgent_DRVWiFi_StationCustomModeStart( pgc->gc.wifi_ssid,pgc->gc.wifi_key, tempWiFiStatus );
        }
    }
}
/****************************************************************
Function    :   GAgent_DevCheckWifiStatus
Description :   check the wifi status and will set the wifi status
                and return it.
wifistatus  :   =0XFFFF get the wifistatus.
                !=0XFFFF & flag=1 set the wifistatus to the wifistatus.
                !=0xFFFF & flag=0 reset the wifistatus.
eg.
set the "WIFI_STATION_CONNECTED" into wifistatus,like:
    GAgent_DevCheckWifiStatus( WIFI_STATION_CONNECTED,1  );
reset the "WIFI_STATION_CONNECTED" from wifistatus,like:
    GAgent_DevCheckWifiStatus( WIFI_STATION_CONNECTED,0  );
return      :   the new wifi status.
Add by Alex.lin     --2015-04-17.
****************************************************************/
uint16 ICACHE_FLASH_ATTR
GAgent_DevCheckWifiStatus( uint16 wifistatus,int8 flag  )
{
    static uint16 halWiFiStatus=0;

    if( 0xFFFF==wifistatus )
    {
        GAgent_Printf( GAGENT_DEBUG," GAgent Get Hal wifiStatus :%04X ",halWiFiStatus );
       return halWiFiStatus;
    }
    else
    {
        if( 1==flag )
        {
            //对应位置1
            halWiFiStatus |=wifistatus;
            GAgent_Printf( GAGENT_DEBUG,"GAgent Hal Set wifiStatus%04X",wifistatus);
        }
        else
        {   //对应位清零
            uint16 tempstatus=0;
            tempstatus = ( 0xFFFF - wifistatus );
            halWiFiStatus &=tempstatus;
            GAgent_Printf( GAGENT_DEBUG,"GAgent Hal ReSet wifiStatus%04X",wifistatus);
        }

    }
    GAgent_Printf( GAGENT_DEBUG," GAgent Hal wifiStatus :%04X ",halWiFiStatus );
    return halWiFiStatus;
}
/****************************************************************
FunctionName    :   GAgentFindTestApHost.
Description     :   find the test ap host,like:GIZWITS_TEST_*
NetHostList_str :   GAgent wifi scan result .
return          :   1-GIZWITS_TEST_1
                    2-GIZWITS_TEST_2
                    fail :0.
Add by Alex.lin         --2015-05-06
****************************************************************/
uint8 ICACHE_FLASH_ATTR
GAgentFindTestApHost( NetHostList_str *pAplist )
{
    int16 i=0;
    int8 apNum=0,ret=0;

    for( i=0;i<pAplist->ApNum;i++ )
    {
        if( 0 == memcmp(pAplist->ApList[i].ssid,(int8 *)GAGENT_TEST_AP1,strlen(GAGENT_TEST_AP1)) )
        {
            if( 2 == apNum )
            {
                apNum = 3;
                break;
            }
            else
            {
                apNum = 1;
            }
        }
        if( 0 == memcmp(pAplist->ApList[i].ssid,(int8 *)GAGENT_TEST_AP2,strlen(GAGENT_TEST_AP2)) )
        {
            /* 两个热点都能找到 */
            if( 1 == apNum )
            {
              apNum = 3;
              break;
            }
            else
            {
              apNum = 2;
            }
        }
    }

    switch( apNum )
    {
        /* only the GIZWITS_TEST_1 */
        case 1:
            ret=1;
        break;
        /* only the GIZWITS_TEST_2 */
        case 2:
            ret=2;
        break;
        /* both of the test ap */
        case 3:
           srand(GAgent_GetDevTime_MS());
           ret = rand()%100;
           ret = (ret%2)+1;
        break;
        default:
        ret =0;
        break;
    }

//    if(NULL !=  pAplist->ApList)
//            os_free( pAplist->ApList);

    return ret;
}
/****************************************************************
FunctionName    :   GAgent_EnterNullMode
Description     :   GAgent Stop ap mode and status mode
Add by Alex.lin         --2015-09-07
****************************************************************/
void ICACHE_FLASH_ATTR
GAgent_EnterNullMode( pgcontext pgc )
{
     GAgent_Printf( GAGENT_INFO,"%s %d",__FUNCTION__,__LINE__ );
     GAgent_DRVWiFi_APModeStop( pgc );
     GAgent_DRVWiFi_StationDisconnect( );
     GAgent_STANotConnAP();
}

void  ICACHE_FLASH_ATTR
GAgent_WiFiEventTick( pgcontext pgc,uint32 dTime_s )
{
    uint16 newStatus=0;
    uint16 gagentWiFiStatus=0;
    static uint32 gagentOnboardingTime=0;

    gagentWiFiStatus = ( (pgc->rtinfo.GAgentStatus)&(LOCAL_GAGENTSTATUS_MASK) ) ;
    newStatus = GAgent_DevCheckWifiStatus( 0xffff,1 );
    GAgent_Printf( GAGENT_INFO,"wifiStatus : %04x new:%04x", gagentWiFiStatus,newStatus );
    if( (gagentWiFiStatus&WIFI_MODE_AP) != ( newStatus&WIFI_MODE_AP) )
    {
        if( newStatus&WIFI_MODE_AP )
        {
            //WIFI_MODE_AP UP
            GAgent_SetWiFiStatus( pgc,WIFI_MODE_AP,1 );
            gagentOnboardingTime = GAGENT_ONBOARDING_TIME;
        }
        else
        {
            //WIFI_MODE_AP DOWN
            GAgent_SetWiFiStatus( pgc,WIFI_MODE_AP,0 );
            gagentOnboardingTime = GAGENT_ONBOARDING_TIME;
        }
        pgc->rtinfo.waninfo.wanclient_num=0;
        pgc->ls.tcpClientNums=0;
        if( pgc->rtinfo.waninfo.CloudStatus == CLOUD_CONFIG_OK )
        {
            GAgent_SetCloudServerStatus( pgc,MQTT_STATUS_START );
        }
        else
        {
            GAgent_SetCloudConfigStatus( pgc,CLOUD_INIT );
        }
        newStatus = GAgent_DevCheckWifiStatus( WIFI_CLOUD_CONNECTED,0 );
        gagentWiFiStatus = ( (pgc->rtinfo.GAgentStatus)&(LOCAL_GAGENTSTATUS_MASK) ) ;
        //GAgent_SetWiFiStatus( pgc,WIFI_CLOUD_CONNECTED,0 );
    }
    if( (gagentWiFiStatus&WIFI_MODE_STATION) != ( newStatus&WIFI_MODE_STATION) )
    {
        if( newStatus&WIFI_MODE_STATION )
        {
            //WIFI_MODE_STATION UP
            GAgent_Printf( GAGENT_INFO,"WIFI_MODE_STATION UP." );
            GAgent_SetWiFiStatus( pgc,WIFI_MODE_STATION,1 );
            pgc->rtinfo.waninfo.ReConnectMqttTime = 0;
        }
        else
        {
            //WIFI_MODE_STATION DOWN
            GAgent_Printf( GAGENT_INFO,"WIFI_MODE_STATION Down." );
            GAgent_SetWiFiStatus( pgc,WIFI_MODE_STATION,0 );
            newStatus = GAgent_DevCheckWifiStatus( WIFI_CLOUD_CONNECTED,0 );
            if( pgc->rtinfo.waninfo.CloudStatus == CLOUD_CONFIG_OK )
            {
                GAgent_SetCloudServerStatus( pgc, MQTT_STATUS_START );
            }
            else
            {
                GAgent_SetCloudConfigStatus( pgc,CLOUD_INIT );
            }
        }
        pgc->rtinfo.waninfo.wanclient_num=0;
        pgc->ls.tcpClientNums=0;
        gagentWiFiStatus = ( (pgc->rtinfo.GAgentStatus)&(LOCAL_GAGENTSTATUS_MASK) ) ;

    }
    if( (gagentWiFiStatus&WIFI_MODE_ONBOARDING) != ( newStatus&WIFI_MODE_ONBOARDING) )
    {

        if( newStatus&WIFI_MODE_ONBOARDING )
        {
            uint16 tempWiFiStatus=0;
            //WIFI_MODE_ONBOARDING UP
            pgc->gc.flag |= XPG_CFG_FLAG_CONFIG_AP;
            GAgent_DevSaveConfigData( &(pgc->gc) );
            GAgent_Printf( GAGENT_INFO,"WIFI_MODE_ONBOARDING UP." );

            if( (newStatus&WIFI_STATION_CONNECTED) == WIFI_STATION_CONNECTED
                   || (newStatus&WIFI_MODE_STATION) == WIFI_MODE_STATION
                )
            {
                tempWiFiStatus = GAgent_DRVWiFi_StationDisconnect();
            }

            if( 0 == pgContextData->rtinfo.isAirlinkConfig )//not airlink config
            {
                if( !(newStatus&WIFI_MODE_AP) )
                {
                    os_memset( pgc->gc.wifi_ssid,0,SSID_LEN_MAX+1 );
                    os_memset( pgc->gc.wifi_key,0,WIFIKEY_LEN_MAX+1 );
                    GAgent_DevSaveConfigData( &(pgc->gc));
                    GAgent_DevGetConfigData( &(pgc->gc));
                    tempWiFiStatus |= GAgent_DRV_WiFi_SoftAPModeStart( pgc->minfo.ap_name,AP_PASSWORD,tempWiFiStatus );
                }
            }
            GAgent_SetWiFiStatus( pgc,WIFI_MODE_ONBOARDING,1 );
            gagentOnboardingTime = GAGENT_ONBOARDING_TIME;
        }
        else
        {
            //WIFI_MODE_ONBOARDING DOWN
            pgc->gc.flag &=~ XPG_CFG_FLAG_CONFIG_AP;
            GAgent_DevSaveConfigData( &(pgc->gc) );
            GAgent_Printf( GAGENT_INFO,"WIFI_MODE_ONBOARDING DOWN." );
            if( gagentOnboardingTime <= 0 )
            {
                GAgent_Printf( GAGENT_INFO,"WIFI_MODE_ONBOARDING Time out ...");
                GAgent_EnterNullMode( pgc );
            }
            else
            {
              /* ?ڹ涨ʱ???ڽ??յ????ð? */
              GAgent_Printf( GAGENT_INFO,"Receive OnBoarding data.");
              pgc->ls.onboardingBroadCastTime = SEND_UDP_DATA_TIMES;
              pgc->gc.flag |=XPG_CFG_FLAG_CONFIG;
              GAgent_DevSaveConfigData( &(pgc->gc) );
              GAgent_WiFiInit( pgc );
            }
            GAgent_SetWiFiStatus( pgc,WIFI_MODE_ONBOARDING,0 );
            gagentOnboardingTime = 0;

        }
        if( pgc->rtinfo.waninfo.CloudStatus == CLOUD_CONFIG_OK )
        {
            GAgent_SetCloudServerStatus( pgc, MQTT_STATUS_START );
        }
        else
        {
            GAgent_SetCloudConfigStatus( pgc,CLOUD_INIT );
        }
        pgc->rtinfo.waninfo.wanclient_num=0;
        pgc->ls.tcpClientNums=0;
        newStatus = GAgent_DevCheckWifiStatus( WIFI_CLOUD_CONNECTED,0 );
        gagentWiFiStatus = ( (pgc->rtinfo.GAgentStatus)&(LOCAL_GAGENTSTATUS_MASK) ) ;

    }
    if( (gagentWiFiStatus&WIFI_STATION_CONNECTED) != ( newStatus & WIFI_STATION_CONNECTED) )
    {
        if( newStatus&WIFI_STATION_CONNECTED )
        {
            GAgent_Printf( GAGENT_INFO," WIFI_STATION_CONNECTED UP" );
            /* ?????????????ƶ?ʱ?? */
            pgc->rtinfo.waninfo.ReConnectMqttTime = GAGENT_MQTT_TIMEOUT;
            pgc->rtinfo.waninfo.ReConnectHttpTime = GAGENT_HTTP_TIMEOUT;
            if( !(newStatus&WIFI_MODE_ONBOARDING) )
            {
                if( (newStatus&WIFI_MODE_AP)==WIFI_MODE_AP )
                {
                    GAgent_DRVWiFi_APModeStop( pgc );
                }
            }
            //WIFI_STATION_CONNECTED UP
            GAgent_DRVWiFiPowerScan( pgc );
            GAgent_SetWiFiStatus( pgc,WIFI_STATION_CONNECTED,1 );
            pgc->rtinfo.wifiLastScanTime = GAGENT_STA_SCANTIME;
        }
        else
        {
            //WIFI_STATION_CONNECTED DOWN
            GAgent_Printf( GAGENT_INFO," WIFI_STATION_CONNECTED Down" );
            GAgent_SetWiFiStatus( pgc,WIFI_STATION_CONNECTED,0 );
            GAgent_SetWiFiStatus( pgc,WIFI_CLIENT_ON,0 );
            GAgent_SetWiFiStatus( pgc,WIFI_CLOUD_CONNECTED,0 );
            GAgent_SetCloudServerStatus( pgc,MQTT_STATUS_START );

            newStatus = GAgent_DevCheckWifiStatus( WIFI_CLOUD_CONNECTED,0 );
        }
        pgc->rtinfo.waninfo.wanclient_num=0;
        pgc->ls.tcpClientNums=0;
        gagentWiFiStatus = ( (pgc->rtinfo.GAgentStatus)&(LOCAL_GAGENTSTATUS_MASK) ) ;
    }
    if( (gagentWiFiStatus&WIFI_CLOUD_CONNECTED) != ( newStatus & WIFI_CLOUD_CONNECTED) )
    {
        if( newStatus&WIFI_CLOUD_CONNECTED )
        {
            //WIFI_CLOUD_CONNECTED UP
            GAgent_Printf( GAGENT_INFO," WIFI_CLOUD_CONNECTED Up." );
            GAgent_SetWiFiStatus( pgc,WIFI_CLOUD_CONNECTED,1 );
        }
        else
        {
            //WIFI_CLOUD_CONNECTED DOWN
            GAgent_Printf( GAGENT_INFO," WIFI_CLOUD_CONNECTED Down." );
            pgc->rtinfo.waninfo.wanclient_num=0;
            GAgent_SetCloudServerStatus( pgc,MQTT_STATUS_START );
            GAgent_SetWiFiStatus( pgc,WIFI_CLOUD_CONNECTED,0 );

        }
        gagentWiFiStatus = ( (pgc->rtinfo.GAgentStatus)&(LOCAL_GAGENTSTATUS_MASK) ) ;
    }
    if( gagentWiFiStatus&WIFI_MODE_TEST )//test mode
    {
        static int8 cnt=0;
        int8 ret =0;
        NetHostList_str *aplist=NULL;

        if( 0 == pgc->rtinfo.scanWifiFlag )
        {
            pgc->rtinfo.testLastTimeStamp += dTime_s;

            if( cnt >= 18 )
            {
                cnt = 0;
                GAgent_SetWiFiStatus( pgc,WIFI_MODE_TEST,0 );
                GAgent_DRVWiFiStopScan( );
                GAgent_EnterNullMode( pgc );
                GAgent_Printf( GAGENT_INFO,"Exit Test Mode...");
            }

            if( pgc->rtinfo.testLastTimeStamp >= 10 )
            {
                cnt++;
                pgc->rtinfo.testLastTimeStamp = 0;
                GAgent_DRVWiFiStartScan();
                GAgent_Printf( GAGENT_INFO,"IN TEST MODE...");
            }
            aplist = GAgentDRVWiFiScanResult( aplist );
            if( NULL==aplist )
            {
                ret = 0;
            }
            else
            {
                if( aplist->ApNum <= 0 )
                {
                    ret = 0;
                }
                else
                {
                    ret = GAgentFindTestApHost( aplist );
                }
            }
        }
        if( ret>0 )
        {
             uint16 tempWiFiStatus=0;
             pgc->rtinfo.scanWifiFlag = 1;
             cnt=0;
             GAgent_DRVWiFiStopScan( );
             if( 1==ret )
             {
                GAgent_Printf( GAGENT_INFO,"Connect to TEST AP:%s",GAGENT_TEST_AP1 );
                os_strcpy( pgc->gc.wifi_ssid,GAGENT_TEST_AP1 );
                os_strcpy( pgc->gc.wifi_key,GAGENT_TEST_AP_PASS );
                GAgent_DevSaveConfigData( &(pgc->gc) );
                tempWiFiStatus |= GAgent_DRVWiFi_StationCustomModeStart( GAGENT_TEST_AP1,GAGENT_TEST_AP_PASS,pgc->rtinfo.GAgentStatus );
             }
             else
             {
                GAgent_Printf( GAGENT_INFO,"Connect to TEST AP:%s",GAGENT_TEST_AP2 );
                os_strcpy( pgc->gc.wifi_ssid,GAGENT_TEST_AP2 );
                os_strcpy( pgc->gc.wifi_key,GAGENT_TEST_AP_PASS );
                GAgent_DevSaveConfigData( &(pgc->gc) );
                tempWiFiStatus |= GAgent_DRVWiFi_StationCustomModeStart( GAGENT_TEST_AP2,GAGENT_TEST_AP_PASS,pgc->rtinfo.GAgentStatus );
             }
        }
        gagentWiFiStatus = ( (pgc->rtinfo.GAgentStatus)&(LOCAL_GAGENTSTATUS_MASK) ) ;
    }
    pgc->rtinfo.wifiLastScanTime+=dTime_s;
    if( gagentWiFiStatus&WIFI_STATION_CONNECTED )
    {
        static int8 tempwifiRSSI=0;
        int8 wifiRSSI=0;
        uint16 wifiLevel=0;
        if( pgc->rtinfo.wifiLastScanTime >= GAGENT_STA_SCANTIME )
        {
            pgc->rtinfo.wifiLastScanTime=0;
            wifiRSSI = GAgent_DRVWiFiPowerScan( pgc );
            GAgent_Printf( GAGENT_INFO,"start to scan wifi power...");
            GAgent_Printf( GAGENT_INFO,"wifiRSSI=%d",wifiRSSI);
            if( abs( wifiRSSI-tempwifiRSSI )>=10 )
            {
                tempwifiRSSI = wifiRSSI;
                wifiLevel = GAgent_GetStaWiFiLevel( wifiRSSI );
                gagentWiFiStatus =gagentWiFiStatus|(wifiLevel<<8);
                pgc->rtinfo.GAgentStatus = gagentWiFiStatus;
                GAgent_Printf( GAGENT_INFO,"SSID power:%d level:%d wifistatus:%04x",wifiRSSI,wifiLevel,gagentWiFiStatus );
            }
            gagentWiFiStatus = ( (pgc->rtinfo.GAgentStatus)&(LOCAL_GAGENTSTATUS_MASK) ) ;
        }
    }
    if( gagentWiFiStatus&WIFI_MODE_AP )
    {
        NetHostList_str *pAplist=NULL;
        int32 i = 0;
        if( pgc->rtinfo.wifiLastScanTime >=GAGENT_AP_SCANTIME )
        {
            pgc->rtinfo.wifiLastScanTime = 0;
            GAgent_DRVWiFiStartScan( );
        }

        pAplist = GAgentDRVWiFiScanResult( pAplist );
        if( NULL == pAplist )
        {
           GAgent_Printf( GAGENT_WARNING,"pAplist is NULL!");
        }
        else
        {
            if( (pgc->rtinfo.aplist.ApList)!=NULL )
            {
                GAgent_Printf( GAGENT_INFO,"free xpg aplist...");
                os_free( (pgc->rtinfo.aplist.ApList) );
                pgc->rtinfo.aplist.ApList=NULL;

            }
            if( pAplist->ApNum>0 )
            {
                pgc->rtinfo.aplist.ApNum = pAplist->ApNum;
                (pgc->rtinfo.aplist.ApList) = (ApHostList_str *)os_malloc( (pAplist->ApNum)*sizeof(ApHostList_str) );
                if( (pgc->rtinfo.aplist.ApList)!=NULL )
                {
                    for( i=0;i<pAplist->ApNum;i++ )
                    {
                        os_strcpy( (char *)pgc->rtinfo.aplist.ApList[i].ssid, (char *)pAplist->ApList[i].ssid);
                        pgc->rtinfo.aplist.ApList[i].ApPower = pAplist->ApList[i].ApPower;
//                            GAgent_Printf( GAGENT_CRITICAL,"AP Scan SSID = %s power = %d",
//                                                                  pgc->rtinfo.aplist.ApList[i].ssid,
//                                                                    pgc->rtinfo.aplist.ApList[i].ApPower );
                    }
                }
            }
        }
        gagentWiFiStatus = ( (pgc->rtinfo.GAgentStatus)&(LOCAL_GAGENTSTATUS_MASK) ) ;
    }
    if( (gagentWiFiStatus&WIFI_MODE_ONBOARDING)==WIFI_MODE_ONBOARDING )
    {
        if( gagentOnboardingTime>0 )
        {
            gagentOnboardingTime--;
        }
        else
        {
            GAgent_DevCheckWifiStatus( WIFI_MODE_ONBOARDING,0 );
            GAgent_Printf( GAGENT_DEBUG,"file:%s function:%s line:%d ",__FILE__,__FUNCTION__,__LINE__ );
        }
        gagentWiFiStatus = ( (pgc->rtinfo.GAgentStatus)&(LOCAL_GAGENTSTATUS_MASK) ) ;
    }
    GAgent_LocalSendGAgentstatus(pgc,dTime_s);
    return ;
}

