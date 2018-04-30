#include "project.h"

int             i;
sem_t           mr_sem, tx_sem, lg_sem, cm_sem;
sem_t           mr_hb_sem, tx_hb_sem, rx_hb_sem, lg_hb_sem;
uint8_t         heartbeat = 0xFF;
/* Message queues for the MsgRouter and Logger task */
x_queue_t       router_q, logger_q;

/* A client table that tracks the client connection status */
struct pollfd   client[OPEN_MAX];

void errorHandling(uint8_t level, char * errMsg)
{
    msg_t errlog;

    errlog.id = 0;
    errlog.dst = MSG_BBB_LOGGING;
    errlog.type = MSG_TYPE_LOG;
    switch (level) {
        case 0:
            /* Log the error to server log file */
            blinkLED();
            sprintf(errlog.content, "%s", errMsg);
            getTimestamp(errlog.timestamp);
            if(msg_send_LINUX_mq(&router_q, &errlog)==0)
                sem_post(&mr_sem);
            break;
        case 1:
            /* Print the fatal error, turn on the LED and hang */
            ledOn(ledPath);
            printf("%s\n", errMsg);
            while(1);
            break;
        case 2:
            msg_destroy_LINUX_mq(&router_q);
            msg_destroy_LINUX_mq(&logger_q);
            ledOn(ledPath);
            printf("%s\n", errMsg);
            while(1);
            break;
        default:;
    }
}

int main(int argc, char const *argv[])
{
    int                 i;
#ifdef SOCKET
    int                 listenfd;
    struct sockaddr_in  server_addr;
#else
    int                 uartfd[2];
    char                port_name[15];
#endif
    pthread_t           msgrouter, tx, rx, logger, command, hb;

    /* Initialize semaphores */
    if((sem_init(&mr_sem,0,0)!=0) || (sem_init(&tx_sem,0,0)!=0) ||
       (sem_init(&lg_sem,0,0)!=0) || (sem_init(&cm_sem,0,0)!=0))
    {
        //perror("[ERROR] [main] sem_init() failed.\n");
        errorHandling(1, "[FATAL!] Failed to initialize semaphores.");
    }

    if((sem_init(&mr_hb_sem,0,0)!=0) || (sem_init(&tx_hb_sem,0,0)!=0) ||
       (sem_init(&lg_hb_sem,0,0)!=0) || (sem_init(&rx_hb_sem,0,0)!=0))
    {
        //perror("[ERROR] [main] sem_init() failed.\n");
        errorHandling(1, "[FATAL] Failed to initialize heartbeat semaphores.");
    }

    /* Create queues */
    if((msg_create_LINUX_mq("/router", 10, &router_q)!=0) ||
       (msg_create_LINUX_mq("/logger", 10, &logger_q)!=0))
    {
        //perror("[ERROR] [main] Failed to initialize queues.\n");
        errorHandling(1, "[FATAL] Failed to initialize queues.");
    }

    /* Initialize client table */
    for(i=0; i<OPEN_MAX; i++)
    {
        client[i].fd = -1;
    }

#ifdef SOCKET
    /* Create a end-point for socket communication */
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0))==-1)
    {
        //perror("[ERROR] [main] socket() failed.\n");
        errorHandling(1, "[FATAL] Failed to create socket.");
    }
    else
        DEBUG(("[main] Created a socket end-point.\n"));

    /* Configure the server address */
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(SERVER_PORT);

    /* Assign a name to the socket */
    if(bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr))!=0)
    {
        //perror("[ERROR] [main] bind() failed.\n");
        errorHandling(1, "[FATAL] Failed to assign name to socket.");
    }
    else
        DEBUG(("[main] Assigned a name to the socket.\n"));

    /* Listen for connection on the socket */
    if(listen(listenfd, 1024)!=0)
    {
        //perror("[ERROR] [main] listen() failed.\n");
        errorHandling(1, "[FATAL] Failed to listen the socket.");
    }
    else
        DEBUG(("[main] Marked the socket as passive.\n"));

    /* Initialize the client tracking table */
    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;
#else
    for(i=1;i<=1;i++)
    {
        sprintf(port_name, "/dev/ttyO2");
        if((uartfd[i-1] = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
        {
            //perror("[ERROR] [main] Failed to open TTY port \n");
            errorHandling(2, "[FATAL] Failed to open TTY ports.");
        }
        else
            DEBUG(("[main] TTY port %d opened.\n", i));

        /***** Set up the communication options *****/
        struct termios options;
        tcgetattr(uartfd[i-1], &options);
        /* 9600 baud, 8-bit, enable receiver, no modem control lines */
        options.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
        /* Ignore parity errors, CR for new line */
        options.c_iflag = IGNPAR | ICRNL;
        /* Discard file information not transmitted */
        tcflush(uartfd[i-1], TCIFLUSH);
        /* Changes occur immediately */
        tcsetattr(uartfd[i-1], TCSANOW, &options);

        client[i].fd = uartfd[i-1];
        client[i].events = POLLRDNORM;
    }
#endif

    /* Create all other threads */
    if((pthread_create(&rx, NULL, RX, NULL) != 0) ||
       (pthread_create(&tx, NULL, task_Tx, NULL) != 0)             ||
       (pthread_create(&logger, NULL, task_Logger, NULL) != 0)     ||
       (pthread_create(&command, NULL, task_Command, NULL) != 0)   ||
       (pthread_create(&hb, NULL, task_HB, NULL) != 0)             ||
       (pthread_create(&msgrouter, NULL, task_MsgRouter, NULL) != 0))
    {
        //perror("[ERROR] [main] Failed to create tasks.\n");
        errorHandling(2, "[FATAL] Failed to create tasks.");
    }

    serverLog(&router_q, "[INFO] Server initialized successfully.");
    sem_post(&mr_sem);
  /* Perform local heartbeat check */

  /* Perform client heartbeat check */

  pthread_join(rx,  NULL);
  pthread_join(tx,        NULL);
  pthread_join(logger,    NULL);
  pthread_join(command,   NULL);
  pthread_join(msgrouter, NULL);
  return 0;
}
