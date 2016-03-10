#include "gagent.h"
#include "mqttbase.h"
#include "mqttlib.h"

int ICACHE_FLASH_ATTR
send_packet(int socketid, const void* buf, unsigned int count)
{
    int ret;
    ret = espconn_sent(pgContextData->rtinfo.waninfo.pm2m_fd, buf, count);
    if( 0 == ret )
    {
        GAgent_Printf(GAGENT_INFO,"MQTT Send packet");
        return count;
    }
    else
    {
        GAgent_Printf(GAGENT_INFO,"MQTT Send packet failed,error code = %d\n",ret);
        return RET_FAILED;
    }
}

/*************************************************
 *
 *      Function                : check mqtt connect
 *      packet_bufferBUF: MQTT receive data
 *      packet_length   :   the packet length
 *      return                  :   success 0,error bigger than 0;
 *   add by Ale lin  2014-03-27
 *
 ***************************************************/
 //Use?
int ICACHE_FLASH_ATTR
check_mqttconnect( uint8_t *packet_buffer,int packet_length )
{
    if(packet_length < 0)
    {
        GAgent_Printf(GAGENT_INFO,"Error on read packet!");
        return 1;
    }

    if(packet_buffer[3] != 0x00)
    {
        GAgent_Printf(GAGENT_INFO,"check_mqttconnect CONNACK failed![%d]", packet_buffer[3]);
        return 3;
    }

    return 0;
}

/*************************************************
 *
 *      Function : check mqtt witch qos 1
 *      packet_bufferBUF: MQTT receive data
 *      packet_length : the packet length
 *   add by Ale lin  2014-04-03
 *
 ***************************************************/
int ICACHE_FLASH_ATTR
check_mqttpushqos1( uint8_t *packet_bufferBUF,int packet_length, uint16_t msg_id )
{
    uint16_t msg_id_rcv;
    uint8_t *packet_buffer=NULL;
    packet_buffer = ( uint8_t* )os_malloc(packet_length);
    os_memset( packet_buffer,0,packet_length);
    os_memcpy( packet_buffer,packet_bufferBUF,packet_length);
    if(packet_length < 0)
    {
        GAgent_Printf(GAGENT_INFO,"Error on read packet!");
        os_free(packet_buffer);
        return -1;
    }
    if(MQTTParseMessageType(packet_buffer) != MQTT_MSG_PUBACK)
    {
        GAgent_Printf(GAGENT_INFO,"PUBACK expected!");
        os_free(packet_buffer);
        return -1;
    }
    msg_id_rcv = mqtt_parse_msg_id(packet_buffer);
    if(msg_id != msg_id_rcv)
    {
        GAgent_Printf(GAGENT_INFO," message id was expected, but message id was found!");
        os_free(packet_buffer);
        return -1;
    }
    os_free(packet_buffer);
    GAgent_Printf(GAGENT_INFO,"check_mqttpushqos1 OK");

    return 1;
}
int ICACHE_FLASH_ATTR
check_mqtt_subscribe( uint8_t *packet_bufferBUF,int packet_length, uint16_t msg_id )
{
    uint16_t msg_id_rcv;
    uint8_t *packet_buffer=NULL;
    packet_buffer = ( uint8_t* )os_malloc(packet_length);
    if( packet_buffer==NULL )
    {
        os_free(packet_buffer);
        return -1;
    }
    os_memset( packet_buffer,0,packet_length);
    os_memcpy( packet_buffer,packet_bufferBUF,packet_length);
    if(packet_length < 0)
    {
        GAgent_Printf(GAGENT_INFO,"Error on read packet!");
        os_free(packet_buffer);
        return -1;
    }

    if(MQTTParseMessageType(packet_buffer) != MQTT_MSG_SUBACK)
    {
        GAgent_Printf(GAGENT_INFO,"SUBACK expected!");
        os_free(packet_buffer);
        return -1;
    }

    msg_id_rcv = mqtt_parse_msg_id(packet_buffer);
    if(msg_id != msg_id_rcv)
    {
        GAgent_Printf(GAGENT_INFO," message id was expected, but message id was found!");
        os_free(packet_buffer);
        return -1;
    }
    os_free(packet_buffer);

    return 1;
}

/**********************************************************
 *
 *          Function    : PubMsg()
 *          broker      : mqtt_broker_handle_t
 *          topic       :   sub topic
 *          Payload     :   msg payload
 *          PayLen      : payload length
 *          flag        : 0 qos 0 1 qos 1
 *          return      : 0 pub topic success 1 pub topic fail.
 *          Add by Alex lin     2014-04-03
 *
 ***********************************************************/
int ICACHE_FLASH_ATTR
PubMsg( mqtt_broker_handle_t* broker, ppacket pp, int flag, int totallen)
{
    uint16_t msg_id;
    int pubFlag=0;

    switch(flag)
    {
    case 0:
        pubFlag = XPGmqtt_publish(broker, pp, 0);
        break;
    case 1:
        pubFlag = XPGmqtt_publish_with_qos(broker, pp, 0, 1, 0, &msg_id);
        break;
    case 2:
        pubFlag = XPGmqtt_publish_with_qos( broker, pp, totallen, 0, 0xFF, &msg_id);
        break;
    default:
        pubFlag=1;
        break;
    }

    return pubFlag;
}
