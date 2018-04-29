#include "messageConfig.h"
#include "message.h"

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

int8_t req_send_LINUX_mq(x_queue_t * q, req_t * req)
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
            if(mq_send(qhandle, (char*)req, sizeof(*req), 0) == 0)
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
int8_t msg_create_FreeRTOS_queue(x_queue_t * q);

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
int8_t msg_destroy_FreeRTOS_queue(x_queue_t * q);

/**
 * @brief send a message via queue
 *
 * @param q - pointer to a queue handle
 *        msg - pointer to the message to be sent
 *
 * @return  0 - success
 *         -1 - failed
 */
int8_t msg_send_FreeRTOS_queue(x_queue_t * q, msg_t * msg);

/**
 * @brief receive a messages from queue
 *
 * @param q - pointer to a queue handle
 *        msg - pointer to the message storage
 *
 * @return  0 - success
 *         -1 - failed
 */
int8_t msg_receive_FreeRTOS_queue(x_queue_t * q, msg_t * msg);

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
 * @brief Pack a request into a packet
 *
 * @param msg - pointer to the request to be packed
 *
 * @return  the message packet
 */
req_packet_t msg_create_requestPacket(req_t * req)
{
    req_packet_t packet;
    packet.header = USER_PACKET_HEADER;
    packet.msg = *req;
    packet.crc = msg_compute_messagePacketCRC((uint8_t *)req, sizeof(*req));

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

#include <stdio.h>
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
        // printf("header: %x\n", packet->header);
        // printf("id: %d | src: %x | dst: %x | type: %x\n", packet->msg.id, packet->msg.src, packet->msg.dst, packet->msg.type);
        // printf("time: %s\n", packet->msg.timestamp);
        // printf("content: %s\n", packet->msg.content);
        /* Packet has a valid header */
        // if(msg_compute_messagePacketCRC((uint8_t *)&packet->msg, sizeof(packet->msg))
        //    != packet->crc)
        // {
        //     printf("wrong crc\n");
        //     ret = 0;
        // }
        // else
            ret = 1;
    }
    else
        printf("wrong header\n");

    return ret;
}

int8_t req_validate_messagePacket(req_packet_t * packet)
{
    int8_t ret = 0;

    if(packet->header == USER_PACKET_HEADER)
    {
        /* Packet has a valid header */
        // if(msg_compute_messagePacketCRC((uint8_t *)&packet->msg, sizeof(packet->msg))
        //    != packet->crc)
        // {
        //     printf("wrong crc\n");
        //     ret = 0;
        // }
        // else
            ret = 1;
    }
    else
        printf("wrong header\n");

    return ret;
}
#endif
