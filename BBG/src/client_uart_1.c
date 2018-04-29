#include "project.h"

void * task_read(void * fd);

int main(int argc, char *argv[]){
    int          file, count;
    msg_t        noise, motion;
    pthread_t    read;
    msg_packet_t packet;

    if ((file = open("/dev/ttyO1", O_RDWR | O_NOCTTY | O_NDELAY))<0)
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

    // send the string plus the null character
    getTimestamp(noise.timestamp);
    packet = msg_create_messagePacket(&noise);
    if ((count = write(file, &packet, sizeof(packet)))<0)
    {
        perror("Failed to write to the output\n");
        return -1;
    }
    while(1);
    // usleep(100000);
    // unsigned char receive[100];
    // if ((count = read(file, (void*)receive, 100))<0)
    // {
    //     perror("Failed to read from the input\n");
    //     return -1;
    // }
    // if (count==0)
    //     printf("There was no data available to read!\n");
    // else
    // {
    //     receive[count]=0;  //There is no null character sent by the Arduino
    //     printf("The following was read in [%d]: %s\n",count,receive);
    // }
    // close(file);
    return 0;
}

void * task_read(void * fd)
{
    int n, uartfd = *(int *)fd;
    msg_packet_t packet;
    msg_t noise, motion;
    printf("read task.\n");
    for( ; ; )
    {
        n = read(uartfd, &packet, sizeof(packet));
        printf("received %d bytes | %d\n", n, sizeof(packet));
        if(n)
        {
            printf("header: %x\n", packet.header);
            printf("id: %d | src: %x | dst: %x | type: %x\n", packet.msg.id, packet.msg.src, packet.msg.dst, packet.msg.type);
            printf("time: %s\n", packet.msg.timestamp);
            printf("content: %s\n", packet.msg.content);
            if(msg_validate_messagePacket(&packet))
            {
                switch (packet.msg.type)
                {
                    case MSG_TYPE_SERVER_REQUEST_TO_CLIENT:
                        if(packet.msg.dst == MSG_TIVA_NOISE_SENSING)
                        {
                            noise.id = 1;
                            noise.src = MSG_TIVA_NOISE_SENSING;
                            noise.dst = MSG_BBB_COMMAND;
                            noise.type = MSG_TYPE_CLIENT_RESPONSE_TO_SERVER;
                            getTimestamp(noise.timestamp);
                            sprintf(noise.content, "3dB");
                            packet = msg_create_messagePacket(&noise);
                            write(uartfd, &packet, sizeof(packet));
                        }
                        else if(packet.msg.dst == MSG_TIVA_MOTION_SENSING)
                        {
                            motion.id = 1;
                            motion.src = MSG_TIVA_MOTION_SENSING;
                            motion.dst = MSG_BBB_COMMAND;
                            motion.type = MSG_TYPE_CLIENT_RESPONSE_TO_SERVER;
                            getTimestamp(motion.timestamp);
                            sprintf(motion.content, "motion");
                            packet = msg_create_messagePacket(&motion);
                            write(uartfd, &packet, sizeof(packet));
                        }
                        break;
                    case MSG_TYPE_CLIENT_HEARTBEAT_REQUEST:
                        break;
                    default:;
                }
            }
        }
    }
}
