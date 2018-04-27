#include "project.h"

msg_t txbuf;

void * task_Tx(void * param)
{
    int8_t          retries;
    msg_packet_t    txPacket;

    struct timespec wait_time =
                    {
                        .tv_sec = 1;
                        .tv_nsec = 0;
                    }

    for( ; ; )
    {
        if(sem_timedwait(&tx_sem, &wait_time) == 0)
        {
            /* Send message to client */
            txPacket = msg_create_messagePacket(&txbuf);
            for(retries=3; retries>0; retries--)
            {
                if(write(client[txPacket.msg.id].fd, (void *)txPacket,
                         sizeof(txPacket)) == sizeof(txPacket))
                    break;
            }
            if(retries = 0)
            {
                perror("[ERROR] [task_Tx] write() failed.\n");
            }
            else
                DEBUG(("[task_Tx] Request to client[%d] has been sent.\n",
                        txPacket.msg.id));
        }

        if(sem_trywait(&tx_hb_sem)==0)
        {
            DEBUG(("[task_Tx] Received heartbeat request.\n"));
            /* Response to heartbeat request */

            DEBUG(("[task_Tx] Responded to heartbeat request.\n"));
        }
    }
}
