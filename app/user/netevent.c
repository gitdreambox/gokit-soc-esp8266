#include "netevent.h"
#include "gagent.h"
#include "platform.h"
#include "lan.h"
#include "lanudp.h"
#include "gagent_md5.h"
struct espconn tcpespconn;
struct espconn udpespconn;
struct espconn udpbroespconn;


LOCAL void ICACHE_FLASH_ATTR
tcp_server_sentcb(void *arg)
{
    //data sent successfully
    GAgent_Printf(GAGENT_DEBUG,"tcp sent cb");
}
/******************************************************************************
 * FunctionName : tcp_server_recv_cb
 * Description  : receive callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL uint32 ICACHE_FLASH_ATTR
tcp_server_recvcb(void *arg, char *pusrdata, unsigned short length)
{
    int i,j;
    int isNewClient = 1;
    int overClientMax = 0;

    struct espconn *pespconn= arg;

    for(i = 0;i < LAN_TCPCLIENT_MAX; i++)
    {
        for(j=0;j<4;j++)
        {
            if(pespconn->proto.tcp->remote_ip[j] != pgContextData->ls.tcpClient[i].remote_ip[j])
                continue;
            else if(3 == j && pespconn->proto.tcp->remote_port== pgContextData->ls.tcpClient[i].remote_port)
            {
                pgContextData->ls.tcpClient[i].fd_isset = 1;
                isNewClient = 0;
                break;
            }
        }
    }

    for(i = 0;i < LAN_TCPCLIENT_MAX; i++)
    {
        if(pgContextData->ls.tcpClient[i].fd < 0)
        {
            if(1 == isNewClient)
            {
                GAgent_Printf(GAGENT_DEBUG,"new client!,port=%d  ip=%d\n",pespconn->proto.tcp->remote_port,pespconn->proto.tcp->remote_ip[3]);
                Lan_AddTcpNewClient(pgContextData, (pespconn->proto.tcp->remote_ip),pespconn->proto.tcp->remote_port);
            }
            resetPacket(pgContextData->rtinfo.TcpRxbuf);
            os_memcpy(pgContextData->rtinfo.TcpRxbuf->phead, pusrdata,length);
            system_os_post( 1, SIG_TCP, (os_param_t)length );
            return 0;
        }
    }
    espconn_disconnect(pespconn);
    GAgent_Printf(GAGENT_WARNING, "[LAN]tcp client over %d channel, denied!", LAN_TCPCLIENT_MAX);
    return -1;
}

/******************************************************************************
 * FunctionName : tcp_server_discon_cb
 * Description  : disconnect callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
tcp_server_disconcb(void *arg)
{
    //tcp disconnect successfully
    //need clear ip port
    int32 i,j;
    struct espconn *pespconn = arg;
    GAgent_Printf(GAGENT_DEBUG,"tcp disconnect succeed !!!\n");
    for(i = 0;i < LAN_TCPCLIENT_MAX; i++)
    {
        for(j=0;j<4;j++)
        {
            if(pespconn->proto.tcp->remote_ip[j] != pgContextData->ls.tcpClient[i].remote_ip[j])
                continue;
            else if(3 == j && pespconn->proto.tcp->remote_port== pgContextData->ls.tcpClient[i].remote_port
                    && (LAN_CLIENT_LOGIN_SUCCESS == pgContextData->ls.tcpClient[i].isLogin))
            {
                pgContextData->ls.tcpClient[i].fd = INVALID_SOCKET;
                pgContextData->ls.tcpClient[i].remote_port = 0;
                if(pgContextData->ls.tcpClientNums > 0)
                {
                    pgContextData->ls.tcpClientNums--;
                }
                if(0 == (pgContextData->ls.tcpClientNums + pgContextData->rtinfo.waninfo.wanclient_num))
                {
                    GAgent_SetWiFiStatus( pgContextData,WIFI_CLIENT_ON,0 );
                }
                break;
            }
        }
    }
}
/******************************************************************************
 * FunctionName : tcp_server_recon_cb
 * Description  : reconnect callback, error occured in TCP connection.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
tcp_server_reconcb(void *arg, sint8 err)
{
   //error occured , tcp connection broke.

    GAgent_Printf(GAGENT_WARNING,"tcp reconnect callback, error code %d !!!\n",err);
}
LOCAL void ICACHE_FLASH_ATTR
tcp_server_listen(void *arg)
{
    struct espconn *pesp_conn = arg;
    pgContextData->ls.tcpServerFd = 1;

    espconn_regist_recvcb(pesp_conn, tcp_server_recvcb);
    espconn_regist_reconcb(pesp_conn, tcp_server_reconcb);
    espconn_regist_disconcb(pesp_conn, tcp_server_disconcb);
    espconn_regist_sentcb(pesp_conn, tcp_server_sentcb);
}
int32 ICACHE_FLASH_ATTR
GAgent_CreateTcpServer( uint16 tcp_port )
{
    pgContextData->ls.tcpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
    pgContextData->ls.tcpServer->type = ESPCONN_TCP;
    pgContextData->ls.tcpServer->state = ESPCONN_NONE;
    pgContextData->ls.tcpServer->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    pgContextData->ls.tcpServer->proto.tcp->local_port = tcp_port;
    espconn_regist_connectcb(pgContextData->ls.tcpServer, tcp_server_listen);// register conn success cb
    espconn_accept(pgContextData->ls.tcpServer);// 0:success

    return 0;
}
int32 ICACHE_FLASH_ATTR
GAgent_CreateUDPServer( uint16 udp_port )
{
    udpespconn.type = ESPCONN_UDP;
    udpespconn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    udpespconn.proto.udp->local_port = udp_port;
    espconn_regist_recvcb(&udpespconn, lan_udp_recv);
    espconn_regist_sentcb(&udpespconn, lan_udp_sentcb);
    if(espconn_create(&udpespconn))
        pgContextData->ls.udpServerFd = 1;
    return 0;
}
int32 ICACHE_FLASH_ATTR
GAgent_CreateUDPBroadCastServer( uint16 udpbroadcast_port )
{
    wifi_set_broadcast_if(1);
    //struct espconn udp_broadcast;
    udpbroespconn.type = ESPCONN_UDP;
    udpbroespconn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    udpbroespconn.proto.udp->remote_port= udpbroadcast_port;
    udpbroespconn.proto.udp->local_port= udpbroadcast_port;
    uint8 ip[4]={255,255,255,255};
    os_memcpy( udpbroespconn.proto.udp->remote_ip,ip,4);
    espconn_regist_recvcb(&udpbroespconn, lan_udp_recv);
    if(espconn_create(&udpbroespconn))
        pgContextData->ls.udpBroadCastServerFd = 1;
    return 0;
}
int32 ICACHE_FLASH_ATTR
Http_ResGetFirmware( pgcontext pgc,int32 socketid,u16 length,uint8 otatype,uint8* httpReceiveBuf )
{
    int32 ret,i;
    int32 headlen = 0;
    static int32 offset = 0;
    static int32 writelen = 0;
    static int8 downloadflag = 1;
    uint8 md5_calc[16] = {0};
    uint8 *buf = NULL;
    uint8 disableDIDflag = 0;
    static uint8 MD5[16] = {0};
    static MD5_CTX ctx;

    if( 0 == downloadflag )
    {
        pgc->rtinfo.onlinePushflag = 0;
        pgc->rtinfo.isOtaRunning = 0;
        return RET_FAILED;
    }

    if( 0 == writelen )
    {
        downloadflag = 1;
        ret = Http_Response_Code( httpReceiveBuf );
        if(200 != ret)
        {
            pgc->rtinfo.isOtaRunning = 0;
            //system_os_post( 1, SIG_CLOUD_M2M, NULL);
            return RET_FAILED;
        }
        headlen = Http_HeadLen( httpReceiveBuf );
        pgc->rtinfo.filelen = Http_BodyLen( httpReceiveBuf );
        GAgent_Printf(GAGENT_INFO,"Firmwarelen = %d\n",pgc->rtinfo.filelen);
        if(OTA_TYPE_MCU == otatype)
        {
            Http_GetMD5( httpReceiveBuf,MD5,pgc->mcu.MD5 );
            pgc->mcu.mcu_firmware_type = Http_GetFileType(httpReceiveBuf);
            pgc->mcu.firmwareLen = pgc->rtinfo.filelen;
        }
        else
        {
            Http_GetMD5( httpReceiveBuf,MD5,pgc->gc.MD5 );
            pgc->gc.wifiFirmwareLen = pgc->rtinfo.filelen;
        }
        buf = httpReceiveBuf + headlen;
        writelen = length - headlen;
        GAgent_MD5Init(&ctx);
    }
    else
    {
        writelen = length;
        buf = httpReceiveBuf;
    }
    ret = GAgent_SaveUpgradFirmware( offset, buf, writelen, otatype );
    os_printf("totallen = %d\n",offset+writelen);
    if(ret != 0)
    {
        GAgent_Printf(GAGENT_INFO, "[CLOUD]%s OTA upgrad fail at off:0x%x", __func__, offset);
        if( 0 == pgc->rtinfo.onlinePushflag )
        {
            pgc->rtinfo.isOtaRunning = 0;
            //system_os_post( 1, SIG_CLOUD_M2M, NULL);
        }
        offset = 0;
        writelen = 0;
        downloadflag = 0;
        return RET_FAILED;
    }
    offset += writelen;
    GAgent_MD5Update(&ctx, buf, writelen);
    if( pgc->rtinfo.filelen == offset )
    {
        GAgent_MD5Final(&ctx, md5_calc);
        if(os_memcmp(MD5, md5_calc, 16) != 0)
        {

            GAgent_Printf(GAGENT_WARNING,"[CLOUD]md5 fail!");
            offset = 0;
            writelen = 0;
            return RET_FAILED;
        }
        else
        {
            GAgent_DevSaveConfigData( &(pgc->gc));
            offset = 0;
            writelen = 0;
            pgc->rtinfo.firmwareInfo.file_offset = 4096;//sizeof(fileInfo);
            pgc->rtinfo.firmwareInfo.file_len = pgc->rtinfo.filelen;
            memcpy(pgc->rtinfo.firmwareInfo.soft_ver,pgc->gc.FirmwareVer,8);
            pgc->rtinfo.firmwareInfo.soft_ver[8] = '\0';
            pgc->rtinfo.lastRxOtaDataTime = 0;

            if(OTA_TYPE_MCU == otatype)
            {
                //save in flash
                pgc->rtinfo.firmwareInfo.file_type = OTA_TYPE_MCU;
                memcpy(pgc->rtinfo.firmwareInfo.hard_ver,pgc->mcu.hard_ver,sizeof(pgc->mcu.hard_ver));
                pgc->rtinfo.firmwareInfo.mcu_type = pgc->mcu.mcu_firmware_type;
                memcpy(pgc->rtinfo.firmwareInfo.md5,pgc->mcu.MD5,32);
                GAgent_SaveFwHeadInfo(pgc);

                os_printf("\r\n");
                GAgent_Printf( GAGENT_CRITICAL,"GAgent Download MCU Firmware success!\n");
                GAgent_LocalInformMcu(pgc,pgc->mcu.firmwareLen);
                pgc->rtinfo.isOtaRunning = 0;
                if(0 == pgc->rtinfo.onlinePushflag)
                {
                    if(1 == GAgent_IsNeedDisableDID(pgc))
                    {
                        GAgent_Printf(GAGENT_INFO,"Need to Disable Device ID!");
                        GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_DISABLE_DID );
                        if(pgc->rtinfo.waninfo.http_socketid > 0)
                        {
                            GAgent_Printf(GAGENT_WARNING,"Req to Disable Device ID!");
                            Cloud_ReqDisable( pgc );
                        }
                        else
                        {
                            Cloud_InitSocket( pgc->rtinfo.waninfo.http_socketid,pgc->gc.GServer_ip,80,0 );
                        }
                    }
                    else //do not need disable did
                    {
                       GAgent_SetCloudConfigStatus ( pgc,CLOUD_CONFIG_OK );
                       espconn_disconnect(pgc->rtinfo.waninfo.phttp_fd);
                       //system_os_post( 1, SIG_CLOUD_M2M, NULL);
                    }
                }
                else
                {
                    pgc->rtinfo.onlinePushflag = 0;
                    GAgent_SetCloudConfigStatus ( pgc,CLOUD_CONFIG_OK );
                }

            }
            else //download wifi fw success
            {
                //save in flash
                pgc->rtinfo.firmwareInfo.file_type = OTA_TYPE_WIFI_MASTER;
                memcpy(pgc->rtinfo.firmwareInfo.hard_ver,WIFI_HARDVER,8);
                memcpy(pgc->rtinfo.firmwareInfo.md5,pgc->gc.MD5,32);
                GAgent_SaveFwHeadInfo(pgc);

                os_printf("\r\n");
                GAgent_Printf( GAGENT_CRITICAL,"GAgent Download WIFI Firmware success,go to copy it to code area!\n");

                system_os_post(1, SIG_WRITE_FW, NULL);
            }
        }
    }
    return RET_SUCCESS;
}
