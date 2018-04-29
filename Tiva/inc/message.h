/*
 * message.h
 *
 *  Created on: Apr 28, 2018
 *      Author: akshit
 */

#ifndef INC_MESSAGE_H_
#define INC_MESSAGE_H_

#include "semphr.h"

typedef uint8_t msg_src_t, msg_dst_t, msg_type_t;
#define MAX_MESSAGE_LENGTH (100)
/* A universal message structure */
typedef struct msg
{
#ifdef USE_SERVER_CLIENT_MESSAGING
    uint8_t id; //0: server, >0: clients
#endif
    msg_src_t src;
    msg_dst_t dst;
    msg_type_t type;
#ifdef USE_MESSAGE_TIMESTAMP
    char timestamp[20]; // format: mm/dd/yyyy hh:mm:ss
#endif
    char content[MAX_MESSAGE_LENGTH];
}msg_t;

typedef struct x_queue
{
    QueueHandle_t queue;      // Enqueue and dequeue messages
    SemaphoreHandle_t lock;   // Protect queue operations
    SemaphoreHandle_t sem;    // Signal and wait for queue availability
}x_queue_t;

/**
 * @brief create a FreeRTOS queue for messaging
 *
 * Create a FreeRTOS queue and initialize its mutex lock and
 * binary semaphore
 *
 * @param q - pointer to a queue handle
 *        q_num_element - max number of elements in the queue
 *        q_element_size - size of each element in the queue
 *
 * @return  0 - success
 *         -1 - failed
 */
int8_t msg_create_FreeRTOS_queue(x_queue_t * q, uint32_t q_num_element, uint32_t q_element_size);

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
int8_t msg_send_FreeRTOS_queue(x_queue_t * q, uint32_t * msg);

/**
 * @brief receive a messages from queue
 *
 * @param q - pointer to a queue handle
 *        msg - pointer to the message storage
 *
 * @return  0 - success
 *         -1 - failed
 */
int8_t msg_receive_FreeRTOS_queue(x_queue_t * q, uint32_t * msg);



#endif /* INC_MESSAGE_H_ */
