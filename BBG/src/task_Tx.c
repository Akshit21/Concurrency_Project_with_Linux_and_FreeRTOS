#include "project.h"


req_t txbuf = {0};

void * task_Tx(void * param)
{
    int8_t          retries;
    req_packet_t    txPacket;
    struct timespec wait_time;

    for( ; ; )
    {
        if (clock_gettime(CLOCK_REALTIME, &wait_time) == -1)
        {
            perror("clock_gettime\n");
        }
        wait_time.tv_sec += 1;
        if(sem_timedwait(&tx_sem, &wait_time) == 0)
        {
            /* Send message to client */
            txPacket = msg_create_requestPacket(&txbuf);
            for(retries=3; retries>0; retries--)
            {
                if(write(client[txPacket.msg.id].fd, (void *)&txPacket,
                         sizeof(txPacket)) == sizeof(txPacket))
                    break;
            }
            if(retries == 0)
            {
                errorHandling(0, "[ERROR] [task_Tx] write() failed.");
                //perror("[ERROR] [task_Tx] write() failed.\n");
            }
            else
                DEBUG(("[task_Tx] Request to client[%d] has been sent.\n",
                        txPacket.msg.id));
        }

        if(sem_trywait(&tx_hb_sem)==0)
        {
            DEBUG(("[task_Tx] Received heartbeat request.\n"));
            /* Response to heartbeat request */
            heartbeat &= ~TX_INACTIVE;
            DEBUG(("[task_Tx] Responded to heartbeat request.\n"));
        }
    }
}
