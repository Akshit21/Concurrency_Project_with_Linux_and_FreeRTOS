#include "project.h"

void * task_read(void * fd);

int main(int argc, char *argv[]){
    int          file, count;
    msg_t        noise, motion;
    pthread_t    read;
    msg_packet_t packet;

    if ((file = open("/dev/ttyO4", O_RDWR | O_NOCTTY))<0)
    {
        perror("UART: Failed to open the file.\n");
        return -1;
    }
    struct termios options;
    tcgetattr(file, &options);
    options.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
    options.c_iflag = IGNPAR | ICRNL;
    tcflush(file, TCIFLUSH);
    tcsetattr(file, TCSANOW, &options);

    pthread_create(&read, NULL, task_read, (void *)&file);

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

    for( ; ; )
    {
        printf("Sending out a packet.\n");
        getTimestamp(noise.timestamp);
        packet = msg_create_messagePacket(&noise);
        if ((count = write(file, &packet, sizeof(packet)))<0)
        {
            perror("Failed to write to the output\n");
            return -1;
        }
        sleep(10);
    }
    return 0;
}

void * task_read(void * fd)
{
    int           n, maxfd, uartfd = *(int *)fd;
    msg_t         noise, motion, hb_res;
    req_packet_t  req;
    msg_packet_t  packet;
    struct pollfd monitor;

    monitor.fd = uartfd;
    monitor.events = POLLRDNORM;
    maxfd = 0;

    for( ; ; )
    {
	    poll(&monitor, maxfd+1, 1000);
        if(monitor.revents & ( POLLHUP | POLLRDNORM))
	    {
            memset(&req, 0, sizeof(req));
            memset(&packet, 0, sizeof(packet));
		    n = read(uartfd, &req, sizeof(req));
        	if(n == sizeof(req))
        	{

            	if(req_validate_messagePacket(&req))
            	{
                	switch (req.msg.type)
                	{
                    	case MSG_TYPE_SERVER_REQUEST_TO_CLIENT:
                    	    if(req.msg.dst == MSG_TIVA_NOISE_SENSING)
                    	    {
                                noise.id = 1;
                                noise.src = MSG_TIVA_NOISE_SENSING;
                                noise.dst = MSG_BBB_COMMAND;
                                noise.type = MSG_TYPE_CLIENT_RESPONSE_TO_SERVER;
                                getTimestamp(noise.timestamp);
                                sprintf(noise.content, "3dB");
                                packet = msg_create_messagePacket(&noise);
                                printf("Responded noice.\n");
                                write(uartfd, &packet, sizeof(packet));
                            }
                            else if(req.msg.dst == MSG_TIVA_MOTION_SENSING)
                            {
                                motion.id = 1;
                                motion.src = MSG_TIVA_MOTION_SENSING;
                                motion.dst = MSG_BBB_COMMAND;
                                motion.type = MSG_TYPE_CLIENT_RESPONSE_TO_SERVER;
                                getTimestamp(motion.timestamp);
                                sprintf(motion.content, "motion");
                                packet = msg_create_messagePacket(&motion);
                                printf("Responded motion.\n");
                                write(uartfd, &packet, sizeof(packet));
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
                            write(uartfd, &packet, sizeof(packet));
                            break;
                        default:;
                    }
                }
            }
	    }
    }
}
