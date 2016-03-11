#include "gagent.h"
#include "http.h"
#include "string.h"
#include "local.h"

extern int32 g_MQTTStatus;

int32 ICACHE_FLASH_ATTR
Http_POST( int32 socketid, const int8 *host,const int8 *passcode,const int8 *mac,const int8 *product_key )
{
    int32 ret=0;
    uint8 *postBuf=NULL;
    int8 *url = "/dev/devices";
    int8 Content[100]={0};
    int32 ContentLen=0;
    int32 totalLen=0;

    postBuf = (uint8*)os_malloc(400);
    if (postBuf==NULL) return 1;
    //g_globalvar.http_sockettype =HTTP_GET_DID;//http_sockettype=1 :http_post type.

    os_sprintf(Content,"passcode=%s&mac=%s&product_key=%s",passcode,mac,product_key);
    ContentLen=os_strlen(Content);
    os_sprintf( (char *)postBuf,"%s %s %s%s%s %s%s%s %d%s%s%s%s%s",
              "POST" ,url,"HTTP/1.1",kCRLFNewLine,
              "Host:",host,kCRLFNewLine,
              "Content-Length:",ContentLen,kCRLFNewLine,
              "Content-Type: application/x-www-form-urlencoded",kCRLFNewLine,
              kCRLFNewLine,
              Content
        );
    totalLen = os_strlen( (char *)postBuf );
    GAgent_Printf(GAGENT_DEBUG,"http_post:%s %d",postBuf,totalLen);
    ret = espconn_sent(pgContextData->rtinfo.waninfo.phttp_fd,postBuf,totalLen );
    GAgent_Printf(GAGENT_DEBUG,"http_post ret: %d",ret);
    os_free( postBuf );
    return ret;
}

int32 ICACHE_FLASH_ATTR
Http_GET( const int8 *host,const int8 *did,int32 socketid )
{
    static int8 *getBuf=NULL;
    int32 totalLen=0;
    int32 ret=0;
    int8 *url = "/dev/devices/";

    getBuf = (int8*)os_malloc( 200 );
    if(getBuf == NULL)
    {
        return 1;
    }
    os_memset( getBuf,0,200 );
    //g_globalvar.http_sockettype =HTTP_PROVISION;//http get type.
    os_sprintf( getBuf,"%s %s%s %s%s%s %s%s%s%s%s",
              "GET",url,did,"HTTP/1.1",kCRLFNewLine,
              "Host:",host,kCRLFNewLine
              "Cache-Control: no-cache",kCRLFNewLine,
              "Content-Type: application/x-www-form-urlencoded",kCRLFLineEnding);
    totalLen = os_strlen( getBuf );

    ret = espconn_sent(pgContextData->rtinfo.waninfo.phttp_fd,getBuf,totalLen );
    GAgent_Printf(GAGENT_DEBUG,"Sent provision:\n %s\n", getBuf);
    os_free(getBuf);
    getBuf = NULL;

    if(ret == 0 )
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
/******************************************************
*
*   FUNCTION        :   delete device did by http
*
*   return          :   0--send successful
*                       1--fail.
*   Add by Alex lin  --2014-12-16
*
********************************************************/
int32 ICACHE_FLASH_ATTR Http_Delete( int32 socketid, const int8 *host,const int8 *did,const int8 *passcode )
{
    int32 ret=0;
    int8 *sendBuf=NULL;
    int8 *url = "/dev/devices";
    int8 *Content = NULL;
    int32 ContentLen=0;
    int32 totalLen=0;
    int8 *DELETE=NULL;
    int8 *HOST=NULL;
    int8 Content_Length[20]={0};
    int8 *contentType="Content-Type: application/x-www-form-urlencoded\r\n\r\n";

    DELETE = (int8*)os_malloc(os_strlen("DELETE  HTTP/1.1\r\n")+os_strlen(url)+1);//+1 for sprintf
    if( DELETE ==NULL )
    {
        return 1;
    }
    HOST = (int8*)os_malloc(os_strlen("Host: \r\n")+os_strlen(host)+1);// +1 for sprintf
    if( HOST==NULL)
    {
      os_free(DELETE);
      return 1;
    }
    Content = (int8*)os_malloc(os_strlen("did=&passcode=")+os_strlen(did)+os_strlen(passcode)+1);// +1 for sprintf
    if( Content==NULL )
    {
      os_free(DELETE);
      os_free(HOST);
      return 1;
    }

    os_sprintf(Content,"did=%s&passcode=%s",did,passcode);
    ContentLen=os_strlen(Content);
    os_sprintf(DELETE,"DELETE %s HTTP/1.1\r\n",url);
    os_sprintf(HOST,"Host: %s\r\n",host);
    os_sprintf(Content_Length,"Content-Length: %d\r\n",ContentLen);
    sendBuf = (int8*)os_malloc(os_strlen(DELETE)+os_strlen(HOST)+os_strlen(Content_Length)+os_strlen(contentType)+ContentLen+1);//+1 for sprintf
    if (sendBuf==NULL)
    {
      os_free(DELETE);
      os_free(HOST);
      os_free(Content);
      return 1;
    }
    os_sprintf(sendBuf,"%s%s%s%s%s",DELETE,HOST,Content_Length,contentType,Content);
    totalLen = os_strlen(sendBuf);
    //    ret = send( socketid, sendBuf,totalLen,0 );
    ret = espconn_sent(pgContextData->rtinfo.waninfo.phttp_fd,sendBuf,totalLen );
    if(ret<=0)
    {
      GAgent_Printf(GAGENT_ERROR," send fail %s %s %d",__FILE__,__FUNCTION__,__LINE__);
      return 1;
    }

    GAgent_Printf( GAGENT_DEBUG , "totalLen = %d",totalLen);
    GAgent_Printf(GAGENT_DEBUG,"%s",sendBuf);

    os_free(DELETE);
    os_free(HOST);
    os_free(Content);
    os_free(sendBuf);
    return 0;
}
/******************************************************
//        functionname    :   Http_ReadSocket
//        description     :   read data form socket
//        socket          :   http server socket
//        Http_recevieBuf :   data buf.
//        bufLen          :   read data length
//        return          :   >0 the data length read form
//                            socket
//                            <0 error,and need to close
//                            the socket.
//******************************************************/
//int32 Http_ReadSocket( int32 socket,uint8 *Http_recevieBuf,int32 bufLen )
//{
//    int32 bytes_rcvd = 0;
//    if( socket<=0 )
//        return bytes_rcvd;
//    memset(Http_recevieBuf, 0, bufLen);

//    bytes_rcvd = recv(socket, Http_recevieBuf, bufLen, 0 );
//    if(bytes_rcvd <= 0)
//    {
//        GAgent_Printf(GAGENT_DEBUG,"Close HTTP Socket[%d].", socket);
//        MBM;
//        return -2;
//    }
//    return bytes_rcvd;
//}
/******************************************************
*
*   Http_recevieBuf :   http receive data
*   return          :   http response code
*   Add by Alex lin  --2014-09-11
*
********************************************************/
int32 ICACHE_FLASH_ATTR
Http_Response_Code( uint8 *Http_recevieBuf )
{
    int32 response_code=0;
    int8* p_start = NULL;
    int8* p_end =NULL;
    int8 re_code[10] ={0};
    os_memset(re_code,0,sizeof(re_code));

    p_start = os_strstr( (char *)Http_recevieBuf," " );
    if(NULL == p_start)
    {
        GAgent_Printf( GAGENT_DEBUG,"file:%s function:%s line:%d ",__FILE__,__FUNCTION__,__LINE__ );
        os_printf("p_start is null\n");
        return RET_FAILED;
    }
    p_end = os_strstr( ++p_start," " );
    if(p_end)
    {
        if(p_end - p_start > sizeof(re_code))
        {
            os_printf("(p_end - p_start > sizeof(re_code)\n");
            return RET_FAILED;
        }
        os_memcpy( re_code,p_start,(p_end-p_start) );
    }

    response_code = atoi(re_code);
    return response_code;
}
int32 ICACHE_FLASH_ATTR
Http_Response_DID( uint8 *Http_recevieBuf,int8 *DID )
{
    int8 *p_start = NULL;
    os_memset(DID,0,DID_LEN);
    p_start = os_strstr( (char *)Http_recevieBuf,"did=");
    if( p_start==NULL )
        return 1;
    p_start = p_start+os_strlen("did=");
    os_memcpy(DID,p_start,DID_LEN);
    DID[DID_LEN - 2] ='\0';
    return 0;
}
int32 ICACHE_FLASH_ATTR
Http_getdomain_port( uint8 *Http_recevieBuf,int8 *domain,int32 *port )
{
    int8 *p_start = NULL;
    int8 *p_end =NULL;
    int8 Temp_port[10]={0};
    os_memset( domain,0,100 );
    p_start = os_strstr( (char *)Http_recevieBuf,"host=");
    if( p_start==NULL ) return 1;
    p_start = p_start+os_strlen("host=");
    p_end = os_strstr(p_start,"&");
    if( p_end==NULL )   return 1;
    os_memcpy( domain,p_start,( p_end-p_start ));
    domain[p_end-p_start] = '\0';
    p_start = os_strstr((++p_end),"port=");
    if( p_start==NULL ) return 1;
    p_start =p_start+os_strlen("port=");
    p_end = os_strstr( p_start,"&" );
    os_memcpy(Temp_port,p_start,( p_end-p_start));
    *port = atoi(Temp_port);
    return 0;
}
/******************************************************
// *   FUNCTION       :   sent the HTTP Get target data
// *                      to http server.
// *   host           :   http server host
// *   type           :   1-wifi ,2-mcu
// *   Add by Alex lin  --2014-10-29
// *
// ********************************************************/
//int32 Http_GetTarget( const int8 *host,
//                      const int8 *product_key,
//                      const int8 *did,enum OTATYPE_T type,
//                      const int8 *hard_version,
//                      const int8 *soft_version,
//                      const int32 current_fid,int32 socketid )
//{
//    //int8 getBuf[500] = {0};
//    int8 *getBuf=NULL;
//    int32 totalLen=0;
//    int32 ret=0;
//    int8 *url = "/dev/ota/target_fid?";
//    ;
//
//    getBuf = (int8*)malloc(500);
//    if(getBuf == NULL)
//    {
//        return 1;
//    }
//
//
//    memset( getBuf,0,500 );

//    sprintf( getBuf,"%s %s%s%s%s%s%s%d%s%s%s%s%s%d%s%s%s%s%s%s%s",
//             "GET",url,"did=",did,"&product_key=",product_key,
//             "&type=",type,"&hard_version=",hard_version,
//             "&soft_version=",soft_version,
//             "&current_fid=",current_fid," HTTP/1.1",kCRLFNewLine,
//             "Host: ",host,kCRLFNewLine,
//             "Content-Type: application/text",kCRLFLineEnding );

//    totalLen =strlen( getBuf );
//    GAgent_Printf(GAGENT_INFO,"totalLen=%d\r\n",totalLen);
//    ret = send( socketid, getBuf,totalLen,0 );
//    free(getBuf);
//    if(ret>0)
//        return 0;

//    return 1;
//}

int32 ICACHE_FLASH_ATTR
CheckFirmwareUpgrade(const int8 *host, const int8 *did,enum OTATYPE_T type,
                              const int8 *passcode,const int8 *hard_version,
                              const int8 *soft_version, int32 socketid )
{
    int8 *getBuf=NULL;
    int32 totalLen=0;
    int32 ret=0;
    int8 *url = "/dev/ota/v4.1/update_and_check";
    int8 Content[100]={0};
    int32 ContentLen=0;
    getBuf = (int8*)os_malloc(500);
    if(getBuf == NULL)
    {
        return RET_FAILED;
    }
    os_sprintf(Content,"passcode=%s&type=%d&hard_version=%s&soft_version=%s",passcode,type,hard_version,soft_version);
    ContentLen=os_strlen(Content);
    os_memset( getBuf,0,500 );
    os_sprintf( getBuf,"%s %s%s%s %s%s%s%s%s%s%d%s%s%s%s%s",
             "POST",url,"/",did,"HTTP/1.1",kCRLFNewLine,
             "Host:",host,kCRLFNewLine,
             "Content-Length:",ContentLen,kCRLFNewLine,
             "Content-Type: application/x-www-form-urlencoded",kCRLFNewLine,
             kCRLFNewLine,
             Content
             );
    totalLen =os_strlen( getBuf );
    GAgent_Printf(GAGENT_INFO,"Http_post_totalLen=%d\r\n",totalLen);
    //ret = send( socketid, getBuf,totalLen,0 );
    ret = espconn_sent(pgContextData->rtinfo.waninfo.phttp_fd,getBuf,totalLen );
    GAgent_Printf( GAGENT_DEBUG,"Req OTA Buf:\n%s",getBuf );
    os_free(getBuf);
    if(ret>0)
        return RET_SUCCESS;

    return RET_FAILED;
}

/******************************************************
*FUNCTION      :   get the http return softver
*                  and download url(for hf wifi)
*  softver     :   softver
*  download_url:   download_url
*          buf :   http receive data.
*       return :  0-return ok, 1-return fail.
*   Add by Alex lin  --2014-10-29
*
********************************************************/
int32 ICACHE_FLASH_ATTR
Http_GetSoftver_Url( int8 *download_url, int8 *softver, uint8 *buf )
{
    int8 *p_start = NULL;
    int8 *p_end =NULL;

    memset(softver, 0x0, 32);
    p_start = strstr( (char *)buf,"soft_ver=");
    if( p_start==NULL )  return 1;
    p_start =  p_start+(sizeof( "soft_ver=" )-1);
    p_end = strstr( p_start,"&");
    if( p_end==NULL )  return 1;
    memcpy( softver,p_start,(p_end-p_start));

    p_start = strstr(p_end,"download_url=");
    if(p_start==NULL ) return 1;
    p_start+=sizeof("download_url=")-1;
    p_end = strstr(p_start,kCRLFNewLine);
    if( p_end==NULL )  return 1;
    memcpy( download_url,p_start,(p_end-p_start));
    download_url[p_end-p_start] = '\0';

    return 0;
}

/******************************************************
// *
// *   FUNCTION       :   get the uuid by productkey in HTTP GET
// *      productkey  :   productkey
// *      host        :   host
// *      return      :   0-OK 1-fail
// *   Add by Alex lin  --2014-11-10
// *
// ********************************************************/
//int32 Http_JD_Get_uuid_req( const int8 *host,const int8 *product_key )
//{
//    int8 *getBuf=NULL;
//    int32 totalLen=0;
//    int32 ret=0;
//    int8 *url = "/dev/jd/product/";
//    if( strlen(product_key)<=0 )
//    {
//        GAgent_Printf(GAGENT_ERROR,"need a productkey to get uuid! ");
//        return 1;
//    }
//    getBuf = (int8*)malloc(500);
//    if (getBuf==NULL)  return 1;
//    memset( getBuf,0,500 );
//    //g_globalvar.http_sockettype =HTTP_GET_JD_UUID;//http get product_uuid for JD.

//    sprintf( getBuf,"%s %s%s%s%s%s%s%s%s%s",
//             "GET",url,product_key," HTTP/1.1",kCRLFNewLine,
//             "Host: ",host,kCRLFNewLine,
//             "Cache-Control: no-cache",kCRLFLineEnding );

//    totalLen =strlen( getBuf );
//    GAgent_Printf(GAGENT_DEBUG,"totalLen=%d\r\n",totalLen);
//    GAgent_Printf(GAGENT_DEBUG,"Sent Http to get JD uuid.\n");
//    GAgent_Printf(GAGENT_DUMP,"%s",getBuf );
//    free(getBuf);
//
//    if(ret>0) return 0;
//    else    return 1;
//}

/******************************************************
// *
// *   FUNCTION           :       post the feedid and accesskey to http server
// *      feedid          :       feedid
// *      accesskey       :       accesskey
// *      return          :
// *   Add by Alex lin  --2014-11-07
// *
// ********************************************************/
//int32 Http_JD_Post_Feed_Key_req( int32 socketid,int8 *feed_id,int8 *access_key,int8 *DId,int8 *host )
//{
//    int32 ret=0;
//    uint8 *postBuf=NULL;
//    int8 *url = "/dev/jd/";
//    int8 Content[200]={0};
//    int32 ContentLen=0;
//    int32 totalLen=0;
//
//    if(strlen(DId)<=0)
//        return 1;

//    postBuf = (uint8*)malloc(400);
//    if (postBuf==NULL)
//        return 1;
//    sprintf(Content,"feed_id=%s&access_key=%s",feed_id,access_key);
//    ContentLen = strlen( Content );

//    snprintf( (char *)postBuf,400,"%s%s%s%s%s%s%s%s%s%s%s%s%s%d%s%s%s",
//              "POST ",url,DId," HTTP/1.1",kCRLFNewLine,
//              "Host: ",host,kCRLFNewLine,
//              "Cache-Control: no-cache",kCRLFNewLine,
//              "Content-Type: application/x-www-form-urlencoded",kCRLFNewLine,
//              "Content-Length: ",ContentLen,kCRLFLineEnding,
//              Content,kCRLFLineEnding
//        );
//    totalLen = strlen( (char *)postBuf );
//    ret = send( socketid,postBuf,totalLen,0 );
//
//    free( postBuf );
//    return 0;
//}
/*********************************************************************
*
*   FUNCTION           :    GAgent OTA by url OTA success will reboot
*   download_url        :    download url
*   return             :     1-OTA fail.
*   Add by Alex lin  --2014-12-01
*
**********************************************************************/
//int32 GAgent_DoOTAbyUrl( const int8 *download_url )
//{
//    int8 *GetBuf=NULL;
//    int8 *p_start = NULL;
//    int8 *p_end =NULL;
//    int32 GetBufLen=0,ret=0;
//    int8 url[100]={0};
//    int8 GET[100]={0};
//    int8 HOST[100]={0};
//    int8 *Type="Content-Type: application/text\r\n";
//    int8 *Control="Cache-Control: no-cache\r\n\r\n";

//    GetBuf = (int8*)malloc(500);
//    if( GetBuf==NULL)
//    {
//        GAgent_Printf(GAGENT_INFO," malloc fail %s %s %d",__FILE__,__FUNCTION__,__LINE__);
//        return 1;
//    }
//    memset(GetBuf,0,500);

//    p_start = strstr( download_url,"/dev/ota/");
//    if( p_start==NULL ) return 1;
//    p_end = strstr(download_url,".bin");
//    if( p_end==NULL )       return 1;
//    p_end=p_end+strlen(".bin");
//    memcpy( url,p_start,(p_end-p_start));
//    url[ (p_end-p_start)] ='\0';

//    sprintf(GET,"GET %s HTTP/1.1%s",url,kCRLFNewLine);
//    sprintf(HOST,"Host: %s%s",HTTP_SERVER,kCRLFNewLine);
//    sprintf( GetBuf,"%s%s%s%s",GET,HOST,Type,Control);

//    GetBufLen = strlen(GetBuf);

//    g_globalvar.waninfo.send2HttpLastTime = DRV_GAgent_GetTime_S();
//    g_globalvar.connect2Cloud=0;
//    ret = Http_InitSocket(1);
//    ret = send( g_globalvar.waninfo.http_socketid, GetBuf,GetBufLen,0 );
//    g_globalvar.http_sockettype=HTTP_OTA;

//    free(GetBuf);
//}

/*********************************************************************
*
*   FUNCTION       :   TO get the http headlen
*     httpbuf      :   http receive buf
*     return       :   the http headlen.
*   Add by Alex lin  --2014-12-02
*
**********************************************************************/
int32 ICACHE_FLASH_ATTR
Http_HeadLen( uint8 *httpbuf )
{
    int8 *p_start = NULL;
    int8 *p_end =NULL;
    int32 headlen=0;
    p_start = (char *)httpbuf;
    p_end = os_strstr( (char *)httpbuf,kCRLFLineEnding);
    if( p_end==NULL )
    {
       GAgent_Printf(GAGENT_DEBUG,"Can't not find the http head!");
       return 0;
    }
    p_end=p_end+os_strlen(kCRLFLineEnding);
    headlen = (p_end-p_start);
    return headlen;
}
/*********************************************************************
*
*   FUNCTION       :   TO get the http bodylen
*      httpbuf     :   http receive buf
*      return      :   the http bodylen.(0-error)
*   Add by Alex lin  --2014-12-02
*
**********************************************************************/
int32 ICACHE_FLASH_ATTR
Http_BodyLen( uint8 *httpbuf )
{
    int8 *p_start = NULL;
    int8 *p_end =NULL;
    int8 bodyLenbuf[10]={0};
    int32 bodylen=0;  //Content-Length:
    p_start = os_strstr( (char *)httpbuf,"Content-Length: ");
    if( p_start==NULL ) return 0;
    p_start = p_start+os_strlen("Content-Length: ");
    p_end = os_strstr( p_start,kCRLFNewLine);
    if( p_end==NULL )   return 0;

    os_memcpy( bodyLenbuf,p_start,(p_end-p_start));
    bodylen = atoi(bodyLenbuf);
    return bodylen;
}
int32 ICACHE_FLASH_ATTR
Http_GetHost( int8 *downloadurl,int8 **host,int8 **url )
{
    int8 *p_start = NULL;
    int8 *p_end =NULL;
    int8 hostlen;

    p_start = os_strstr( downloadurl,"http://" );
    if(p_start==NULL)
       return RET_FAILED;
    p_start = p_start + os_strlen("http://");
    p_end = os_strstr( p_start,"/");
    if(p_end==NULL)
       return RET_FAILED;
    *host = (char *)os_malloc(p_end-p_start+1);
    if( NULL == *host )
    {
       GAgent_Printf(GAGENT_WARNING, "OTA host malloc fail!");
       return RET_FAILED;
    }
    os_memcpy(&(*host)[0],p_start,p_end-p_start);
    (*host)[p_end-p_start] = '\0';
    hostlen = p_end-p_start;

    p_start = p_end;
    if(p_start==NULL)
       return RET_FAILED;
    p_end = p_start + os_strlen(downloadurl) - hostlen;
    if(p_end==NULL)
       return RET_FAILED;
    *url = (char *)os_malloc(p_end-p_start+1);
    if( NULL == *url )
    {
       GAgent_Printf(GAGENT_WARNING, "OTA url malloc fail!");
       return RET_FAILED;
    }
    os_memcpy(&(*url)[0],p_start,p_end-p_start);
    (*url)[p_end-p_start] = '\0';

    return RET_SUCCESS;
}
/*************************************************************
*
*   FUNCTION  :  get MD5 from http head.
*   httpbuf   :  http buf.
*   MD5       :  MD5 form http head(16b).
*           add by alex.lin ---2014-12-04
*************************************************************/
int32 ICACHE_FLASH_ATTR
Http_GetMD5( uint8 *httpbuf,uint8 *MD5,int8 *strMD5)
{
    int8 *p_start = NULL;
    int8 *p_end =NULL;
    int8 MD5_TMP[16];
    uint8 Temp[3]={0};
    int8 *str;
    int32 i=0,j=0;
    p_start = os_strstr( (char *)httpbuf,"Firmware-MD5: ");
    if(p_start==NULL)
       return 1;
    p_start = p_start+os_strlen("Firmware-MD5: ");
    p_end = os_strstr( p_start,kCRLFNewLine);
    if(p_end==NULL)
       return 1;
    if((p_end-p_start)!=32) return 1;
    os_memcpy( strMD5,p_start,32 );
    strMD5[32] = '\0';
    os_memcpy( MD5,p_start,16);
    MD5[16]= '\0';

    for(i=0;i<32;i=i+2)
    {
       Temp[0] = strMD5[i];
       Temp[1] = strMD5[i+1];
       Temp[2] = '\0';
       MD5_TMP[j]= strtol(Temp, &str,16);
       j++;
    }
    os_memcpy(MD5,MD5_TMP,16);
    GAgent_Printf(GAGENT_DUMP," MD5 From HTTP:\n");
    for(j=0;j<16;j++)
       GAgent_Printf(GAGENT_DUMP,"%02x",MD5[j]);

    return 16;
}
int32 ICACHE_FLASH_ATTR
Http_GetFileType(uint8 *httpbuf)
{
    int8 *p_start = NULL;
    int8 *tmp_pstart = NULL;
    int8 *p_end =NULL;
    int8 fileExt_Len=0;
    int8 filename_Len = 0;
    int8 fileExt[5];
    int8 filename[100] = {0};
    int i;
    p_start = strstr( (char *)httpbuf,"filename=" );
    if(p_start==NULL)
       return 0;
    p_start = p_start+strlen("filename=" );
    p_end = strstr( p_start,kCRLFNewLine);
    if(p_end==NULL)
       return 0;
    filename_Len = (p_end-p_start);
    memcpy(filename,p_start,filename_Len);
    filename[filename_Len]='\0';

    p_start = strstr( (char *)filename,"." );
    if(p_start==NULL)
       return 0;
    while(p_start)
    {
        tmp_pstart = p_start+strlen(".");
        p_start = strstr( tmp_pstart, "." );
        if(!p_start)
        {
            p_start = tmp_pstart;
            break;
        }
    }
    p_end = strstr( p_start,"\"");
    if(p_end==NULL)
       return 0;

    fileExt_Len = p_end - p_start + 1;//add th last ' " '
    memcpy(fileExt,p_start,fileExt_Len);
    fileExt[fileExt_Len]='\0';
    for (i = 0; i < strlen(fileExt); i++)
    {
        fileExt[i] = tolower(fileExt[i]);
    }
    if( NULL != strstr(fileExt,"hex\"") )
    {
        GAgent_Printf(GAGENT_INFO,"download firmware type is hex.");
        return MCU_FIRMWARE_TYPE_HEX;
    }
    else if( NULL != strstr(fileExt,"bin\"") )
    {
        GAgent_Printf(GAGENT_INFO,"download firmware type is bin.");
        return MCU_FIRMWARE_TYPE_BIN;
    }
    else
    {
        GAgent_Printf(GAGENT_INFO,"download firmware type is unknow,default send by bin type.");
        return MCU_FIRMWARE_TYPE_BIN;
    }
}
int32 ICACHE_FLASH_ATTR
Http_ReqGetFirmwareLen( int8 *url,int8 *host,int32 socketid )
{
    static int8 *getBuf = NULL;
    int32 totalLen=0;
    int32 ret=0;
    getBuf = (int8*)os_malloc( 200 );
    if(getBuf == NULL)
    {
        if(NULL!=host)
        {
           os_free(host);
           host = NULL;
        }
        if(NULL!=url)
        {
           os_free(url);
           url = NULL;
        }
        return RET_FAILED;
    }
    os_memset( getBuf,0,200 );
    os_sprintf( getBuf,"%s %s %s%s%s %s%s%s%s",
              "HEAD",url,"HTTP/1.1",kCRLFNewLine,
              "Host:",host,kCRLFNewLine,
              "Content-Type: application/text",kCRLFLineEnding);
    totalLen =os_strlen( getBuf );
    ret = espconn_sent(pgContextData->rtinfo.waninfo.phttp_fd,getBuf,totalLen );
    GAgent_Printf( GAGENT_DEBUG,"Req mcu OTA len:%d send ret %d:\n%s",totalLen,ret ,getBuf );
    os_free(getBuf);
    getBuf = NULL;

    if(NULL!=host)
    {
       os_free(host);
       host = NULL;
    }
    if(NULL!=url)
    {
       os_free(url);
       url = NULL;
    }
    if(ret<0 )
    {
        return RET_FAILED;
    }
    else
    {
        return RET_SUCCESS;
    }
}

int32 ICACHE_FLASH_ATTR
Http_ReqGetFirmware( int8 *url,int8 *host,int32 socketid )
{
    static int8 *getBuf = NULL;
    int32 totalLen=0;
    int32 ret=0;
    getBuf = (int8*)os_malloc( 200 );
    if(getBuf == NULL)
    {
        if(NULL!=host)
        {
           os_free(host);
           host = NULL;
        }
        if(NULL!=url)
        {
           os_free(url);
           url = NULL;
        }
        return RET_FAILED;
    }
    os_memset( getBuf,0,200 );
    os_sprintf( getBuf,"%s %s %s%s%s %s%s%s%s",
              "GET",url,"HTTP/1.1",kCRLFNewLine,
              "Host:",host,kCRLFNewLine,
              "Content-Type: application/text",kCRLFLineEnding);
    totalLen =os_strlen( getBuf );
    ret = espconn_sent(pgContextData->rtinfo.waninfo.phttp_fd,getBuf,totalLen );
    GAgent_Printf( GAGENT_DEBUG,"Req OTA len:%d send ret %d:\n%s",totalLen,ret ,getBuf );
    os_free(getBuf);
    getBuf = NULL;

    if(NULL!=host)
    {
       os_free(host);
       host = NULL;
    }
    if(NULL!=url)
    {
       os_free(url);
       url = NULL;
    }
    if(ret!=0 )
    {
        return RET_FAILED;
    }
    else
    {
        return RET_SUCCESS;
    }
}
/****************************************************************
*       FunctionName     :      Http_Get3rdCloudInfo
*       Description      :      get 3rd cloud name and info form
*                               buf.
*       buf              :      data afer provision.
*       szCloud3Name     :      3rd cloud name.
*       szCloudInfo      :      3rd cloud info.
*       return           :      1 need 3rd cloud
*                               0 only need gizwits
*       Add by Alex.lin   --2015-03-03
****************************************************************/
//uint8 Http_Get3rdCloudInfo( int8 *szCloud3Name,int8 *szCloud3Info,uint8 *buf )
//{
//    int8 *p_start = NULL;
//    int8 *p_end =NULL;
//    int8 *cloudName = "3rd_cloud=";
//    int8 *uuid = "product_uuid=";
//
//    memset( szCloud3Name,0,10 );
//    memset( szCloud3Info,0,32 );
//
//    p_start = strstr( (char *)buf,cloudName );
//    if( p_start==NULL )
//        return 0;
//    p_start+=strlen( cloudName );
//    p_end = strstr( p_start,"&");
//    if( p_end==NULL )
//        return 0;
//    memcpy( szCloud3Name,p_start,(p_end-p_start) );
//    szCloud3Name[p_end-p_start]='\0';
//
//    p_start = strstr( p_start,uuid );
//    if( p_start==NULL )
//        return 0;
//    p_start +=strlen( uuid );
//    p_end = strstr( p_start,kCRLFNewLine );
//    if( p_end==NULL )
//        return 0;
//    memcpy( szCloud3Info,p_start,(p_end-p_start));
//    szCloud3Info[p_end-p_start]='\0';
//    return 1;
//}

