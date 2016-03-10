#ifndef _LANUDP_H_
#define _LANUDP_H_

void Lan_udpDataHandle(pgcontext pgc, ppacket prxBuf, int32 len);
void lan_udp_recv(void *arg, char *pusrdata, unsigned short length);
void lan_udp_sentcb(void *arg);


#endif
