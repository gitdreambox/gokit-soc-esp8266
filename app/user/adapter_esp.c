#include "gagent.h"
#include "platform.h"
#include "iof_arch.h"
#include "http.h"
#include "cloud.h"
#include "gagent_typedef.h"
#include "netevent.h"
#define MACSTR_1 "%02x%02x%02x%02x%02x%02x"
#define GAGENT_CONFIG_FILE "./config/gagent_config.config"
struct espconn http_user_tcp_conn;
struct _esp_tcp http_user_tcp;
struct espconn M2M_user_tcp_conn;
struct _esp_tcp M2M_user_tcp;
struct scan_config *config;
static NetHostList_str gAplist;
struct upgrade_server_info server;
os_timer_t m2m_recon_timer;
os_timer_t http_recon_timer;

os_event_t erase_sector_queue[240];
typedef void (*erase_big_sector_cb)(void*arg);
erase_big_sector_cb     EB;

void ICACHE_FLASH_ATTR
msleep()
{
    int i;
    for( i = 0; i < 20; i++)
        os_delay_us(50000);
}
#if 0
int32 ICACHE_FLASH_ATTR
GAgent_DevGetConfigData( gconfig *pConfig )
{
    return system_param_load( 0x4b, 0, pConfig, sizeof(gconfig) );
}
int32 ICACHE_FLASH_ATTR
GAgent_DevSaveConfigData( gconfig *pConfig )
{
    return system_param_save_with_protect( 0x4b, (void *)pConfig, sizeof(gconfig) );
}
#endif

int32 ICACHE_FLASH_ATTR
GAgent_DevGetConfigData( gconfig *pConfig )
{
    int32 ret;
    ret = spi_flash_read(0x204000, pConfig, sizeof(gconfig));
    if(0 != ret)
    {
        GAgent_Printf(GAGENT_WARNING,"spi_flash_read failed,err=%d\n",ret);
    }

    return ret;
}
int32 ICACHE_FLASH_ATTR
GAgent_DevSaveConfigData( gconfig *pConfig )
{
    int32 ret;
    ret = spi_flash_erase_sector(516);
    if(0 != ret)
    {
        GAgent_Printf(GAGENT_WARNING,"spi_flash_erase_sector failed,err=%d\n",ret);

    }

    ret = spi_flash_write(0x204000, pConfig, sizeof(gconfig));
    if(0 != ret)
    {
        GAgent_Printf(GAGENT_WARNING,"spi_flash_write failed,err=%d\n",ret);
    }
    return ret;
}

uint32 ICACHE_FLASH_ATTR
SocketUdp_sendto(uint32 fd,uint8* buf,uint32 len)
{
    return espconn_sent((struct espconn *)fd,buf,len);
}
void ICACHE_FLASH_ATTR
HttpServer_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    static int32 first_dns = 1;
    struct espconn *pespconn = (struct espconn *)arg;
    if (ipaddr == NULL)
    {
        pgContextData->rtinfo.waninfo.RefreshIPTime = 1;
        GAgent_Printf( GAGENT_WARNING,"http user_dns_found NULL \r\n");
    }
    else
    {
        if( pgContextData->gc.GServer_ip[0] != *((uint8 *)&ipaddr->addr) ||
            pgContextData->gc.GServer_ip[1] != *((uint8 *)&ipaddr->addr+1) ||
            pgContextData->gc.GServer_ip[2] != *((uint8 *)&ipaddr->addr+2) ||
            pgContextData->gc.GServer_ip[3] != *((uint8 *)&ipaddr->addr+3)
           )
        {
            GAgent_Printf( GAGENT_DEBUG,"Save GService ip into flash!");
            os_memcpy((pgContextData->gc.GServer_ip),(uint8 *)&ipaddr->addr,4);
            GAgent_DevSaveConfigData( &(pgContextData->gc) );
        }
        GAgent_Printf( GAGENT_INFO,"dns GServer_ip=%d.%d.%d.%d \n",pgContextData->gc.GServer_ip[0],
            pgContextData->gc.GServer_ip[1],pgContextData->gc.GServer_ip[2],pgContextData->gc.GServer_ip[3]);
    }
    system_os_post( 1, SIG_CLOUD_HTTP, NULL);

}
void ICACHE_FLASH_ATTR
M2MServer_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;
    if (ipaddr == NULL)
    {
        GAgent_Printf( GAGENT_WARNING,"m2m user_dns_found NULL \r\n");
        pgContextData->rtinfo.waninfo.RefreshIPTime = 1;
        return;
    }
    else
    {
        if( pgContextData->gc.m2m_ip[0] != *((uint8 *)&ipaddr->addr) ||
            pgContextData->gc.m2m_ip[1] != *((uint8 *)&ipaddr->addr+1) ||
            pgContextData->gc.m2m_ip[2] != *((uint8 *)&ipaddr->addr+2) ||
            pgContextData->gc.m2m_ip[3] != *((uint8 *)&ipaddr->addr+3)
           )
        {
            GAgent_Printf( GAGENT_DEBUG,"Save M2M ip into flash!");
            os_memcpy((pgContextData->gc.m2m_ip),(uint8 *)&ipaddr->addr,4);
            GAgent_DevSaveConfigData( &(pgContextData->gc) );
        }
        GAgent_Printf( GAGENT_INFO,"dns m2m_ip=%d.%d.%d.%d \n",pgContextData->gc.m2m_ip[0],
            pgContextData->gc.m2m_ip[1],pgContextData->gc.m2m_ip[2],pgContextData->gc.m2m_ip[3]);
        if( MQTT_STATUS_RUNNING != pgContextData->rtinfo.waninfo.mqttstatus )
        {
            GAgent_SetCloudServerStatus( pgContextData,MQTT_STATUS_START );
            pgContextData->rtinfo.m2mDnsflag = RET_SUCCESS;
            system_os_post( 1, SIG_CLOUD_M2M, NULL);
        }
    }
}

uint32 ICACHE_FLASH_ATTR
GAgent_GetHostByName( int8 *domain, int8 *IPAddress, int32 type)
{
    struct espconn user_tcp_conn;
    struct _esp_tcp user_tcp;
    user_tcp_conn.proto.tcp = &user_tcp;
    user_tcp_conn.type = ESPCONN_TCP;
    user_tcp_conn.state = ESPCONN_NONE;

    ip_addr_t tcp_server_ip;
    tcp_server_ip.addr = 0;
    if(1 == type)
    {
        GAgent_Printf( GAGENT_DEBUG,"HTTP espconn_gethostbyname\n");
        return espconn_gethostbyname(&user_tcp_conn,domain,&tcp_server_ip,HttpServer_dns_found);
    }
    else
    {
        GAgent_Printf( GAGENT_DEBUG,"M2M espconn_gethostbyname\n");
        return espconn_gethostbyname(&user_tcp_conn,domain,&tcp_server_ip,M2MServer_dns_found);
    }
}

uint32 ICACHE_FLASH_ATTR
GAgent_GetDevTime_MS()
{
    return (system_get_time() / 1000);
}
uint32 ICACHE_FLASH_ATTR
GAgent_GetDevTime_S()
{
     return (system_get_time() / 1000 / 1000);
}
/****************************************************************
FunctionName    :   GAgent_DevReset
Description     :   dev exit but not clean the config data
pgc             :   global staruc
return          :   NULL
Add by Alex.lin     --2015-04-18
****************************************************************/
void ICACHE_FLASH_ATTR
GAgent_DevReset()
{
    GAgent_Printf( GAGENT_CRITICAL,"Please restart GAgent !!!\r\n");
    system_restart();
}
void ICACHE_FLASH_ATTR
GAgent_DevInit( pgcontext pgc )
{
    wifi_set_event_handler_cb( wifi_handle_event_cb );
}
int8 ICACHE_FLASH_ATTR
GAgent_DevGetMacAddress( uint8* szmac )
{
    uint8 mac[6]={0};
    if(1 == wifi_get_opmode())
        wifi_get_macaddr( STATION_IF, mac );
    else
        wifi_get_macaddr( SOFTAP_IF, mac );
    os_sprintf((char *)szmac,"%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return 0;
}
void ICACHE_FLASH_ATTR
GAgent_GetFlashPara(uint8 filetype, uint32* pStartaddr)
{
    switch(filetype)
    {
        case 1://wifi master firmware
            *pStartaddr = 0x4096;
            break;
        case 2://wifi slave firmware
            *pStartaddr = 0x101000;
            break;
        case 3://mcu firmware
            *pStartaddr = 0x209000;
//            GAgent_GetFwHeadInfo(pgContextData,&(pgContextData->rtinfo.firmwareInfo));
//            *pStartaddr  += pgContextData->rtinfo.firmwareInfo.file_offset;
            break;
        default:
            break;
    }
}
void ICACHE_FLASH_ATTR
GAgent_SetFlashPara(uint8 filetype, uint32* pStartaddr,
                        uint32* pTotallen,uint32* pSerialnum)
{
    switch(filetype)
    {
        case OTA_TYPE_WIFI_MASTER://wifi master firmware
            *pStartaddr = 0x209000;
            *pTotallen = pgContextData->gc.wifiFirmwareLen;
            *pSerialnum = 521;
//            *pSerialnum = 520;
//            GAgent_GetFwHeadInfo(pgContextData,&(pgContextData->rtinfo.firmwareInfo));
//            *pStartaddr  += pgContextData->rtinfo.firmwareInfo.file_offset;
            break;
        case OTA_TYPE_WIFI_SLAVE://wifi slave firmware
            *pStartaddr = 0x101000;
            *pTotallen = pgContextData->gc.wifiFirmwareLen;
            *pSerialnum = 257;
            break;
        case OTA_TYPE_MCU://mcu firmware
            *pStartaddr = 0x209000;
            *pTotallen = pgContextData->mcu.firmwareLen;
            *pSerialnum = 521;
//            *pSerialnum = 520;
//            GAgent_GetFwHeadInfo(pgContextData,&(pgContextData->rtinfo.firmwareInfo));
//            *pStartaddr  += pgContextData->rtinfo.firmwareInfo.file_offset;
            break;
        default:
            break;

    }
}
uint32 ICACHE_FLASH_ATTR
GAgent_SaveFile( int32 offset,uint8 *buf,uint32 len,uint8 filetype )
{
    int32 ret = 0;
    uint32 tmpbuf[1440];
    uint32 write_offset = 0;
    uint32 start_addr = 0;
    uint32 total_len = 0;
    uint32 serial_num = 0;
    static uint32 file_offset = 0;
    static uint8 remainbuf[3] = {0};
    static uint32 remainlen = 0;
    static uint32 lastremainlen = 0;
    static uint32 erasecount = 0;
    uint8 piece_offset =0;
    os_memset(tmpbuf, 0, sizeof(tmpbuf));

    if( 0 == offset )
    {
        file_offset = 0;
        remainlen = 0;
        lastremainlen = 0;
        erasecount = 0;
    }
        
    GAgent_SetFlashPara(filetype, &start_addr, &total_len, &serial_num);

    if( file_offset+len >= 4096*erasecount )
    {
        ret = spi_flash_erase_sector(serial_num+erasecount);
        if(ret != SPI_FLASH_RESULT_OK)
        {
            GAgent_Printf(GAGENT_WARNING,"erase sector err=%d\n",ret);
        }
        erasecount++;
        ret = spi_flash_erase_sector(serial_num+erasecount);
        if(ret != SPI_FLASH_RESULT_OK)
        {
            GAgent_Printf(GAGENT_WARNING,"erase sector err=%d\n",ret);
        }
        erasecount++;
    }

    if( (remainlen+len)%4 == 0 )
    {
        write_offset = remainlen + len;
        os_memcpy((uint8 *)tmpbuf, remainbuf, remainlen);
        os_memcpy((uint8 *)tmpbuf+remainlen, buf, len);
        ret = spi_flash_write(start_addr+file_offset, tmpbuf, write_offset);
        file_offset += write_offset;
        if( file_offset + remainlen + len == total_len )
        {
            file_offset = 0;
            memset(remainbuf, 0, sizeof(remainbuf));
            remainlen = 0;
            lastremainlen = 0;
            erasecount = 0;
        }
    }
    else//can not divide exactlly by 4
    {
        if( file_offset + remainlen + len == total_len )//last paket
        {
            write_offset = remainlen + len;
            os_memcpy((uint8 *)tmpbuf, remainbuf, remainlen);
            os_memcpy((uint8 *)tmpbuf+remainlen, buf, len);
            ret = spi_flash_write(start_addr+file_offset, tmpbuf, write_offset);
            file_offset += write_offset;

            file_offset = 0;
            memset(remainbuf, 0, sizeof(remainbuf));
            remainlen = 0;
            lastremainlen = 0;
            erasecount = 0;
        }
        else
        {
            write_offset = (remainlen + len) - (remainlen + len)%4;
            os_memcpy((uint8 *)tmpbuf, remainbuf, remainlen);
            os_memcpy((uint8 *)tmpbuf+remainlen, buf, len);
            ret = spi_flash_write(start_addr+file_offset, tmpbuf, write_offset);
            file_offset += write_offset;
            lastremainlen = remainlen;
            remainlen = (remainlen + len)%4;
            os_memset(remainbuf, 0, sizeof(remainbuf));
            os_memcpy(remainbuf, buf+write_offset-lastremainlen, remainlen);
         }
    }
    return ret;
}
int32 ICACHE_FLASH_ATTR
GAgent_ReadFile( uint32 offset,int8* buf,uint32 len,uint8 filetype )
{
    int32 ret;
    uint32 tmpbuf[256];
    uint32 start_addr = 0;
    os_memset(tmpbuf, 0, sizeof(tmpbuf));
    GAgent_GetFlashPara(filetype, &start_addr);
    ret = spi_flash_read(start_addr+offset, tmpbuf, len);
    if( ret != 0 )
    {
        GAgent_Printf( GAGENT_WARNING,"spi_flash_read failed,error num = %d\n",ret);
        return RET_FAILED;
    }
    os_memcpy(buf, tmpbuf, len);
    return len;
}
uint32 ICACHE_FLASH_ATTR
GAgent_SaveUpgradFirmware( int offset,uint8 *buf,int len,uint8 filetype )
{
    return GAgent_SaveFile( offset, buf, len, filetype );
}

void ICACHE_FLASH_ATTR
WifiStatusHandler(int event)
{

}
/****************************************************************
FunctionName    :   GAgent_DRVBootConfigWiFiMode.
Description     :   ??è?é?μ??°éè??μ?WiFi??DD?￡ê?,′?oˉêy??Dèòaé?μ?Dèòa
                    è·?¨??DD?￡ê?μ??￡?éóDó??￡??óú?éò?èè?D???￡ê?μ???ì¨￡?
                    ′?′|·μ??0.
return          :   1-boot?¤?èéè???aSTA??DD?￡ê?
                    2-boot?¤?èéè???aAP??DD?￡ê?
                    3-boot?¤?èéè???aSTA+AP?￡ê???DD
                    >3 ±￡á??μ?￡
Add by Alex.lin     --2015-06-01
****************************************************************/
int8 ICACHE_FLASH_ATTR
GAgent_DRVBootConfigWiFiMode( void )
{
    static int8 flag=0;
    if( flag>=1 && RET_SUCCESS==pgContextData->rtinfo.getInfoflag
        && RET_FAILED==pgContextData->rtinfo.webconfigflag )
    {
        GAgent_Printf( GAGENT_INFO,"%s ReStart.",__FUNCTION__);
        //GAgent_DevReset( );
    }
    flag++;
    return wifi_get_opmode();
}
/****************************************************************
FunctionName    :   GAgent_DRVGetWiFiMode.
Description     :   í¨1y?D??pgc->gc.flag  |= XPG_CFG_FLAG_CONNECTED￡?
                    ê?·?????à′?D??GAgentê?·?òa??ó?STA ?ò AP?￡ê?.
pgc             :   è???±?á?.
return          :   1-boot?¤?èéè???aSTA??DD?￡ê?
                    2-boot?¤?èéè???aAP??DD?￡ê?
                    3-boot?¤?èéè???aSTA+AP?￡ê???DD
                    >3 ±￡á??μ?￡
Add by Alex.lin     --2015-06-01
****************************************************************/
int8 ICACHE_FLASH_ATTR
GAgent_DRVGetWiFiMode( pgcontext pgc )
{
    int8 ret =0;
    int8 ssidlen=0,keylen=0;
    ssidlen = os_strlen( pgc->gc.wifi_ssid );
    keylen = os_strlen( pgc->gc.wifi_key );
    GAgent_Printf( GAGENT_INFO,"SSIDLEN=%d, keyLEN=%d ",ssidlen,keylen );
    if( (ssidlen>0 && ssidlen<=SSID_LEN_MAX) && keylen<=WIFIKEY_LEN_MAX )
    {
        //GAgent_Printf( GAGENT_INFO,"start STA!!!\r\n");
        pgc->gc.flag |= XPG_CFG_FLAG_CONNECTED;
        ret = 1;
    }
    else
    {
        //GAgent_Printf( GAGENT_INFO,"start AP!!!\r\n");
        os_memset( pgc->gc.wifi_ssid,0,SSID_LEN_MAX+1 );
        os_memset( pgc->gc.wifi_key,0,WIFIKEY_LEN_MAX+1 );
        pgc->gc.flag &=~ XPG_CFG_FLAG_CONNECTED;
        ret = 2;
    }
    GAgent_DevSaveConfigData( &(pgc->gc));
    GAgent_DevGetConfigData( &(pgc->gc));
    return ret;
}
//return the new wifimode
int8 ICACHE_FLASH_ATTR
GAgent_DRVSetWiFiStartMode( pgcontext pgc,uint32 mode )
{
    return ( pgc->gc.flag +=mode );
}
void ICACHE_FLASH_ATTR
DRV_ConAuxPrint( char *buffer, int len, int level )
{
    buffer[len]='\0';
    os_printf("%s", buffer);
}

void ICACHE_FLASH_ATTR
GAgent_LocalDataIOInit( pgcontext pgc ,int32 BaudRate )
{
    UART_SetBaudrate( 0, BaudRate);
}
void ICACHE_FLASH_ATTR
softap_init(void)
{
    int ret1,ret2;
    struct softap_config soft_ap;

    ret2 = wifi_get_opmode();
    if( ret2 != STATIONAP_MODE )
    {
        wifi_set_opmode(STATIONAP_MODE);
    }
    wifi_softap_get_config_default(&soft_ap);

    os_memset(soft_ap.ssid, 0, 32);
    os_memset(soft_ap.password, 0, 64);
    os_memcpy(soft_ap.ssid, pgContextData->minfo.ap_name, os_strlen(pgContextData->minfo.ap_name));
    os_memcpy(soft_ap.password, "123456789", 9);
    GAgent_Printf(GAGENT_INFO,"soft_ap.ssid %s\n",soft_ap.ssid);

    soft_ap.ssid_len=os_strlen(soft_ap.ssid);

    soft_ap.authmode = AUTH_WPA_WPA2_PSK;
    soft_ap.ssid_len = 0;// or its actual length
    soft_ap.max_connection = 4; // how many stations can connect to ESP8266 softAP at most.

    ret2 = wifi_softap_set_config(&soft_ap);
}

int16 ICACHE_FLASH_ATTR
GAgent_DRV_WiFi_SoftAPModeStart( const int8* ap_name,const int8 *ap_password,int16 wifiStatus )
{
    int ret;
    struct ip_info info;
    wifi_station_set_auto_connect(0);
    wifi_station_disconnect();
    ret = wifi_set_opmode(STATIONAP_MODE);
    if( FALSE == ret )
    {
       GAgent_Printf( GAGENT_WARNING,"wifi set opmode STATIONAP_MODE failed!\n");
    }
    else
    {
        softap_init();
        GAgent_DevCheckWifiStatus( WIFI_MODE_AP,1 );
        GAgent_DevCheckWifiStatus( WIFI_MODE_ONBOARDING,1 );
        GAgent_DevCheckWifiStatus( WIFI_MODE_STATION,0 );
    }
    wifi_softap_dhcps_stop();
    IP4_ADDR(&info.ip, 10, 10, 100, 254);
    IP4_ADDR(&info.gw, 10, 10, 100, 254);
    IP4_ADDR(&info.netmask, 255, 255, 255, 0);
    wifi_set_ip_info(SOFTAP_IF, &info);
    wifi_softap_dhcps_start();
    GAgent_DoTcpWebConfig( pgContextData);
    GAgent_DevCheckWifiStatus( WIFI_STATION_CONNECTED,0 );
    return WIFI_MODE_AP;
}
int16 ICACHE_FLASH_ATTR
GAgent_DRVWiFi_StationCustomModeStart(int8 *StaSsid,int8 *StaPass,uint16 wifiStatus )
{
    struct station_config stationConf;

    //need not mac address
    stationConf.bssid_set = 0;
    wifi_set_opmode(STATION_MODE);
    GAgent_DevCheckWifiStatus( WIFI_MODE_STATION,1 );
    GAgent_DevCheckWifiStatus( WIFI_MODE_AP,0 );
    GAgent_DevGetMacAddress(pgContextData->minfo.szmac);
    os_memcpy(&stationConf.ssid, StaSsid, 32);
    os_memcpy(&stationConf.password, StaPass, 64);
    wifi_station_set_config(&stationConf);
    //wifi_station_disconnect();
    if( false == wifi_station_connect())
    {
        GAgent_Printf( GAGENT_WARNING,"connect AP failed!\n");
    }
    GAgent_Printf( GAGENT_INFO," Station ssid:%s StaPass:%s",StaSsid,StaPass );
    return WIFI_STATION_CONNECTED;
}
int16 ICACHE_FLASH_ATTR
GAgent_DRVWiFi_StationDisconnect()
{
    wifi_station_disconnect();
    return 0;
}
void ICACHE_FLASH_ATTR
GAgent_DRVWiFi_APModeStop( pgcontext pgc )
{
/*  uint16 tempStatus=0;
    tempStatus = pgc->rtinfo.GAgentStatus;
    tempStatus = GAgent_DevCheckWifiStatus( tempStatus );*/
    GAgent_DevCheckWifiStatus( WIFI_MODE_AP,0 );
    return ;
}
int8 ICACHE_FLASH_ATTR
GAgent_DRVWiFiPowerScan( pgcontext pgc )
{
    int32 rssi;
    rssi = wifi_station_get_rssi();
    if( 31 == rssi )
    {
        GAgent_Printf( GAGENT_WARNING,"scan wifi power failed!");
        return 0;
    }
    if( rssi+100 < 0 )
    {
        return 0;
    }
    if( rssi+100 >80)
    {
        return 80;
    }
    return (rssi +100);

}
int8 ICACHE_FLASH_ATTR
GAgent_DRVWiFiPowerScanResult( pgcontext pgc )
{
    int32 rssi;
    rssi = wifi_station_get_rssi();
    if( 31 == rssi )
    {
        GAgent_Printf( GAGENT_WARNING,"scan wifi power failed!");
        return 0;
    }
    if( rssi+100 < 0 )
    {
        return 0;
    }
    if( rssi+100 >80)
    {
        return 80;
    }
    return (rssi +100);
}
void ICACHE_FLASH_ATTR
GAgent_DevTick()
{

    //fflush(stdout);
}
void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
    switch(status)
    {
        case SC_STATUS_WAIT:
            os_printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            os_printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
            sc_type *type = pdata;
            if (*type == SC_TYPE_ESPTOUCH)
            {
                os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            }
            else
            {
                os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:
            os_printf("SC_STATUS_LINK\n");
            struct station_config *sta_conf = pdata;
            wifi_station_set_config(sta_conf);
            wifi_station_disconnect();
            wifi_station_connect();
            pgContextData->rtinfo.waninfo.RefreshIPTime = 0;
            os_timer_disarm(&(pgContextData->rtinfo.AirLinkTimer));
            os_memcpy(pgContextData->gc.wifi_ssid,sta_conf->ssid,32);
            os_memcpy(pgContextData->gc.wifi_key,sta_conf->password,64);
            GAgent_DevSaveConfigData( &(pgContextData->gc) );
            GAgent_Printf( GAGENT_CRITICAL,"AirLink result ssid:%s key:%s",pgContextData->gc.wifi_ssid,pgContextData->gc.wifi_key );
            pgContextData->gc.flag |= XPG_CFG_FLAG_CONNECTED;
            pgContextData->ls.onboardingBroadCastTime = SEND_UDP_DATA_TIMES;
            //GAgent_WiFiInit( pgContextData );
            CreateUDPBroadCastServer( pgContextData);
            GAgent_DevCheckWifiStatus( WIFI_MODE_ONBOARDING,0 );
            pgContextData->rtinfo.isAirlinkConfig = 0;
            break;
        case SC_STATUS_LINK_OVER:
            os_printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL)
            {
                uint8 phone_ip[4] = {0};
                os_memcpy(phone_ip, (uint8*)pdata, 4);
                os_printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
            }
            smartconfig_stop();
            break;
    }
}

void ICACHE_FLASH_ATTR
GAgent_OpenAirlink( int32 timeout_s )
{
    //TODO
    if(wifi_get_opmode() != 1)
        wifi_set_opmode(STATION_MODE);
    GAgent_DevCheckWifiStatus( WIFI_MODE_AP,0 );
    GAgent_DevCheckWifiStatus( WIFI_MODE_STATION,1 );
    smartconfig_start(smartconfig_done);
    return ;
}
void ICACHE_FLASH_ATTR
GAgent_AirlinkResult( pgcontext pgc )
{

    return ;
}
void ICACHE_FLASH_ATTR
scan_done_cb (void *arg, STATUS status)
{
     int i = 0;
     if (status == OK)
     {
        struct bss_info *bss_link = (struct bss_info *)arg;
        bss_link = bss_link->next.stqe_next; //ignore first

        gAplist.ApNum = 0;
        if( (gAplist.ApList)!=NULL )
        {
            GAgent_Printf(GAGENT_INFO,"Free buf...");
            os_free( gAplist.ApList );
        }
        while(bss_link != NULL)
        {
            bss_link = bss_link->next.stqe_next;
            gAplist.ApNum++;
        }
        GAgent_Printf( GAGENT_INFO,"ApNum = %d",gAplist.ApNum);
        gAplist.ApList = (ApHostList_str *)os_malloc( gAplist.ApNum*sizeof(ApHostList_str) );
        bss_link = (struct bss_info *)arg;
        for(i=0; i<gAplist.ApNum; i++)
        {
            bss_link = bss_link->next.stqe_next;
            os_memcpy(gAplist.ApList[i].ssid,bss_link->ssid,sizeof(bss_link->ssid));
            gAplist.ApList[i].ApPower = bss_link->rssi + 100;
        }
//        for( i=0;i<gAplist.ApNum;i++ )
//           GAgent_Printf( GAGENT_DEBUG,"SSID = %s power = %d %d", gAplist.ApList[i].ssid,gAplist.ApList[i].ApPower,bss_link->rssi );
     }
     else
     {
        GAgent_Printf( GAGENT_WARNING,"scan wifi list failed!\n");
        gAplist.ApNum == 0;
     }
}
void ICACHE_FLASH_ATTR
GAgent_DRVWiFiStartScan( )
{
    if(wifi_get_opmode() == SOFTAP_MODE)
    {
        GAgent_Printf( GAGENT_WARNING,"ap mode can't scan !!!\r\n");
        return;
    }
    config = NULL;
    wifi_station_scan (config, scan_done_cb);
}
void ICACHE_FLASH_ATTR
GAgent_DRVWiFiStopScan( )
{

}
NetHostList_str * ICACHE_FLASH_ATTR
GAgentDRVWiFiScanResult( NetHostList_str *aplist )
{
    if( gAplist.ApNum==0 )
        return NULL;
    aplist  = &(gAplist);
    return  aplist;
}

uint32 ICACHE_FLASH_ATTR
GAgent_ReadOTAFile( uint32 offset,int8* buf,uint32 len,uint8 filetype )
{
    return  GAgent_ReadFile( offset, buf, len, filetype );
}

int32 ICACHE_FLASH_ATTR
GAgent_WIFIOTAByUrl( pgcontext pgc,int8 *szdownloadUrl )
{
    int32 ret = 0;
    int32 http_socketid = -1;
    uint8 OTA_IP[32]={0};
    int8 *url = NULL;
    int8 *host = NULL;
    if( RET_FAILED == Http_GetHost( szdownloadUrl,&host,&url ) )
    {
        return RET_FAILED;
    }
    GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_WIFI_OTA );

    if(pgc->rtinfo.waninfo.http_socketid > 0)
    {
        ret = Http_ReqGetFirmware( url,host,http_socketid );

        if(RET_SUCCESS == ret)
        {
            GAgent_Printf(GAGENT_DEBUG,"Gagent Http_ReqGetFirmware success!\n");
        }
        else
        {
            GAgent_Printf(GAGENT_DEBUG,"Gagent Http_ReqGetFirmware failed!\n");
        }
    }
    else
    {
        //Cloud_InitSocket( pgc->rtinfo.waninfo.http_socketid,pgc->gc.GServer_ip,80,0 );
        //Http_ReqGetFirmware( url,host,http_socketid );
        GAgent_Printf(GAGENT_WARNING,"Http socket closed!");
    }

    return ret;
}

void ICACHE_FLASH_ATTR
wifi_handle_event_cb(System_Event_t *evt)
{
    os_printf("event %x\n", evt->event);
    switch (evt->event)
    {
        case EVENT_STAMODE_CONNECTED:
            os_printf("connect to ssid %s, channel %d\n",
            evt->event_info.connected.ssid,
            evt->event_info.connected.channel);
            break;
        case EVENT_STAMODE_DISCONNECTED:
            os_printf("disconnect from ssid %s, reason %d\n",
            evt->event_info.disconnected.ssid,
            evt->event_info.disconnected.reason);
            GAgent_DevCheckWifiStatus( WIFI_STATION_CONNECTED,0 );
            break;
        case EVENT_STAMODE_AUTHMODE_CHANGE:
            os_printf("mode: %d -> %d\n",
            evt->event_info.auth_change.old_mode,
            evt->event_info.auth_change.new_mode);
            break;
        case EVENT_STAMODE_GOT_IP:
            os_printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
            IP2STR(&evt->event_info.got_ip.ip),
            IP2STR(&evt->event_info.got_ip.mask),
            IP2STR(&evt->event_info.got_ip.gw));
            os_printf("\n");
            GAgent_DevCheckWifiStatus( WIFI_STATION_CONNECTED,1 );
            espconn_disconnect(pgContextData->ls.webTcpServer);
            pgContextData->ls.tcpWebConfigFd = INVALID_SOCKET;
            break;
        case EVENT_SOFTAPMODE_STACONNECTED:
            os_printf("station: " MACSTR "join, AID = %d\n",
            MAC2STR(evt->event_info.sta_connected.mac),
            evt->event_info.sta_connected.aid);
            break;
        case EVENT_SOFTAPMODE_STADISCONNECTED:
            os_printf("station: " MACSTR "leave, AID = %d\n",
            MAC2STR(evt->event_info.sta_disconnected.mac),
            evt->event_info.sta_disconnected.aid);
            break;
        default:
            break;
    }
}

void ICACHE_FLASH_ATTR
user_http_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
    //os_printf("------http recv !!! %d\n", length);
    int32 ret;
    pgContextData->rtinfo.waninfo.httpRecvFlag = 1;

    if(CLOUD_RES_WIFI_OTA == pgContextData->rtinfo.waninfo.CloudStatus)
    {
        pgContextData->rtinfo.lastRxOtaDataTime = GAgent_GetDevTime_S();
        ret = Http_ResGetFirmware( pgContextData,0,length,OTA_TYPE_WIFI_MASTER,pusrdata );
        if(RET_FAILED == ret)
        {
            GAgent_Printf( GAGENT_CRITICAL,"GAgent Download WIFI Firmware failed,go to check MCU OTA!\n");
            pgContextData->rtinfo.isOtaRunning = 0;
            if(0 == pgContextData->rtinfo.onlinePushflag)
            {
                pgContextData->rtinfo.OTATypeflag = OTATYPE_MCU;
                GAgent_SetCloudConfigStatus( pgContextData,CLOUD_RES_GET_SOFTVER );
                system_os_post( 1, SIG_CLOUD_HTTP, NULL);
            }
            else //online push
            {
                pgContextData->rtinfo.onlinePushflag = 0;
                GAgent_SetCloudConfigStatus ( pgContextData,CLOUD_CONFIG_OK );
            }
        }
    }
    else if(CLOUD_RES_MCU_OTA == pgContextData->rtinfo.waninfo.CloudStatus)
    {
        pgContextData->rtinfo.lastRxOtaDataTime = GAgent_GetDevTime_S();
        ret = Http_ResGetFirmware( pgContextData,0,length,OTA_TYPE_MCU,pusrdata );
        if(RET_FAILED == ret)
        {
            GAgent_Printf( GAGENT_CRITICAL,"GAgent Download MCU Firmware failed!\n");
            pgContextData->rtinfo.isOtaRunning = 0;
            if(0 == pgContextData->rtinfo.onlinePushflag)
            {
                if(1 == GAgent_IsNeedDisableDID(pgContextData))
                {
                    GAgent_Printf(GAGENT_INFO,"Need to Disable Device ID!");
                    GAgent_SetCloudConfigStatus( pgContextData,CLOUD_RES_DISABLE_DID );
                    if(pgContextData->rtinfo.waninfo.http_socketid > 0)
                    {
                        GAgent_Printf(GAGENT_WARNING,"Req to Disable Device ID!");
                        Cloud_ReqDisable( pgContextData );
                    }
                    else
                    {
                        Cloud_InitSocket( pgContextData->rtinfo.waninfo.http_socketid,pgContextData->gc.GServer_ip,80,0 );
                    }
                }
                else //do not need disable did
                {
                   GAgent_SetCloudConfigStatus ( pgContextData,CLOUD_CONFIG_OK );
                   system_os_post( 1, SIG_CLOUD_HTTP, NULL);
                }
            }
            else //online push
            {
                pgContextData->rtinfo.onlinePushflag = 0;
                GAgent_SetCloudConfigStatus ( pgContextData,CLOUD_CONFIG_OK );
            }
        }

    }
    else
    {
        if( length > 1440)
        {
            GAgent_Printf(GAGENT_ERROR,"Http recv length error,length = %d!\n", length);
            system_os_post( 1, SIG_CLOUD_M2M, NULL);
        }
        os_memcpy(pgContextData->rtinfo.HttpRxbuf->phead, pusrdata,length);
        pgContextData->rtinfo.waninfo.httpDataLen = length;
        system_os_post( 1, SIG_CLOUD_HTTP, NULL );
    }
}

void ICACHE_FLASH_ATTR
user_http_sent_cb(void *arg)
{
    GAgent_Printf( GAGENT_DEBUG,"http sent succeed !!!");
}
void ICACHE_FLASH_ATTR
user_http_discon_cb(void *arg)
{
    //tcp disconnect successfully
    pgContextData->rtinfo.waninfo.http_socketid = -1;
    GAgent_Printf( GAGENT_INFO,"http tcp disconnect succeed !!!");
}
void ICACHE_FLASH_ATTR
user_http_recon_cb(void *arg, sint8 err)
{
    //error occured , tcp connection broke. user can try to reconnect here.
    GAgent_Printf( GAGENT_WARNING,"http reconnect callback, error code %d !!!",err);
    pgContextData->rtinfo.waninfo.http_socketid = -1;
    os_timer_disarm(&http_recon_timer);
    os_timer_setfn(&http_recon_timer, (os_timer_func_t *)Http_ReconnectTimer, pgContextData);
    os_timer_arm(&http_recon_timer, pgContextData->rtinfo.waninfo.ReConnectMqttTime*1000, 1);
}
void ICACHE_FLASH_ATTR
user_http_connect_cb(void *arg)
{
    int32 ret;
    int8 *pDeviceID=NULL;
    pgconfig pConfigData=NULL;
    int32 cloudstatus = 0;
    uint16 GAgentStatus = 0;

    pgcontext pgc = pgContextData;
    pConfigData = &(pgc->gc);
    pDeviceID = pConfigData->DID;
    cloudstatus = pgc->rtinfo.waninfo.CloudStatus;
    GAgentStatus = pgc->rtinfo.GAgentStatus;

    struct espconn *pespconn = arg;
    pgContextData->rtinfo.waninfo.phttp_fd = pespconn;
    pgContextData->rtinfo.waninfo.ReConnectHttpTime = GAGENT_HTTP_TIMEOUT;
    pgContextData->rtinfo.waninfo.http_socketid = 1;
    GAgent_Printf( GAGENT_INFO,"http connect succeed !!!");

    espconn_regist_recvcb(pespconn, user_http_recv_cb);
    espconn_regist_sentcb(pespconn, user_http_sent_cb);
    espconn_regist_disconcb(pespconn, user_http_discon_cb);

    switch(cloudstatus)
    {
        case CLOUD_RES_GET_DID:
            ret = Cloud_ReqRegister( pgc );
            break;
        case CLOUD_RES_PROVISION:
            ret = Cloud_ReqProvision( pgc );
            break;
        case CLOUD_RES_GET_SOFTVER:
            ret = Cloud_ReqGetSoftver( pgc,pgc->rtinfo.OTATypeflag);
            break;
        case CLOUD_RES_DISABLE_DID:
            Cloud_ReqDisable( pgc );
            break;
        case CLOUD_REQ_GET_GSERVER_TIME:
            ret = GAgent_ReqServerTime(pgc);
            break;
        default:
            break;

    }

}

void ICACHE_FLASH_ATTR
user_m2m_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
    pgContextData->rtinfo.waninfo.m2mRecvFlag = 1;
    resetPacket(pgContextData->rtinfo.MqttRxbuf);
    os_memcpy(pgContextData->rtinfo.MqttRxbuf->phead, pusrdata,length);
    system_os_post( 1, SIG_CLOUD_M2M, NULL);
}
void ICACHE_FLASH_ATTR
user_m2m_sent_cb(void *arg)
{
    //data sent successfully
    GAgent_Printf( GAGENT_DEBUG,"m2m sent succeed !!!");
}
void ICACHE_FLASH_ATTR
user_m2m_discon_cb(void *arg)
{
    //tcp disconnect successfully
    GAgent_Printf( GAGENT_INFO,"m2m tcp disconnect succeed !!!");
    GAgent_DevCheckWifiStatus( WIFI_CLOUD_CONNECTED,0 );
    GAgent_SetCloudServerStatus( pgContextData, MQTT_STATUS_START );
    pgContextData->rtinfo.waninfo.m2m_socketid = INVALID_SOCKET;
    os_timer_disarm(&m2m_recon_timer);
    os_timer_setfn(&m2m_recon_timer, (os_timer_func_t *)M2M_ReconnectTimer, pgContextData);
    os_timer_arm(&m2m_recon_timer, pgContextData->rtinfo.waninfo.ReConnectMqttTime*1000, 1);
}
void ICACHE_FLASH_ATTR
user_m2m_recon_cb(void *arg, sint8 err)
{
    //error occured , tcp connection broke. user can try to reconnect here.
    GAgent_Printf( GAGENT_WARNING,"m2m reconnect callback, error code %d !!!",err);
    GAgent_DevCheckWifiStatus( WIFI_CLOUD_CONNECTED,0 );
    GAgent_SetCloudServerStatus( pgContextData, MQTT_STATUS_START );
    pgContextData->rtinfo.waninfo.m2m_socketid = INVALID_SOCKET;
    os_timer_disarm(&m2m_recon_timer);
    os_timer_setfn(&m2m_recon_timer, (os_timer_func_t *)M2M_ReconnectTimer, pgContextData);
    os_timer_arm(&m2m_recon_timer, pgContextData->rtinfo.waninfo.ReConnectMqttTime*1000, 1);
}
void ICACHE_FLASH_ATTR
user_m2m_connect_cb(void *arg)
{
    pgcontext pgc = pgContextData;
    uint32 dTime=0,ret=0,dataLen=0;
    int32 packetLen=0;
    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;
    int8 *username=NULL;
    int8 *password=NULL;
    uint8* pMqttBuf=NULL;
    uint16 mqttstatus=0;
    uint8 mqttpackType=0;

    pConfigData = &(pgc->gc);
    pGlobalVar = pgc;

    mqttstatus = pGlobalVar->rtinfo.waninfo.mqttstatus;
    username = pConfigData->DID;
    password = pConfigData->wifipasscode;

    struct espconn *pespconn = arg;
    pgContextData->rtinfo.waninfo.pm2m_fd = pespconn;

    GAgent_Printf( GAGENT_INFO,"m2m connect succeed !!!");
    pgContextData->rtinfo.waninfo.ReConnectMqttTime = GAGENT_MQTT_TIMEOUT;
    pgContextData->rtinfo.waninfo.m2m_socketid = 1;
    espconn_set_opt(pespconn, ESPCONN_COPY);
    espconn_regist_recvcb(pespconn, user_m2m_recv_cb);
    espconn_regist_sentcb(pespconn, user_m2m_sent_cb);
    espconn_regist_disconcb(pespconn, user_m2m_discon_cb);

    switch(mqttstatus)
    {
        case MQTT_STATUS_START:
            Cloud_ReqConnect( pgc,username,password);
            break;
        case MQTT_STATUS_RES_LOGIN:
            Cloud_ReqConnect( pgc,username,password);
            break;
    }

}
uint32 ICACHE_FLASH_ATTR
Http_ReconnectTimer( pgcontext pgc )
{
    if( pgc->rtinfo.GAgentStatus & WIFI_MODE_TEST )//test mode
    {
        os_timer_disarm(&http_recon_timer);
    }
    if( pgContextData->rtinfo.waninfo.http_socketid < 0 &&
        CLOUD_CONFIG_OK != pgc->rtinfo.waninfo.CloudStatus )
    {
        GAgent_Printf( GAGENT_WARNING,"Http_ReconnectTimer, Req to reconnect gserver !");
        Cloud_InitSocket( pgc->rtinfo.waninfo.http_socketid,pgc->gc.GServer_ip,80,0 );
        pgc->rtinfo.waninfo.send2HttpLastTime = GAgent_GetDevTime_S();
        pgc->rtinfo.waninfo.ReConnectHttpTime += GAGENT_CLOUDREADD_TIME;
        if((GAgent_GetDevTime_S()-pgc->rtinfo.waninfo.firstConnectHttpTime) >= 2 * ONE_HOUR)
        {
            GAgent_Printf(GAGENT_CRITICAL, "disconnect to http server over 2 h.reset!!!\r\n");
            GAgent_DevReset();
        }
        return 0;
    }
    else
    {
        os_timer_disarm(&http_recon_timer);
    }
}

int32 ICACHE_FLASH_ATTR
Cloud_InitSocket( int32 iSocketId,int8 *p_szServerIPAddr,int32 port,int8 flag )
{
    int32 ret=0;
    int32 tempSocketId=0;
    pgContextData->rtinfo.waninfo.httpRecvFlag = 0;

    http_user_tcp_conn.proto.tcp = &http_user_tcp;
    http_user_tcp_conn.type = ESPCONN_TCP;
    http_user_tcp_conn.state = ESPCONN_NONE;
    os_memcpy(http_user_tcp_conn.proto.tcp->remote_ip,pgContextData->gc.GServer_ip, 4); // remote ip of tcp server which get by dns
    http_user_tcp_conn.proto.tcp->remote_port = 80;
    http_user_tcp_conn.proto.tcp->local_port = espconn_port(); //local port of ESP8266

    espconn_regist_connectcb(&http_user_tcp_conn, user_http_connect_cb); // register connect callback
    espconn_regist_reconcb(&http_user_tcp_conn, user_http_recon_cb); // register reconnect callback as error handler
    ret = espconn_connect(&http_user_tcp_conn); // tcp connect
    if(ret != 0)
    {
        GAgent_Printf( GAGENT_WARNING,"http espconn_connect failed,error = %d\n", ret);
    }

    return iSocketId;
}

int32 ICACHE_FLASH_ATTR
Cloud_M2M_InitSocket( int32 iSocketId,int8 *p_szServerIPAddr,int32 port,int8 flag )
{
    int32 ret=0;
    int32 tempSocketId=0;
    pgContextData->rtinfo.waninfo.m2mRecvFlag = 0;

    M2M_user_tcp_conn.proto.tcp = &M2M_user_tcp;
    M2M_user_tcp_conn.type = ESPCONN_TCP;
    M2M_user_tcp_conn.state = ESPCONN_NONE;
    os_memcpy(M2M_user_tcp_conn.proto.tcp->remote_ip,pgContextData->gc.m2m_ip, 4); // remote ip of tcp server which get by dns
    M2M_user_tcp_conn.proto.tcp->remote_port = pgContextData->minfo.m2m_Port;
    M2M_user_tcp_conn.proto.tcp->local_port = espconn_port(); //local port of ESP8266

    espconn_regist_connectcb(&M2M_user_tcp_conn, user_m2m_connect_cb); // register connect callback
    espconn_regist_reconcb(&M2M_user_tcp_conn, user_m2m_recon_cb); // register reconnect callback as error handler
    espconn_connect(&M2M_user_tcp_conn); // tcp connect

    return iSocketId;
}
uint32 ICACHE_FLASH_ATTR
M2M_ReconnectTimer( pgcontext pgc )
{
    uint16 mqttstatus=0;
    mqttstatus = pgc->rtinfo.waninfo.mqttstatus;
    if( MQTT_STATUS_START == mqttstatus )
    {
        Cloud_M2M_InitSocket( pgc->rtinfo.waninfo.m2m_socketid ,pgc->gc.m2m_ip ,pgc->minfo.m2m_Port,0 );
        GAgent_SetCloudServerStatus( pgc,MQTT_STATUS_RES_LOGIN );
        GAgent_Printf(GAGENT_INFO,"M2M_ReconnectTimer !Req to reconnect M2M !\n");
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
        return 0;
    }
    else
    {
        os_timer_disarm(&m2m_recon_timer);
    }
}

void ICACHE_FLASH_ATTR
tx_buffer(uint8 *buf, uint16 len)
{
    uint16 i;
    for (i = 0; i < len; i++)
    {
        uart_tx_one_char(0,buf[i]);
    }
}
void ICACHE_FLASH_ATTR
AirLinkResult( pgcontext pgc )
{
    smartconfig_stop();
    GAgent_DevCheckWifiStatus( WIFI_MODE_ONBOARDING,1 );
    pgc->rtinfo.isAirlinkConfig = 0;
    GAgent_Printf( GAGENT_INFO,"AirLink Timeout ...");
    GAgent_Printf( GAGENT_INFO,"Into SoftAp Config...");
    os_memset( pgc->gc.wifi_ssid,0,SSID_LEN_MAX+1 );
    os_memset( pgc->gc.wifi_key,0,WIFIKEY_LEN_MAX+1 );
    GAgent_DevSaveConfigData( &(pgc->gc));
    GAgent_DevGetConfigData( &(pgc->gc));
    GAgent_DRV_WiFi_SoftAPModeStart( pgc->minfo.ap_name,AP_PASSWORD,0 );
}
void ICACHE_FLASH_ATTR
GAgent_Start_M2M_ReConnTimer()
{
    espconn_disconnect(pgContextData->rtinfo.waninfo.pm2m_fd);
    os_timer_disarm(&m2m_recon_timer);
    os_timer_setfn(&m2m_recon_timer, (os_timer_func_t *)M2M_ReconnectTimer, pgContextData);
    os_timer_arm(&m2m_recon_timer, pgContextData->rtinfo.waninfo.ReConnectMqttTime*1000, 1);

}
void ICACHE_FLASH_ATTR
GAgent_STANotConnAP()
{
     wifi_station_set_auto_connect(0);
     wifi_set_opmode(STATION_MODE);
     GAgent_DevCheckWifiStatus( WIFI_MODE_STATION,1 );
}
uint32 ICACHE_FLASH_ATTR
GAgent_getIpaddr(uint8 *ipaddr)
{
    int32 ret;
    uint8 strIpaddr[4];
    struct ip_info ipconfig;
    if( 0x01 == wifi_get_opmode())
    {
        ret = wifi_get_ip_info(STATION_IF, &ipconfig);
    }
    else
    {
        ret = wifi_get_ip_info(SOFTAP_IF, &ipconfig);
    }
    if( true == ret )
    {
        os_sprintf(ipaddr, IPSTR, IP2STR(&ipconfig.ip));
        GAgent_Printf(GAGENT_DEBUG,"ipaddr = %s\n",ipaddr);
        return RET_SUCCESS;
    }
    else
    {
        return RET_FAILED;
    }
}
uint32 ICACHE_FLASH_ATTR
GAgent_sendWifiInfo( pgcontext pgc )
{
    int32 pos = 0;
    uint8 ip[16] = {0};
    if(0 != GAgent_getIpaddr(ip))
    {
        GAgent_Printf(GAGENT_WARNING,"GAgent get ip failed!");
    }

    /* ModuleType */
    pgc->rtinfo.Txbuf->ppayload[0] = 0x01;
    pos += 1;

    /* MCU_PROTOCOLVER */
    memcpy( pgc->rtinfo.Txbuf->ppayload+pos, pgc->mcu.protocol_ver, MCU_PROTOCOLVER_LEN );
    pos += MCU_PROTOCOLVER_LEN;

    /* HARDVER */
    memcpy( pgc->rtinfo.Txbuf->ppayload+pos, WIFI_HARDVER, 8 );
    pos += 8;

    /* SOFTVAR */
    memcpy( pgc->rtinfo.Txbuf->ppayload+pos, WIFI_SOFTVAR, 8 );
    pos += 8;

    /* MAC */
    memset( pgc->rtinfo.Txbuf->ppayload+pos, 0 , 16 );
    memcpy( pgc->rtinfo.Txbuf->ppayload+pos, pgc->minfo.szmac, 16 );
    pos += 16;

    /* IP */
    memset( pgc->rtinfo.Txbuf->ppayload+pos, 0 , 16 );
    memcpy( pgc->rtinfo.Txbuf->ppayload+pos, ip, strlen(ip) );
    pos += 16;

    /* MCU_MCUATTR */
    memcpy( pgc->rtinfo.Txbuf->ppayload+pos, pgc->mcu.mcu_attr, MCU_MCUATTR_LEN);
    pos += MCU_MCUATTR_LEN;

    pgc->rtinfo.Txbuf->pend = pgc->rtinfo.Txbuf->ppayload + pos;
    return RET_SUCCESS;
}

/****************************************************************
        FunctionName  :  GAgent_sendmoduleinfo.
        Description      :  send module info,according to the actual situation to choose one .
        return             :  0 successful other fail.
****************************************************************/
uint32 ICACHE_FLASH_ATTR
GAgent_sendmoduleinfo( pgcontext pgc )
{
    return GAgent_sendWifiInfo(pgc);
}
void ICACHE_FLASH_ATTR GAgent_ChangeRunArea()
{
    system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
    system_upgrade_reboot();
}
int32 ICACHE_FLASH_ATTR
GAgent_copyFirmware( pgcontext pgc,uint32 src_addr,uint32 des_addr,uint32 totallen )
{
    static uint32 offset = 0;
    uint32 file_offset;
    uint32 serial_num = -1;
    int32 len = 1024;
    int32 ret;
    int32 erasecount = 0;
    uint32 tmpbuf[1024];
    int8 writeFlag = 0;

    if(0x1000 == des_addr)
    {
        serial_num = 1;
    }
    else
    {
        serial_num = 257;
    }
    system_soft_wdt_stop();
    //GAgent_DevGetConfigData( &(pgc->gc));
    GAgent_Printf(GAGENT_INFO,"GAgent had saved firmware length is %d\n",pgc->gc.wifiFirmwareLen);
    while(offset < pgc->gc.wifiFirmwareLen)
    {
        os_printf("offset = %d\n",offset);
        WRITE_PERI_REG(0X60000914,0X73);

        if(offset+len > pgc->gc.wifiFirmwareLen)
        {
            len = pgc->gc.wifiFirmwareLen - offset;
        }

        //read from flash
        memset(tmpbuf, 0 ,sizeof(tmpbuf));
        ret = spi_flash_read(src_addr+offset, tmpbuf, len);
        if(0 != ret)
        {
            GAgent_Printf( GAGENT_WARNING,"spi_flash_read failed,err=%d\n",ret);
            writeFlag = -1;
            break;
        }

        //erase flash
        if( offset >= 4096*erasecount )
        {
            ret = spi_flash_erase_sector(serial_num+erasecount);
            if(ret != SPI_FLASH_RESULT_OK)
            {
                GAgent_Printf(GAGENT_WARNING,"erase sector err=%d\n",ret);
            }
            erasecount++;
            ret = spi_flash_erase_sector(serial_num+erasecount);
            if(ret != SPI_FLASH_RESULT_OK)
            {
                GAgent_Printf(GAGENT_WARNING,"erase sector err=%d\n",ret);
            }
            erasecount++;
        }

        //write to flash
        ret = spi_flash_write(des_addr+offset, tmpbuf, len);
        if(0 != ret)
        {
            GAgent_Printf(GAGENT_WARNING,"spi_flash_write failed,err=%d\n",ret);
            writeFlag = -1;
            break;
        }
        offset += len;
    }
    os_printf("offset = %d\n",offset);
    system_soft_wdt_restart();
    if(-1 != writeFlag )
    {
        GAgent_Printf( GAGENT_CRITICAL,"copy firmware success!\n");
        memcpy(pgc->gc.master_softver,pgc->gc.slave_softver,sizeof(pgc->gc.slave_softver));
        pgc->gc.otaStatus = OTA_INIT;
        pgc->gc.boot_run_cnt = 0;
        GAgent_DevSaveConfigData( &(pgc->gc));
        GAgent_ChangeRunArea();
        return RET_SUCCESS;
    }
    else
    {
        GAgent_Printf( GAGENT_CRITICAL,"copy firmware fail!\n");
        if(0x1000 == des_addr)
        {
            pgc->rtinfo.isOtaRunning = 0;
            system_os_post( 1, SIG_CLOUD_HTTP, NULL );
        }
        offset = 0;
        return RET_FAILED;
    }
}
void ICACHE_FLASH_ATTR
GAgent_SaveFwHeadInfo(pgcontext pgc)
{
    int32 ret;

    ret = spi_flash_erase_sector(520);
    if(0 != ret)
    {
        GAgent_Printf(GAGENT_WARNING,"spi_flash_erase_sector failed,err=%d\n",ret);

    }
    ret = spi_flash_write(0x208000, &(pgc->rtinfo.firmwareInfo), pgc->rtinfo.firmwareInfo.file_offset);
    if( ret != 0 )
    {
        GAgent_Printf( GAGENT_WARNING,"spi_flash_write failed,err = %d\n",ret);
        return RET_FAILED;
    }

    return RET_SUCCESS;
}
int32 ICACHE_FLASH_ATTR
GAgent_GetFwHeadInfo(pgcontext pgc, fileInfo *fwInfo)
{
    int32 ret;
    ret = spi_flash_read(0x208000, fwInfo, sizeof(fileInfo));
    if( ret != 0 )
    {
        GAgent_Printf( GAGENT_WARNING,"spi_flash_read failed,error num = %d\n",ret);
        return RET_FAILED;
    }

    return RET_SUCCESS;
}
int32 ICACHE_FLASH_ATTR GAGENT_CheckOtaStatus( pgcontext pgc )
{
    int32 dTime;

    if(0 < pgContextData->rtinfo.lastRxOtaDataTime)//从接收OTA分包数据的第二包开始计算时间间隔
    {
        dTime = abs(GAgent_GetDevTime_S() - pgContextData->rtinfo.lastRxOtaDataTime);
        if(dTime > 5)
        {
            pgContextData->rtinfo.lastRxOtaDataTime = 0;
            pgContextData->rtinfo.isOtaRunning = 0;
            pgContextData->rtinfo.otaWriteLen = 0;
            if(CLOUD_RES_WIFI_OTA == pgContextData->rtinfo.waninfo.CloudStatus)
            {
                GAgent_Printf( GAGENT_CRITICAL,"Http timeout,GAgent download WIFI firmware failed,go to check MCU OTA!\n");
                if(0 == pgContextData->rtinfo.onlinePushflag)
                {
                    pgContextData->rtinfo.OTATypeflag = OTATYPE_MCU;
                    GAgent_SetCloudConfigStatus( pgContextData,CLOUD_RES_GET_SOFTVER );
                    system_os_post( 1, SIG_CLOUD_HTTP, NULL);
                }
                else //online push
                {
                    pgContextData->rtinfo.onlinePushflag = 0;
                    GAgent_SetCloudConfigStatus ( pgContextData,CLOUD_CONFIG_OK );
                }
            }
            else
            {

                GAgent_Printf( GAGENT_CRITICAL,"Http timeout,GAgent download MCU firmware failed!\n");
                if(0 == pgContextData->rtinfo.onlinePushflag)
                {
                    if(1 == GAgent_IsNeedDisableDID(pgContextData))
                    {
                        GAgent_Printf(GAGENT_INFO,"Need to Disable Device ID!");
                        GAgent_SetCloudConfigStatus( pgContextData,CLOUD_RES_DISABLE_DID );
                        if(pgContextData->rtinfo.waninfo.http_socketid > 0)
                        {
                            GAgent_Printf(GAGENT_WARNING,"Req to Disable Device ID!");
                            Cloud_ReqDisable( pgContextData );
                        }
                        else
                        {
                            Cloud_InitSocket( pgContextData->rtinfo.waninfo.http_socketid,pgContextData->gc.GServer_ip,80,0 );
                        }
                    }
                    else //do not need disable did
                    {
                       GAgent_SetCloudConfigStatus ( pgContextData,CLOUD_CONFIG_OK );
                       system_os_post( 1, SIG_CLOUD_HTTP, NULL);
                    }
                }
                else //online push
                {
                    pgContextData->rtinfo.onlinePushflag = 0;
                    GAgent_SetCloudConfigStatus ( pgContextData,CLOUD_CONFIG_OK );
                }
            }
        }
    }
    return;
}
