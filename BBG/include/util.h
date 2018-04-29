#ifndef __UTIL_H__
#define __UTIL_H__

void getTimestamp(char * buf);
int8_t serverLog(x_queue_t *q, char * str);
void printPacket(msg_packet_t * packet);
#endif
