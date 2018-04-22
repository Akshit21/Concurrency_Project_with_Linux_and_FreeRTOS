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
int8_t msg_create_LINUX_mq(char * q_name, uint32_t q_size, x_queue_t * q);
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
int8_t msg_destroy_LINUX_mq(x_queue_t * q);
{
    int ret = 0;
    
    /* Ensure that no other thread can touch the queue to be destroy */
    pthread_mutex_lock(&q->lock);
    
    /* Unlink the mqueue */
    if(mq_unlink(q->name) != 0)
        ret = -1;
    
    pthread_mutex_unlock(&q_lock);
    
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
int8_t msg_send_LINUX_mq(x_queue_t * q, msg_t * msg);
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
            if(mq_send(qhandle, (char*)msg, sizeof(msg), 0) == 0)
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
int8_t msg_receive_LINUX_mq(x_queue_t * q, msg_t * msg);
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
            if(mq_receive(qhandle, (char*)&msg, 1024, 0) == -1)
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
