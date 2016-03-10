#include "gagent.h"
#include "lan.h"

int8 *buf_body;
static int8 sendflag = 0;
os_timer_t webtcp_recon_timer;
uint32 WebTcp_ReconnectTimer( pgcontext pgc );


uint32 ICACHE_FLASH_ATTR
web_tcp_server_recvcb(void *arg, char *pusrdata, unsigned short length)
{
    //os_printf("\nweb tcp recv  !!! \n%s\n",pusrdata);
    struct espconn *pespconn= arg;
    resetPacket(pgContextData->rtinfo.TcpRxbuf);
    os_memcpy(pgContextData->rtinfo.TcpRxbuf->phead, pusrdata,length);
    system_os_post( 1, SIG_WEBCONFIG, (os_param_t)length );

    return 0;
}

void ICACHE_FLASH_ATTR
web_tcp_server_disconcb(void *arg)
{
    struct espconn *pespconn = arg;
    os_printf("\nweb tcp disconnect succeed !!! \r\n");
    pgContextData->ls.tcpWebConfigFd = INVALID_SOCKET;
}
void ICACHE_FLASH_ATTR
web_tcp_server_reconcb(void *arg, sint8 err)
{
    os_printf("web tcp reconnect callback, error code %d !!! \r\n",err);
    pgContextData->ls.tcpWebConfigFd = INVALID_SOCKET;
}
void ICACHE_FLASH_ATTR
web_tcp_server_sentcb(void *arg)
{

    if( 1 == sendflag )
    {
        espconn_sent( pgContextData->ls.webTcpServer, buf_body, os_strlen(buf_body) );
    }
    if( 2 == sendflag )
    {
        espconn_sent( pgContextData->ls.webTcpServer, buf_body, os_strlen(buf_body) );
        pgContextData->rtinfo.webconfigflag = RET_SUCCESS;
    }
}
void ICACHE_FLASH_ATTR
web_tcp_server_listen(void *arg)
{
    struct espconn *pesp_conn = arg;
    pgContextData->ls.tcpWebConfigFd = 1;

    espconn_regist_recvcb(pesp_conn, web_tcp_server_recvcb);
    espconn_regist_reconcb(pesp_conn, web_tcp_server_reconcb);
    espconn_regist_disconcb(pesp_conn, web_tcp_server_disconcb);
    espconn_regist_sentcb(pesp_conn, web_tcp_server_sentcb);
}

int32 ICACHE_FLASH_ATTR
GAgent_CreateWebTcpServer( uint16 tcp_port )
{
    pgContextData->ls.webTcpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
    pgContextData->ls.webTcpServer->type = ESPCONN_TCP;
    pgContextData->ls.webTcpServer->state = ESPCONN_NONE;
    pgContextData->ls.webTcpServer->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    pgContextData->ls.webTcpServer->proto.tcp->local_port = tcp_port;
    espconn_regist_connectcb(pgContextData->ls.webTcpServer, web_tcp_server_listen);// register conn success cb
    espconn_accept(pgContextData->ls.webTcpServer);
    sendflag = 0;
    return 0;
}
uint32 ICACHE_FLASH_ATTR
WebTcp_ReconnectTimer( pgcontext pgc )
{
    if( INVALID_SOCKET == pgc->ls.tcpWebConfigFd )
    {
        GAgent_CreateWebTcpServer( 80 );
    }
    else
    {
        os_timer_disarm(&webtcp_recon_timer);
    }
}

int32 ICACHE_FLASH_ATTR
handleWebConfig( pgcontext pgc )
{
    int32   read_len;
    int8    *buf_head, *index_ssid, *index_pass, *p;
    pgconfig pConfigData=NULL;
    pConfigData = &(pgc->gc);

    buf_head = (char *)os_malloc(1024);
    if (buf_head == NULL)
    {
        return -1;
    }
    os_memset(buf_head, 0, 1024);
    os_memcpy(buf_head, pgContextData->rtinfo.TcpRxbuf->phead, 1024);
    buf_body = (char *)os_malloc(1024);
    if (buf_body == NULL)
    {
        os_free(buf_head);
        return -1;
    }

    os_memset(buf_body, 0, 1024);

    if( os_strstr(buf_head, "web_config.cgi") == NULL)
    {
        os_sprintf(buf_body, "%s", "<html><body><form action=\"web_config.cgi\" method=\"get\">"
                 "<div align=\"center\" style=\"font-size:30px; padding-top:100px;\">"
                 "<p>[0~9],[a~z],[A~Z],[-],[_],[.]</p>"
                 "<p>SSID:<input type=\"text\" name=\"ssid\" style=\"font-size:30px;\"/></p>"
                 "<p>PASS:<input type=\"text\" name=\"pass\" style=\"font-size:30px;\"/></p>"
                 "<p><input type=\"submit\" value=\"OK\" style=\"font-size:30px;\"/></p>"
                 "</form></body></html>");

        os_memset(buf_head, 0, 1024);
        os_sprintf(buf_head, "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html;charset=gb2312\r\n"
                 "Content-Length: %d\r\n"
                 "Cache-Control: no-cache\r\n"
                 "Connection: close\r\n\r\n",
                 (int)os_strlen(buf_body));

        sendflag  = 1;
        espconn_sent( pgc->ls.webTcpServer, buf_head, os_strlen(buf_head) );
    }
    else
    {
        //GET /web_config.cgi?fname=chensf&lname=pinelinda HTTP/1.1
        index_ssid = os_strstr(buf_head, "ssid=");
        index_pass = os_strstr(buf_head, "pass=");
        if(index_ssid && index_pass)
        {
            index_ssid += os_strlen("ssid=");
            index_pass += os_strlen("pass=");
            p = os_strchr(index_ssid, '&');
            if(p) *p = '\0';
            p = os_strchr(index_pass, ' ');
            if(p) *p = '\0';

            if((os_strlen(index_ssid) > 32) || (os_strlen(index_pass) > 32))
            {
                GAgent_Printf( GAGENT_CRITICAL,"web_config SSID and pass too length !" );
            }
            GAgent_Printf( GAGENT_CRITICAL,"web_config SSID and pass !" );
            os_memset(pConfigData->wifi_ssid, 0, SSID_LEN_MAX );
            os_memset(pConfigData->wifi_key, 0, WIFIKEY_LEN_MAX);

            os_memcpy(pConfigData->wifi_ssid, index_ssid, os_strlen(index_ssid));
            os_memcpy(pConfigData->wifi_key, index_pass, os_strlen(index_pass));
            pConfigData->wifi_ssid[ os_strlen(index_ssid) ] = '\0';
            pConfigData->wifi_key[ os_strlen(index_pass) ] = '\0';

            pConfigData->flag |= XPG_CFG_FLAG_CONNECTED;
            pConfigData->flag |= XPG_CFG_FLAG_CONFIG;
            pgc->ls.onboardingBroadCastTime = SEND_UDP_DATA_TIMES;
            GAgent_DevSaveConfigData( pConfigData );

            os_sprintf(buf_body, "%s", "<html><body>"
                     "<div align=\"center\" style=\"font-size:40px; padding-top:100px;\">"
                     "<p>Config the wifi module success</p>"
                     "</body></html>");

            os_memset(buf_head, 0, 1024);
            os_sprintf(buf_head, "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html;charset=gb2312\r\n"
                     "Content-Length: %d\r\n"
                     "Cache-Control: no-cache\r\n"
                     "Connection: close\r\n\r\n",
                     (int)os_strlen(buf_body));

            sendflag  = 2;
            espconn_sent( pgc->ls.webTcpServer, buf_head, os_strlen(buf_head) );
        }
    }
    os_free(buf_head);
    os_free(buf_body);
//    msleep(100);
    return 0;
}
/****************************************************************
Function    :   Socket_CreateWebConfigServer
Description :   creat web config server.
tcp_port    :   web server port.#default 80
return      :   the web socket id .
Add by Alex.lin     --2015-04-24.
****************************************************************/
int32 ICACHE_FLASH_ATTR
GAgent_CreateWebConfigServer( uint16 tcp_port )
{
    return GAgent_CreateWebTcpServer( tcp_port );
}
/****************************************************************
Function    :   GAgent_DoTcpWebConfig
Description :   creat web config server.
return      :   NULL
Add by Alex.lin     --2015-04-25.
****************************************************************/
void ICACHE_FLASH_ATTR
GAgent_DoTcpWebConfig( pgcontext pgc )
{
    uint16 GAgentStatus=0;
    int32 newfd = 0;
//    struct sockaddr_t addr;
//    int32 addrLen= sizeof(addr);
    GAgentStatus = pgc->rtinfo.GAgentStatus;

    //will close socket when conn AP success
//    if( (GAgentStatus&WIFI_MODE_AP)!= WIFI_MODE_AP )
//    {
//        if( pgc->ls.tcpWebConfigFd>0 )
//        {
//            close( pgc->ls.tcpWebConfigFd );
//            pgc->ls.tcpWebConfigFd = INVALID_SOCKET;
//        }
//        return ;
//    }
//    if( (GAgentStatus&WIFI_MODE_ONBOARDING)!= WIFI_MODE_ONBOARDING )
//        return ;
    if( pgc->ls.tcpWebConfigFd <= 0 )
    {
        GAgent_Printf( GAGENT_INFO,"Creat Tcp Web Server." );
        pgc->ls.tcpWebConfigFd = GAgent_CreateWebConfigServer( 80 );
    }
//    if( pgc->ls.tcpWebConfigFd<0 )
//        return ;
//    if( !FD_ISSET(pgc->ls.tcpWebConfigFd, &(pgc->rtinfo.readfd)) )
//        return ;
//
//    newfd = Socket_accept(pgc->ls.tcpWebConfigFd, &addr, (socklen_t *)&addrLen);
//    if(newfd < 0)
//        return ;
//    handleWebConfig( pgc,newfd);
//    close(newfd);
//    newfd = INVALID_SOCKET;
}



