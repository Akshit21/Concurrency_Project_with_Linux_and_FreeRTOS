#include "project.h"

void * task_RxSocket(void * param)
{
    int             i, maxfd, connfd, socketfd, num_ready;
    char            log[60];
    ssize_t         n;
    socklen_t       clilen;
    msg_packet_t    rxbuf;
    struct sockaddr_in client_addr;

    maxfd = 0;

    for( ; ; )
    {
        /* Poll for connection event and incoming data */
        num_ready = poll(client, maxfd+1, 1000);

        if(num_ready>0)
        {
            DEBUG(("[task_RxSocket] Active socket activities.\n"));
            if(client[0].revents & POLLRDNORM)
            {
                /* New client connection */
                clilen = sizeof(client_addr);
                connfd = accept(client[0].fd, (struct sockaddr*)&client_addr, &clilen);
                DEBUG(("[task_RxSocket] New Client connected.\n"));

                /* Update the client tracking table */
                for(i=1; i<OPEN_MAX; i++)
                {
                    if(client[i].fd<0)
                    {
                        client[i].fd = connfd;
                        client[i].events = POLLRDNORM;
                        break;
                    }
                }
                if(i==OPEN_MAX)
                    DEBUG(("[task_RxSocket] Too many clients.\n"));

                if(i>maxfd)
                    maxfd = i;

                if(--num_ready <= 0)
                    continue;
            }

            for(i=1; i<OPEN_MAX; i++)
            {
                /* Check all connected clients for data */
                if((socketfd = client[i].fd) < 0)
                    continue;
                if(client[i].revents & ( POLLRDNORM | POLLERR))
                {
                    /* Read from client */
                    if((n = read(socketfd, &rxbuf, sizeof(rxbuf))) < 0)
                    {
                        if(errno==ECONNRESET)
                        {
                            /* Connection reset by the client */
                            DEBUG(("[task_RxSocket] Client[%d] aborted connection.\n", i));
                            sprintf(log, "[task_RxSocket] Client[%d] aborted connection.", i);
                            serverLog(&router_q, log);
                            close(socketfd);
                            client[i].fd = -1;
                        }
                        else
                        {
                            errorHandling(0, "[ERROR] [task_RxSocket] read() failed.");
                            //perror("[ERROR] [task_RxSocket] read() failed.\n");
                        }
                    }
                    else if (n == 0)
                    {
                        /* Connection closed by the client */
                        DEBUG(("[task_RxSocket] Client[%d] closed connection.\n", i));
                        sprintf(log, "[task_RxSocket] Client[%d] close connection.", i);
                        serverLog(&router_q, log);
                        close(socketfd);
                        client[i].fd = -1;
                    }
                    else
                    {
                        DEBUG(("[task_RxSocket] received packet from client[%d].\n", i));
                        /* Validate the packet */
                        if(msg_validate_messagePacket(&rxbuf))
                        {
                            DEBUG(("[task_RxSocket] Client packet validated.\n"));

                            /* Enqueue the msg */
                            if(msg_send_LINUX_mq(&router_q, &rxbuf.msg) != 0)
                            {
                                errorHandling(0, "[ERROR] [task_RxSocket] Failed to enqueue client message.");
                                //perror("[ERROR] [task_RxSocket] Failed to enqueue client message.\n");
                            }
                            else
                            {
                                DEBUG(("[task_RxSocket] client[%d] message enqueued.\n", i));
                                sem_post(&mr_sem);
                            }
                        }
                        else
                            DEBUG(("[task_RxSocket] Invalid client packet.\n"));
                    }

                    if(--num_ready <= 0)
                        /* No more readable file descriptors */
                        break;
                    }
            }
        }

        if(sem_trywait(&rx_hb_sem)==0)
        {
            DEBUG(("[task_RxSocket] Received heartbeat request.\n"));
            /* Response to heartbeat request */
            heartbeat &= ~RX_INACTIVE;
            DEBUG(("[task_RxSocket] Responded to heartbeat request.\n"));
        }
    }
}
