#include "project.h"

void * task_RxSocket(void * param)
{
    int             i, maxfd, listenfd, connfd, socketfd, num_ready;
    ssize_t         n;
    msg_packet_t    rxbuf;
    struct sockaddr server_addr, client_addr;

    /* Create a end-point for socket communication */
    if((listenfd = Socket(AF_INET, SOCK_STREAM, 0))==-1)
    {
        perror("[ERROR] [task_RxSocket] socket() failed.\n");
    }
    else
        DEBUG(("[task_RxSocket] Created a socket end-point.\n"));

    /* Configure the server address */
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(SERV_PORT);

    /* Assign a name to the socket */
    if(bind(listenfd, &server_addr, sizeof(server_addr))!=0)
    {
        perror("[ERROR] [task_RxSocket] bind() failed.\n");
    }
    else
        DEBUG(("[task_RxSocket] Assigned a name to the socket.\n"));

    /* Listen for connection on the socket */
    if(listen(listenfd, 1024)!=0)
    {
        perror("[ERROR] [task_RxSocket] listen() failed.\n");
    }
    else
        DEBUG(("[task_RxSocket] Marked the socket as passive.\n"));

    /* Initialize the client tracking table */
    client[0].fd = listenfd;
    client[0].event = POLLRDNORM;
    for(i=1; i<OPEN_MAX; i++)
    {
        client[i].fd = -1;
    }
    maxfd = 0;

    for( ; ; )
    {
        /* Poll for connection event and incoming data */
        num_ready = poll(client, maxfd+1, 1000);

        if(num_ready>0)
        {
            if(client[0].revents & POLLRDNORM)
            {
                /* New client connection */
                connfd = accpet(listenfd, &client_addr, sizeof(client_addr));
                DEBUG(("[task_RxSocket] New Client connected.\n"));

                /* Update the client tracking table */
                for(i=1; i<OPEN_MAX; i++)
                {
                    if(client[i].fd<0)
                    {
                        client[i].fd = connfd;
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
                if(client[i].revent & (POLLRDNORM | POLLER))
                {
                    /* Read from client */
                    if((n = read(socketfd, &rxbuf, sizeof(rxbuf))) < 0)
                    {
                        if(errno==ECONNRESET)
                        {
                            /* Connection reset by the client */
                            DEBUG(("[task_RxSocket] Client[%d] aborted connection.\n", i));
                            close(socketfd);
                            client[i].fd = -1;
                        }
                        else
                        {
                            perror("[ERROR] [task_RxSocket] read() failed.\n");
                        }
                    }
                    else if (n == 0)
                    {
                        /* Connection closed by the client */
                        DEBUG(("[task_RxSocket] Client[%d] closed connection.\n", i));
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
                                perror("[ERROR] [task_RxSocket] Failed to enqueue client
                                        message.\n");
                            }
                            else
                                DEBUG(("[task_RxSocket] client[%d] message enqueued.\n", i));
                        }
                    }

                    if(--num_ready <= 0)
                        /* No more readable file descriptors */
                        break;
                    }
            }
        }

        if(sem_trywait(&rx_hb_sem)==0)
        {
            DEBUG(("[task_Serial] Received heartbeat request.\n"));
            /* Response to heartbeat request */

            DEBUG(("[task_Serial] Responded to heartbeat request.\n"));
        }
    }
}
