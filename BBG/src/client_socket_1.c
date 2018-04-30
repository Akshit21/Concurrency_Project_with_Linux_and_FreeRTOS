#include "project.h"

void *task_read(void * fd);

/* Client Socket */
int main(int argc, char const *argv[])
{
    int                socketfd, flag=0;
    msg_t              noise, motion;
    msg_packet_t       packet;
    struct sockaddr_in serv_addr;

    /* Create a socket */
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd <0)
    {
        perror("Failed to create socket.\n");
        exit(EXIT_FAILURE);
    }

    /* Server Port attributes*/
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(SERVER_PORT);

    noise.id = 1;
    noise.src = MSG_TIVA_NOISE_SENSING;
    noise.dst = MSG_BBB_LOGGING;
    noise.type = MSG_TYPE_LOG;
    sprintf(noise.content, "3dB");

    motion.id = 1;
    motion.src = MSG_TIVA_MOTION_SENSING;
    motion.dst = MSG_BBB_LOGGING;
    motion.type = MSG_TYPE_LOG;
    sprintf(motion.content, "motion");

    /* Connect to the server */
    if(connect(socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))==0)
  	{
        setsockopt(socketfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

        /* Create a reading task */
        pthread_t read;
        pthread_create(&read, NULL, task_read, (void*)&socketfd);

        for (int i = 0; i<10; i++)
        {
            printf("Sending out a packet.\n");
            /* Send the message request */
            getTimestamp(noise.timestamp);
            packet = msg_create_messagePacket(&noise);
            write(socketfd, &packet, sizeof(packet));
            sleep(10);
        }

        printf("Closing the socket now.\n");
        if(close(socketfd)!=0)
        {
            perror("Failed to close socket connection.\n");
        }

  	}
  	else
  	{
    	perror("Failed to connect.\n");
    	exit(EXIT_FAILURE);
  	}

    exit(EXIT_SUCCESS);
}

void *task_read(void * fd)
{
    int          n, socketfd = *(int*)fd;
    msg_t        noise, motion, hb_res;
    req_packet_t req;
    msg_packet_t packet;

    for( ; ; )
    {
        memset(&req, 0, sizeof(req));
        memset(&req, 0 ,sizeof(req));
        n = read(socketfd, &req, sizeof(req));
        //printf("received %d bytes | %d\n", n, sizeof(packet));
	    if(n==sizeof(req))
        {
            printf("received %d.\n", req.header);
            //printPacket(&req);
            if(req_validate_messagePacket(&req))
            {
                switch (req.msg.type)
                {
                    case MSG_TYPE_SERVER_REQUEST_TO_CLIENT:
                        if(req.msg.dst == MSG_TIVA_NOISE_SENSING)
                        {
                            printf("noise\n");
                            noise.id = 1;
                            noise.src = MSG_TIVA_NOISE_SENSING;
                            noise.dst = MSG_BBB_COMMAND;
                            noise.type = MSG_TYPE_CLIENT_RESPONSE_TO_SERVER;
                            getTimestamp(noise.timestamp);
                            sprintf(noise.content, "3dB");
                            packet = msg_create_messagePacket(&noise);
                            write(socketfd, &packet, sizeof(packet));
                        }
                        else if(req.msg.dst == MSG_TIVA_MOTION_SENSING)
                        {
                            printf("motion\n");
                            motion.id = 1;
                            motion.src = MSG_TIVA_MOTION_SENSING;
                            motion.dst = MSG_BBB_COMMAND;
                            motion.type = MSG_TYPE_CLIENT_RESPONSE_TO_SERVER;
                            getTimestamp(motion.timestamp);
                            sprintf(motion.content, "motion");
                            packet = msg_create_messagePacket(&motion);
                            write(socketfd, &packet, sizeof(packet));
                        }
                        break;
                    case MSG_TYPE_CLIENT_HEARTBEAT_REQUEST:
                        hb_res.id = 1;
                        hb_res.src = MSG_TIVA_SOCKET;
                        hb_res.dst = MSG_BBB_SOCKET;
                        hb_res.type = MSG_TYPE_CLIENT_HEARTBEAT_RESPONSE;
                        getTimestamp(hb_res.timestamp);
                        hb_res.content[0] = 1;
                        hb_res.content[1] = 1;
                        packet = msg_create_messagePacket(&hb_res);
                        write(socketfd, &packet, sizeof(packet));
                        break;
                    default:;
                }
            }
        }
    }
}
