/* reference
https://github.com/derekmolloy/exploringBB/blob/master/chp08/uart/uartC/uart.c */

#include "project.h"
/**
 * @brief Client communication handling task
 *
 * This task is a backup option to handle the messaging between the server and
 * the client when the network channel is not available. This task keeps listening
 * the serial port for any incoming data package. If there is, it checks for either
 * a dedicated client communication handling task has been created for this client,
 * if so, it passes the message to that handling task; otherwise, it will create a
 * handling task for it and then pass the message.
 *
 * This task is also responsible for sending out user requests to the client.
 *
 * @param client_id - unique client id for different communication handling.
 *
 * @return none.
 */
void * task_RxUART(void * param)
{
    int          i, maxfd, num_ready;
    ssize_t      n;
    msg_packet_t rxbuf;

    client[3].fd = -1;
    maxfd = 2;

    for( ; ; )
    {
        /* Poll for incoming data */
        num_ready = poll(client, maxfd+1, 1000);

        if(num_ready>0)
        {
            DEBUG(("[task_RxUART] Active UART activities.\n"));
            for(i=1;i<=2;i++)
            {

                if(client[i].revents & POLLRDNORM)
                {
                    /* Read from client */
                    if((n=read(client[i].fd, &rxbuf, sizeof(rxbuf))) ==sizeof(rxbuf))
                    {
                        DEBUG(("[task_RxUART] Received packet from client[%d].\n", i));
                        if(msg_validate_messagePacket(&rxbuf))
                        {
                            DEBUG(("[task_RxUART] Client packet validated.\n"));
                            /* Enqueue the msg */
                            if(msg_send_LINUX_mq(&router_q, &rxbuf.msg) != 0)
                            {
                                perror("[ERROR] [task_RxUART] Failed to enqueue client message.\n");

			                }
                            else
			                {
				                DEBUG(("[task_RxUART] Client[%d] message enqueued.\n", i));
			                    sem_post(&mr_sem);
			                }
			            }
                    }
                }
            }
        }

        if(sem_trywait(&rx_hb_sem)==0)
        {
            DEBUG(("[task_RxUART] Received heartbeat request.\n"));
            /* Response to heartbeat request */
            heartbeat &= ~RX_INACTIVE;
            DEBUG(("[task_RxUART] Responded to heartbeat request.\n"));
        }
    }
}
