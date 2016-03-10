#ifndef _GAGENT_TYPEDEF_H_H_
#define _GAGENT_TYPEDEF_H_H_
#include "platform.h"

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
//typedef char  int8;
typedef short int16;
typedef int   int32;

typedef unsigned int u32;
typedef unsigned char u8;
typedef unsigned short u16;


typedef void (*task)(void *arg);

#ifndef NULL
#define NULL 0
#endif

#ifndef FALSE
#define FALSE 0
#endif /* endof */

#ifndef TRUE
#define TRUE 1
#endif /* endof */


#define DID_LEN      24
#define PASSCODE_LEN 10
#define PASSCODE_MAXLEN 32
#define PK_LEN       32
#define IP_LEN_MAX   4         /* eg:192.168.180.180 15byte.Should be ended with '\0' */
#define IP_LEN_MIN   0          /* eg:1.1.1.1 7bytes. Should be ended with '\0' */
#define SSID_LEN_MAX     32
#define WIFIKEY_LEN_MAX  64
#define CLOUD3NAME   10
#define FIRMWARELEN  32
#define MAC_LEN      32
#define APNAME_LEN   32
#define M2MSERVER_LEN 128
#define PHONECLIENTID 23
#define PRODUCTUUID_LEN   32
#define FEEDID_LEN 64
#define ACCESSKEY_LEN  64
#define MCU_P0_LEN 2
#define MCU_CMD_LEN 2
#define MCU_PROTOCOLVER_LEN 8
#define MCU_P0VER_LEN 8
#define MCU_HARDVER_LEN 8
#define MCU_SOFTVER_LEN 8
#define MCU_MCUATTR_LEN 8
#define WIFI_HARDVER_LEN 8
#define WIFI_SOFTVER_LEN 8

/* use for mqtt var len */
typedef struct _varc
{
    int8 var[4];//the value of mqtt
    int8 varcbty;// 1b-4b B=Bit,b=byte
} varc;

/*  */
typedef struct _jd_info
{
    int8 product_uuid[PRODUCTUUID_LEN+1];
    int8 feed_id[FEEDID_LEN+1];
    int8 access_key[ACCESSKEY_LEN+1];
    int8 ischanged;
    int8 tobeuploaded;
}jd_info;


/* OTA 升级类型 */
typedef enum OTATYPE_T
{
    OTATYPE_WIFI = 1,
    OTATYPE_MCU = 2,
    OTATYPE_INVALID
}OTATYPE;

typedef struct _packet
{
    int32 totalcap;  /* 全局缓存区的大小 4k+128byte */
    int32 remcap;    /* 数据包头的冗余长度 128byte */
    uint8 *allbuf;   /* 数据起始地址 */
    int32 bufcap;    /* 数据区域的大小 4K */

    uint32 type;     /* 消息体类型 */
    uint8 *phead;    /* 数据包头起始地址*/
    uint8 *ppayload;   /* 业务逻辑头起始地址*/
    uint8 *pend;     /* 数据结束地址 */
} packet,*ppacket;

typedef struct  _GAgent3Cloud
{
#pragma anon_unions
    union
    {
        jd_info jdinfo;
        int8 rsvd[1024];
    };
    int8 cloud3Name[CLOUD3NAME]; /*3rd cloud name*/
}GAgent3Cloud;

typedef struct
{
    int32 file_type;//wifi or mcu
    int32 mcu_type;//bin or hex
    uint32 file_len;
    int32 file_offset;
    int8 hard_ver[8+2];
    int8 soft_ver[8+2];
    int8 md5[32];
}fileInfo;

/* 需要保存在flash上的数据 */
typedef struct GAGENT_CONFIG
{
    uint32 magicNumber;
    uint32 flag;
    int8 wifipasscode[PASSCODE_MAXLEN+1]; /* gagent passcode +1 for printf*/
    int8 wifi_ssid[SSID_LEN_MAX+1]; /* WiFi AP SSID */
    int8 wifi_key[WIFIKEY_LEN_MAX+1]; /* AP key */
    int8 DID[DID_LEN]; /* Device, generate by server, unique for devices */
    int8 FirmwareVerLen[2];
    int8 FirmwareVer[FIRMWARELEN+1];

    int8 old_did[DID_LEN];
    int8 old_wifipasscode[PASSCODE_MAXLEN + 1];
    int8 old_productkey[PK_LEN + 1];    /* Add 1byte '\0' */
    uint8 m2m_ip[IP_LEN_MAX + 1];        /* Add 1byte '\0' */
    uint8 GServer_ip[IP_LEN_MAX + 1];    /* Add 1byte '\0' */
    //ip_addr_t GServer_ipaddr;
    int8 airkiss_value; //airkiss BC value to app.
    uint32 uiBaund;
    uint32 wifiFirmwareLen;
    uint8 hard_ver[MCU_HARDVER_LEN+1];
    uint8 master_softver[MCU_SOFTVER_LEN+1];
    uint8 slave_softver[MCU_SOFTVER_LEN+1];
    int8  MD5[32+1];
    uint8 otaStatus;
    uint8 boot_run_cnt;
    //int8 rsvd[256];
	union
    {
        int8 rsvd[256];
        struct
        {
            uint8 uiBaundIndex;
        }rsvd_st;
    }Ursvd;

    /** 3rd cloud **/
    GAgent3Cloud cloud3info;
}GAGENT_CONFIG_S, gconfig, *pgconfig;

typedef struct
{
    int32 sn;   /* sn of cmd */
    int32 fd;   /* >=0:fd;<0 broadcast */
    int32 remote_port;
    uint8 remote_ip[4];
    uint16 cmd;
}stLanAttrs_t;

typedef struct
{
    int32 sn;
    uint16 cmd;
    int8 phoneClientId[PHONECLIENTID+1];
}stCloudAttrs_t;

typedef struct
{
    uint32 type;
    stLanAttrs_t lanClient;
    stCloudAttrs_t cloudClient;
}stChannelAttrs_t;

typedef struct
{
   uint8 cmd;
   uint8 sn;
}localTxbufInfo;

/* MCU信息 */
typedef struct _XPG_MCU
{
    /* XPG_Ping_McuTick */
    //uint32 XPG_PingTime;
    uint32 oneShotTimeout;
    uint32 firmwareLen;
    uint32 uiBaund;
    uint16  passcodeEnableTime;
    uint16  passcodeTimeout;
    uint8 timeoutCnt;
    volatile uint8 isBusy;
    //int8 loseTime;
    /* 8+1('\0') for print32f. */
    uint8   protocol_ver[MCU_PROTOCOLVER_LEN+1];
    uint8   p0_ver[MCU_P0VER_LEN+1];
    uint8   hard_ver[MCU_HARDVER_LEN+1];
    uint8   soft_ver[MCU_SOFTVER_LEN+1];
    uint8   product_key[PK_LEN+1];
    uint8   mcu_attr[MCU_MCUATTR_LEN];
    int8    MD5[32+1];
    int8    mcu_upgradeflag;
    int8    mcu_firmware_type;
    localTxbufInfo TxbufInfo;
}XPG_MCU;

typedef struct _wifiStatus
{
   int8 wifiStrength; //WiFi信号强度.
}wifistatus;
typedef struct _waninfo
{
    stCloudAttrs_t srcAttrs;
    uint32 send2HttpLastTime;
    uint32 send2MqttLastTime;
    uint32 ReConnectMqttTime;
    uint32 ReConnectHttpTime;
    uint32 firstConnectHttpTime;
    uint32 RefreshIPLastTime;
    uint32 RefreshIPTime;
    int32 http_socketid;
    struct espconn *phttp_fd;
    struct espconn *pm2m_fd;
    int8  httpRecvFlag;
    int8  m2mRecvFlag;
    int32 m2m_socketid;
    int32 wanclient_num;

    uint16 CloudStatus;
    uint16 mqttstatus;
    uint16 mqttMsgsubid;
    uint16 Cloud3Status; //第三方云的状态机

    int8 Cloud3Flag;/* need to connect 3rd cloud flag */
    int8 AirLinkFlag;
    int8 phoneClientId[PHONECLIENTID+1];
    int8 cloudPingTime;
    int8 httpCloudPingTime;

    uint16 httpDataLen;
}WanInfo;
typedef struct _runtimeinfo3rd
{
    /* 3rd info*/
    uint32 JD_Post_lastTime;
}RunTimeInfo3rd;
typedef struct _localmodule
{
    uint8 timeoutCnt;
    uint32 oneShotTimeout;
    int32 uart_fd;

}localmodule;
typedef struct _ApHostList_str
{
     uint8 ssid[SSID_LEN_MAX+1];
     uint8 ApPower; /* 0-100 min:0;max:100 */
}ApHostList_str;
typedef struct _NetHostList_str
{
     uint8 ApNum;
     ApHostList_str* ApList;
}NetHostList_str;
/* 大文件传输结构体 */
typedef struct
{
    /* 同步标志 */
    /* 同时作为全局标志 */
    int using;
    /* 文件信息 */
    int filename;
    /* 总大小 */
    int totalsize;
    /* 收取大小 */
    int recvsize;
    /* 包大小 */
    int piecesize;
    /* 包数量 */
    int piececount;
    /* checksum值。具体形式待定 */
    int checksum;
    /* 当前所存放的包信息 */
    int currentpiece;
    /* 最后一次收到文件数据的时间。用来做超时判断 */
    int lastrecv;
    int lastpiece;
    /* 数据段。使用packet类型 */
    ppacket data;
}fdesc, *pfdesc;

typedef struct runtimeinfo_t
{
    uint32 clock;
    uint32 wifistatustime;
    uint32 updatestatusinterval;
    uint32 testLastTimeStamp;
    uint32 wifiLastScanTime;
    uint32 filelen;
    uint32 uartRxTime;
    uint32 uartTxTime;
    uint32 uartTxLen;
    uint32 resendTime;
    uint32 getInfoflag;
    os_timer_t uartRxTimer;
    os_timer_t AirLinkTimer;
    uint32 uartRxflag;

    uint16 GAgentStatus;/* gagentStatus */
    uint16 lastGAgentStatus;
    int16 loglevel;

    int8 status_ip_flag; /* when got the ip in station mode,value is 1. */
    uint8 logSwitch[2];

    uint8 scanWifiFlag;
    int8 firstStartUp;

    wifistatus devWifiStatus;

    WanInfo waninfo;
    localmodule local;

    ppacket Txbuf;/* send data to local buf */
    ppacket Rxbuf;/* receive data from local buf UdpRxbuf*/

    uint32 uart_recv_len;
    ppacket UartRxbuf;
    ppacket UartReTxbuf;//串口重发buf
    ppacket TcpRxbuf;
    //ppacket UdpRxbuf;
    ppacket HttpRxbuf;
    ppacket MqttRxbuf;
    uint8* pRxBuf;//底层串口接收buf
    uint8* pOtaBuf;//ota专用，ota完成后释放

    NetHostList_str aplist;
    RunTimeInfo3rd cloud3rd;
    stChannelAttrs_t stChannelAttrs;    /* the attrs of channel connected to GAgent */
    fdesc file;
    int local_send_ready_signal_flag;
    OTATYPE OTATypeflag;
    int8 onlinePushflag;
    int8 stopSendFlag;
    uint32 isAirlinkConfig;
    int8 webconfigflag;
    int8 bigdataUploadflag;
    int8 reqFirewareLenflag;
    int8 isOtaRunning;//正在进行OTA时不连接M2M
    int8 m2mDnsflag;//解析m2m结果
    uint32 mDevStartTime;
    fileInfo firmwareInfo;
    int32 bigDataRemainLen;
    uint32 lastRxOtaDataTime;

}runtimeinfo, *pruntimeinfo;

typedef struct modeinfo_t
{
    int32 m2m_Port;
    int8 m2m_SERVER[M2MSERVER_LEN+1];
    uint8 szmac[MAC_LEN+1];
    int8 ap_name[APNAME_LEN+1];
}modeinfo, *pmodeinfo;


typedef struct webserver_t
{
    int32 fd;
    int32 nums;
    u8 *buf;
}webserver, *pwebserver;

typedef struct lanserver_t
{
    int32 udpBroadCastServerFd;
    int32 udp3rdCloudFd;
    int32 udpServerFd;
    int32 tcpServerFd;
    int32 tcpWebConfigFd;
    int32 tcpClientNums;
    uint32 onboardingBroadCastTime;//config success broadcast Counter
    uint32 startupBroadCastTime;//first on ele broadcast Counter
    int32 broResourceNum;//public resource counter for broadcast
    /* the client attrs connected to Gagent while sending ctrl cmd */
    stLanAttrs_t srcAttrs;
    struct espconn *udpBroadcast;
    struct espconn *udpServer;
    struct espconn *tcpServer;
    struct espconn *webTcpServer;

    struct{
        int32 fd;
        int32 remote_port;
        int32 timeout;
        int32 isLogin;
        uint8 remote_ip[4];
        uint8 fd_isset;
    }tcpClient[8];
//    struct sockaddr_t addr;
}lanserver, *planserver;

typedef struct
{
    uint8 pk[32 + 2];
    uint8 did[32 + 2];
    uint8 hv[8 + 2];
    uint8 sv[8 + 2];
    uint8 check;
}trans_mcuotainfo, *ptrans_mcuotainfo;

/* global context, or gagent context */
typedef struct gagentcontext_t
{
    /* modeinfo mi; */
    modeinfo minfo;
    runtimeinfo rtinfo;
    /* mcuinfo mcui; */
    XPG_MCU mcu;
    /* webserver ws; */
    webserver ws;
    /* lanserver ls; */
    lanserver ls;
    trans_mcuotainfo tmcu;
    /* logman lm; */
    gconfig gc;
}gcontext, *pgcontext;

//typedef int32 (*filter_t)(BufMan*, u8*);
typedef int32 (*pfMasterMCU_ReciveData)( pgcontext pgc );
typedef int32  (*pfMasertMCU_SendData)( int serial_fd,unsigned char *buf,int buflen );
#endif
