#include "project.h"


void getTimestamp(char * buf)
{
    time_t t = time(NULL);
    sprintf(buf, "%ju", (uintmax_t)t);
}

int8_t serverLog(x_queue_t * q, char * str)
{
    int8_t ret = 0;
    msg_t log;
    log.id = 0;
    log.src = 0;
    log.dst = MSG_BBB_LOGGING;
    log.type = MSG_TYPE_LOG;
    getTimestamp(log.timestamp);
    sprintf(log.content, "%s", str);
    if(msg_send_LINUX_mq(q, &log)!=0)
    {
        ret = -1;
    }
    return ret;
}

void printPacket(msg_packet_t * packet)
{
    if(packet == NULL)
        return;
    else
    {
        printf("********** Packet Revealed *************");
        printf("header: %x\n", packet->header);
        printf("id: %d | src: %x | dst: %x | type: %x\n", packet->msg.id, packet->msg.src, packet->msg.dst, packet->msg.type);
        printf("time: %s\n", packet->msg.timestamp);
        printf("content: %s\n", packet->msg.content);
        printf("****************************************");
    }
    return;
}
