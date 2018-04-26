/* reference */
https://github.com/derekmolloy/exploringBB/blob/master/chp08/uart/uartC/uart.c

#include "project.h"

sem_t ser_hb_sem, ser_req_sem;

static void checkForClientMsg(int32_t serial_file);
static void parseMessage(msg_t * msg);

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
void * task_Serial(void * param)
{
    int32_t serial_file;

    /***** Open a serial file *****/
    if((serial_file = open("/dev/tty04", O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
    {
        perror("[ERROR] [Task_Serial] Failed to open the file.\n");
    }
    else
    {
        DEBUG(("[Task_Serial] Serial port opened.\n"));
    }

    /***** Initial the semaphore for heartbeat and *****/
    if((sem_init(&ser_hb_sem, 0, 0)!=0) || (sem_init(&ser_req_sem, 0, 0)!=0))
    {
        perror("[ERROR] [Task_Serial] Failed to initialie semaphores.\n");
    }
    else
    {
        DEBUG(("[Task_Serial] Semaphores initialized.\n"));
    }

    /***** Set up the communication options *****/
    struct termios options;
    tcgetattr(serial_file, &options);
    /* 9600 baud, 8-bit, enable receiver, no modem control lines */
    options.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
    /* Ignore parity errors, CR for new line */
    options.c_iflag = IGNPAR | ICRNL;
    /* Discard file information not transmitted */
    tcflush(serial_file, TCIFLUSH);
    /* Changes occur immediately */
    tcsetattr(serial_file, TCSANOW, &options);
    DEBUG(("[Task_Serial] Serial port attributes configured."));

    /***** Sub task 1: Handling heartbeat *****/
    if(sem_trywait(&ser_hb_sem)==0)
    {
        DEBUG(("[Task_Serial] Received heartbeat request.\n"));
        /* Response to heartbeat request */

        DEBUG(("[Task_Serial] Responded to heartbeat request.\n"));
    }

    /***** Sub task 2: Check if there is message to be sent out to clients *****/
    if(sem_trywait(&ser_req_sem) == 0)
    {
        DEBUG(("[Task_Serial] User request is sent out to the client.\n"));
    }

    /***** Sub task 3: Detect and process incoming message package from client *****/
    checkForClientMsg(serial_file);

    usleep(10);
}

/**
 * @brief Detect and process incoming message package from client
 *
 * If client message is detected, decode the message and parse it to the corresponding
 * client comm handling task.
 *
 * @param   serial_file the file handle of the serial port
 *
 * @return  none,
 */
static void checkForClientMsg(int32_t serial_file)
{
    char rx;
    msg_t rx_msg;
    /* Check the serial port for data */
    if(read(serial_file, (void*)&rx, 1) == 1)
    {
        /* Check for packet header */
        if(rx == PACKET_HEADER)
        {
            DEBUG(("[Task_Serial] Detected client message packet.\n"));
            /* Get the length of the message */
            if(read(serial_file, (void*)&rx, 1) == 1)
            {
                /* Get the message body */
                if(read(serial_file, (void*)&rx_msg, rx) == rx)
                {
                    /* Validate the CRC */
                    if(read(serial_file, (void*)&rx, 1) ==1)
                    {
                        if(validateCRC(rx_msg, rx))
                        {
                            DEBUG(("[Task_Serial] Received a valid message from client.\n"));
                            /* Parse the message to the corresponding client
                               comm handling task */
                            parseMessage(&rx_msg);
                            DEBUG(("[Task_Serial] Parsed message to client.\n"));
                            /* Zero the received message */
                            memset(&rx_msg, 0, sizeof(rx_msg));
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief Parse a message to the corresponding client comm handling task
 *
 * If the corresponding client comm handling task does not exist, the function
 * will create a dedicated mqueue and task for the client before it parse the message.
 *
 * @param   msg - pointer to the message to be parsed
 *
 * @return  none,
 */
static void parseMessage(msg_t *msg)
{
    /* Check if a corresponding client comm handling task exists */
    if(!client_table[msg->id])
    {
        /* Create a new comm handling task for the client and a dedicated mqueue */
        pthread_t thread;
        char qname[MAX_QUEUE_NAME_LENGTH];
        sprintf(qname, "/c%u",msg->id);
        if(msg_create_LINUX_mq(qname, MAX_LINUX_MQ_SIZE, &client_queue[msg->id])!=0)
        {
            perror("[ERROR] [Task_Serial] Failed to create a mqueue for the client.\n");
        }
        if(pthread_create(&thread, NULL, task_ClientCommHandling,
                          (void*)&client_queue[msg->id]) != 0)
        {
            perror("[ERROR] [Task_Serial] Failed to create a comm handling task
                    for the client.\n");
        }
        DEBUG(("[Task_Serial] Created a new task and mqueue for the client.\n"));
    }

    /* Enqueue the message to the corresponding mqueue */
    if(msg_send_LINUX_mq(&client_queue[msg->id], msg) != 0)
    {
        perror("[ERROR] [Task_Serial] Failed to enqueue the message.\n");
    }
    DEBUG(("[task_Serial] Enqueued the client message to the queue.\n"));
}
