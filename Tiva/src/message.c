#include "messageConfig.h"
#include "message.h"
#include "utils/uartstdio.h"
#include <stdio.h>

#ifdef USE_MESSAGE_OVER_LINUX_MQUEUE

/**
 * @brief create a LINUX mqueue for messaging
 *
 * Create a mqueue and initialize its mutex lock and
 * conditional variable
 *
 * @param   q_name - name of the queue
 *          q_size - maximum number of messages on the queue
 *          q - pointer to a queue handle
 *
 * @return  0 - success
 *         -1 - failed
 */
int8_t msg_create_LINUX_mq(char * q_name, uint32_t q_size, x_queue_t * q)
{
    int ret = 0;

    /* Queue attributes */
    q->attr.mq_maxmsg = q_size;
    q->attr.mq_msgsize = sizeof(msg_t);
    q->attr.mq_flags = 0;

    /* Initilaize the queue lock */
    if(pthread_mutex_init(&q->lock, NULL) != 0)
        ret = -1;

    /* Assign the queue name */
    strncpy(q->name, q_name, strlen(q_name));

    return ret;
}

/**
 * @brief destroy a LINUX mqueue in use
 *
 * Close and unlink a mqueue, destroy its mutex lock and
 * conditional variable
 *
 * @param q - pointer to a queue handle
 *
 * @return  0 - success
 *         -1 - failed
 */
int8_t msg_destroy_LINUX_mq(x_queue_t * q)
{
    int ret = 0;

    /* Ensure that no other thread can touch the queue to be destroy */
    pthread_mutex_lock(&q->lock);

    /* Unlink the mqueue */
    if(mq_unlink(q->name) != 0)
        ret = -1;

    pthread_mutex_unlock(&q->lock);

    /* Destroy the queue lock */
    if(pthread_mutex_destroy(&q->lock) != 0)
        ret = -1;

    return ret;
}
/**
 * @brief send a message via queue
 *
 * @param q - pointer to a queue handle
 *        msg - pointer to the message to be sent
 *
 * @return  0 - success
 *         -1 - failed
 */
int8_t msg_send_LINUX_mq(x_queue_t * q, msg_t * msg)
{
    uint8_t retries = 3;
    mqd_t qhandle;
    int8_t ret = 0;

    pthread_mutex_lock(&q->lock);

    /* Open the mqueue */
    qhandle = mq_open(q->name, O_CREAT|O_RDWR, S_IWUSR | S_IRUSR, &q->attr);
    if(qhandle == (mqd_t) -1)
        ret = -1;

    /* Enqueue the messages with retries if the queue is opened */
    else
    {
        do
        {
            if(mq_send(qhandle, (char*)msg, sizeof(*msg), 0) == 0)
                break;
            else
                retries --;
        }while(retries > 0 );

        if(retries == 0)
            ret = -1;

        /* Close the queue */
        if(mq_close(qhandle) == -1)
            ret = -1;
    }

    pthread_mutex_unlock(&q->lock);

    return ret;
}

/**
 * @brief receive a messages from queue, the function will block when the queue is empty
 *
 * @param q - pointer to a queue handle
 *        msg - pointer to the message storage
 *
 * @return  0 - success
 *         -1 - failed
 */
int8_t msg_receive_LINUX_mq(x_queue_t * q, msg_t * msg)
{
    uint8_t retries = 3;
    mqd_t qhandle;
    int8_t ret = 0;

    pthread_mutex_lock(&q->lock);

    /* Open the mqueue */
    qhandle = mq_open(q->name, O_CREAT|O_RDWR, S_IWUSR | S_IRUSR, &q->attr);
    if(qhandle == (mqd_t) -1)
        ret = -1;

    /* Dequeue a message with retries if the queue is opened */
    else
    {
        do
        {
            if(mq_receive(qhandle, (char*)msg, 1024, 0) == -1)
                retries --;
            else
                break;
        }while(retries > 0);

        if(retries == 0)
            ret = -1;

        /* Close the queue */
        if(mq_close(qhandle) == -1)
            ret = -1;
    }

    pthread_mutex_unlock(&q->lock);

    return ret;
}
#endif


#ifdef USE_MESSAGE_OVER_FREERTOS_QUEUE

#include "task.h"
/**
 * @brief create a FreeRTOS queue for messaging
 *
 * Create a FreeRTOS queue and initialize its mutex lock and
 * binary semaphore
 *
 * @param q - pointer to a queue handle
 *
 * @return  0 - success
 *         -1 - failed
 */
int8_t msg_create_FreeRTOS_queue(x_queue_t * q, uint32_t q_num_element, uint32_t q_element_size)
{
    /* Initialize queue lock*/
    q->lock = xSemaphoreCreateMutex();
    if(q->lock == NULL)     return -1;

    /* Create queue */
    q->queue = xQueueCreate( q_num_element, sizeof( q_element_size ) );
    if(q->queue == NULL)    {vSemaphoreDelete( q->lock );return -1;}

    return 0;
}

/**
 * @brief destroy a FreeRTOS queue in use
 *
 * Close and delete a FreeRTOS queue, destroy its mutex lock and
 * conditional variable
 *
 * @param q - pointer to a queue handle
 *
 * @return  0 - success
 *         -1 - failed
 */
int8_t msg_destroy_FreeRTOS_queue(x_queue_t * q)
{
    /* Ensure that no other thread can touch the queue to be destroy */
    if( xSemaphoreTake( q->lock , ( TickType_t ) 500 ) == pdTRUE )
    {
        /* Delete the queue */
        vQueueDelete( q->queue );

        /* Release the mutex */
        xSemaphoreGive( q->lock );
    }
    else    { return -1; }

    /* delete the queue lock */
    vSemaphoreDelete( q->lock );
    return 0;
}

/**
 * @brief send a message via queue
 *
 * @param q - pointer to a queue handle
 *        msg - pointer to the message to be sent
 *
 * @return  0 - success
 *         -1 - failed
 */
int8_t msg_send_FreeRTOS_queue(x_queue_t * q, msg_packet_t * msg)
{
    int8_t ret = -1;
    /* Acquire the lock */
    if( xSemaphoreTake( q->lock , ( TickType_t ) 200 ) == pdTRUE )
    {
        /* Enqueue the messages */
        if( xQueueSend( q->queue, msg, 200) == pdPASS )
        {
            UARTprintf("[LOG] Sent data to the Queue Successfully\n");
            ret = 0;
        }
        else
        {
            xQueueReset( q->queue );
        }
        xSemaphoreGive( q->lock );
    }

    return ret;
}

/**
 * @brief receive a messages from queue
 *
 * @param q - pointer to a queue handle
 *        msg - pointer to the message storage
 *
 * @return  0 - success
 *         -1 - failed
 */
int8_t msg_receive_FreeRTOS_queue(x_queue_t * q, msg_packet_t * msg)
{
    int8_t ret = -1;
    /* Acquire the lock */
    if( xSemaphoreTake( q->lock , ( TickType_t ) 500 ) == pdTRUE )
    {
        /* Dequeue the messages */
        if( uxQueueMessagesWaiting( q->queue ) > 0 && xQueueReceive( q->queue, msg, 200) == pdPASS )
        {
            UARTprintf("[LOG] Received data from the Queue Successfully\n");
            ret = 0;
        }
        else
        {
            //xQueueReset( q->queue );
            ret = -1;
        }
        xSemaphoreGive( q->lock );
    }

    return ret;
}

/**
 * @brief Pack a message into a struct
 *
 * @param src - src id
 *
 * @return  the message struct
 */
msg_t msg_create_msgStruct(msg_src_t src)
{
    msg_t msg;
    msg.id = 1;
    msg.src = src;
    msg.dst = MSG_BBB_LOGGING;
    msg.type = MSG_TYPE_LOG;
    // msg.timeStamp = xTaskGetTickCount();
    if(src == MSG_TIVA_NOISE_SENSING)
        memcpy(msg.content, "[A] N", sizeof(msg.content));
    else
        memcpy(msg.content, "[A] M", sizeof(msg.content));

    return msg;
}

#endif

#ifdef USE_MESSAGE_PACKET

/**
 * @brief Pack a message into a packet
 *
 * @param msg - pointer to the message to be packed
 *
 * @return  the message packet
 */
msg_packet_t msg_create_messagePacket(msg_t * msg)
{
    msg_packet_t packet;
    packet.header = USER_PACKET_HEADER;
    packet.msg = *msg;
    packet.crc = msg_compute_messagePacketCRC((uint8_t *)msg, sizeof(*msg));

    return packet;
}

/**
 * @brief Compute the CRC of a message
 *
 * @param msg - pointer to a message
 *
 * @return  a CRC value.
 * https://www.pololu.com/docs/0J44/6.7.6
 */
crc_t msg_compute_messagePacketCRC(uint8_t *msg, uint32_t length)
{
    crc_t crc;
    uint32_t i, j;

    for(i=0; i<length; i++)
    {
        crc ^= msg[i];
        for(j=0; j<8; j++)
        {
            if(crc&1)
                crc ^= 0x91;
            crc >>= 1;
        }
    }
    return crc;
}

/**
 * @brief validate a packet by checking the packet header and CRC
 *
 * @param packet - pointer to a message packet
 *
 * @return 0 - not valid
 *         1 - valid
 */
int8_t msg_validate_messagePacket(msg_packet_t * packet)
{
    int8_t ret = 0;

    if(packet->header == USER_PACKET_HEADER)
    {
        /* Packet has a valid header */
        if(msg_compute_messagePacketCRC((uint8_t *)&packet->msg, sizeof(packet->msg))
           == packet->crc)
            ret = 1;
    }

    return ret;
}
#endif
