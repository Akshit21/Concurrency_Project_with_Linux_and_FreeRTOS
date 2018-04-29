/*
 * message.c
 *
 *  Created on: Apr 28, 2018
 *      Author: akshit
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include "drivers/pinout.h"
#include "utils/uartstdio.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/gpio.h"
#include "driverlib/comp.h"
#include "driverlib/rom_map.h"
#include "driverlib/emac.h"

#include "driverlib/inc/hw_memmap.h"

#include "message.h"

int8_t msg_create_FreeRTOS_queue(x_queue_t * q, uint32_t q_num_element, uint32_t q_element_size)
{
    /* Initialize queue lock*/
    q->lock = xSemaphoreCreateMutex();
    if(q->lock == NULL)     return -1;

    /* Create queue */
    q->queue = xQueueCreate( q_num_element, sizeof( q_element_size ) );
    if(q->queue == NULL)    return -1;

    return 0;
}

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

int8_t msg_send_FreeRTOS_queue(x_queue_t * q, uint32_t * msg)
{
    uint8_t retries = 3;

    /* Acquire the lock */
    if( xSemaphoreTake( q->lock , ( TickType_t ) 500 ) == pdTRUE )
    {
        /* Enqueue the messages with retries */
        do
        {
            if( xQueueSend( q->queue, msg,
                           ( TickType_t ) 500 ) == pdPASS )
                break;
            else
                retries --;
        } while ( retries > 0);

        xSemaphoreGive( q->lock );
    }

    if (retries == 0)   return -1;

    return 0;
}

int8_t msg_receive_FreeRTOS_queue(x_queue_t * q, uint32_t * msg)
{
    uint8_t retries = 3;

    /* Acquire the lock */
    if( xSemaphoreTake( q->lock , ( TickType_t ) 500 ) == pdTRUE )
    {
        /* Enqueue the messages with retries */
        do
        {
            if( xQueueReceive( q->queue, msg,
                               ( TickType_t ) 500 ) == pdPASS )
                break;
            else
                retries --;
        } while ( retries > 0);

        xSemaphoreGive( q->lock );
    }

    if (retries == 0)   return -1;

    return 0;
}
