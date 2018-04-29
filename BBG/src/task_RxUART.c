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
    int     i, maxfd, uartfd[2], num_ready;
    char    port_name[15];
    ssize_t n;
    msg_packet_t rxbuf;
	client[2].fd = -1;
    /* Open all available TTY file */
    for(i=0;i<2;i++)
    {
        sprintf(port_name, "/dev/ttyO%d", i+1);
        if((uartfd[i] = open(port_name, O_RDWR | O_NOCTTY )) < 0)
        {
            perror("[ERROR] [task_RxUART] Failed to open TTY port \n");
        }
        else
            DEBUG(("[task_RxUART] TTY port %d opened.\n", i+1));

        /***** Set up the communication options *****/
        struct termios options;
        tcgetattr(uartfd[i], &options);
        /* 9600 baud, 8-bit, enable receiver, no modem control lines */
        options.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
        /* Ignore parity errors, CR for new line */
        options.c_iflag = IGNPAR | ICRNL;
        /* Discard file information not transmitted */
        tcflush(uartfd[i], TCIFLUSH);
        /* Changes occur immediately */
        tcsetattr(uartfd[i], TCSANOW, &options);

        client[i].fd = uartfd[i];
        client[i].events = POLLRDNORM;
    }
    maxfd = 2;

    for( ; ; )
    {
        /* Poll for incoming data */
        num_ready = poll(client, maxfd+1, 1000);

        if(num_ready>0)
        {
            DEBUG(("[task_RxUART] Active UART activities.\n"));
            for(i=0;i<2;i++)
            {

                if(client[i].revents & POLLRDNORM)
                {
                    /* Read from client */
                    if((n=read(uartfd[i], &rxbuf, sizeof(rxbuf))) ==sizeof(rxbuf))
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

            DEBUG(("[task_RxUART] Responded to heartbeat request.\n"));
        }
    }
}
