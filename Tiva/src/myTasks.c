/*
 * myTasks.c
 *
 *  Created on: Apr 22, 2018
 *      Author: akshit
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>

#include "drivers/pinout.h"
#include "utils/uartstdio.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/gpio.h"
#include "driverlib/comp.h"
#include "driverlib/rom_map.h"

#include "driverlib/inc/hw_memmap.h"

#include "main.h"
#include "myTask.h"
#include "message.h"
#include "myUART.h"

#ifdef QUEUE_TEST
x_queue_t noise_queue, motion_queue;
uint8_t flag = 1, flag1 = 1;
#endif

x_queue_t message_queue;
SemaphoreHandle_t hb_sem;
extern TaskHandle_t interface_task_handle;
extern TaskHandle_t noise_task_handle;
extern TaskHandle_t motion_task_handle;
TimerHandle_t tHandle;

uint8_t isAlive[2] = {0};

void vTimerCallback(TimerHandle_t timerHandle)
{
    if(xSemaphoreTake(hb_sem, 100) == pdTRUE)
    {
        isAlive[0] = 0;
        isAlive[1] = 0;
        xSemaphoreGive(hb_sem);
    }
}

void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    signed char *pcTaskName )
{
    printf("Stack Overflow\n");
    while(1);
}

void AnalogComparatorInit(void)
{
    /*  Enable the COMP module. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_COMP0);

    /* Configure the internal Reference voltage */
    ComparatorRefSet(COMP_BASE ,COMP_REF_1_03125V);

    /* Configure Comparator 0 (Noise Sensor)*/
    ComparatorConfigure(COMP_BASE, 0,
                        (COMP_TRIG_NONE | COMP_INT_FALL |
                         COMP_ASRCP_REF | COMP_OUTPUT_INVERT));

    /* Configure Comparator 1 (Motion Sensor) */
    ComparatorConfigure(COMP_BASE, 1,
                        (COMP_TRIG_NONE | COMP_INT_FALL |
                         COMP_ASRCP_REF | COMP_OUTPUT_INVERT));

}

/* Noise Sensor Processing PC7 */
void noise_sensor_task(void *params)
{
    //uint32_t notify = 0;
    while(1)
    {
        if(ComparatorValueGet(COMP_BASE, 0)) /* If there is noise */
        {
#ifdef MY_DEBUG  /* Blink LED if there is noise */
            static uint32_t blink_led = GPIO_PIN_0;
            LEDWrite(0x0F, blink_led);
            blink_led ^= (GPIO_PIN_0);
            UARTprintf("NOISE !!!!\n");
#endif
            /* We can use task notify or queue send */
#ifdef QUEUE_TEST
            if (flag)
            {
                static uint32_t i = 10;
                uint32_t j;
                if (msg_send_FreeRTOS_queue(&motion_queue, &i) == 0)
                   {UARTprintf("Sent data from Noise Task Successfully\n"); i++;}
                else
                   {UARTprintf("Motion Queue is full\n"); xQueueReset( motion_queue.queue );}

                if (msg_receive_FreeRTOS_queue(&noise_queue, &j) == 0)
                   {UARTprintf("Received data from Motion = %u\n", j);}
                else
                   {UARTprintf("Noise Queue is empty\n");}

            }
#endif
            /* Notify the interface task about an event */
            msg_packet_t try;
            memset(&try, 0, sizeof(try));
            try.crc = 10;
            try.header = 2;
            memcpy(try.msg.content, "noise", sizeof(try.msg.content));

            //UART_send((int8_t*)&try, sizeof(msg_packet_t));
            xTaskNotify(interface_task_handle,NOISE_ALERT,eSetBits);
        }

#ifdef NOTIFY_HEARTBEAT
        /* Heart Beat */
        if( xTaskNotifyWait( pdFALSE,      /* Don't clear bits on entry. */
                             ULONG_MAX,    /* Clear all bits on exit. */
                             &notify,      /* Stores the notified value. */
                             pdMS_TO_TICKS(500)) == pdPASS )
        {
            if(notify & HEARTBEAT_REQ1)
                xTaskNotify(interface_task_handle,HEARTBEAT_NOISE,eSetBits);
        }
#endif
        if(xSemaphoreTake(hb_sem, 200) == pdTRUE)
        {
            isAlive[0] = 1;
            xSemaphoreGive(hb_sem);
        }
        vTaskDelay( 1000 / portTICK_PERIOD_MS ); // wait for one second
    }
}


/* Motion Sensor Processing PC4 */
void motion_sensor_task(void *params)
{
    //uint32_t notify = 0;

    while(1)
    {
        if(ComparatorValueGet(COMP_BASE, 1)) /* If there is motion */
        {
#ifdef MY_DEBUG  /* Blink LED if there is motion */
            static uint32_t blink_led = GPIO_PIN_1;
            LEDWrite(0x0F, blink_led);
            blink_led ^= (GPIO_PIN_1);
            UARTprintf("STRANGER ALERT !!!\n");
#endif
#ifdef QUEUE_TEST
            /* We can use task notify or queue send */
            if (flag1)
            {
                static uint32_t i = 20;
                uint32_t j;
                if (msg_send_FreeRTOS_queue(&noise_queue, &i) == 0)
                    {UARTprintf("Sent data from Noise Task Successfully\n");i++;}
                else
                    {UARTprintf("Noise Queue is full\n"); xQueueReset( noise_queue.queue );}

                if (msg_receive_FreeRTOS_queue(&motion_queue, &j) == 0)
                   {UARTprintf("Received data from Noise = %u\n", j);}
                else
                   {UARTprintf("Motion Queue is empty\n");}
            }
#endif
            /* Notify the interface Task about an event */
            msg_packet_t try;
            memset(&try, 0, sizeof(try));
            try.crc = 10;
            try.header = 2;
            memcpy(try.msg.content, "motion", sizeof(try.msg.content));

            //UART_send((int8_t*)&try, sizeof(msg_packet_t));
            xTaskNotify(interface_task_handle,MOTION_ALERT,eSetBits);
        }

        /* UPDATE HEARTBEAT */
#ifdef NOTIFY_HEARTBEAT
        if( xTaskNotifyWait( pdFALSE,      /* Don't clear bits on entry. */
                             ULONG_MAX,    /* Clear all bits on exit. */
                             &notify,      /* Stores the notified value. */
                             pdMS_TO_TICKS(500)) == pdPASS )
        {
            if(notify & HEARTBEAT_REQ2)
                xTaskNotify(interface_task_handle,HEARTBEAT_MOTION,eSetBits);
        }
#endif
        if(xSemaphoreTake(hb_sem, 200) == pdTRUE)
        {
            isAlive[1] = 1;
            xSemaphoreGive(hb_sem);
        }
        vTaskDelay( 2000 / portTICK_PERIOD_MS ); // wait for two second
    }

}

void hb_task(void *params)
{
    uint8_t retries = 3, flag1 = 0, flag2 = 0;
    msg_packet_t alert_msg;
    msg_t myMsg;
    memset(&alert_msg, 0, sizeof(msg_packet_t));
    memset(&myMsg, 0, sizeof(msg_t));
    myMsg.id = 1;
    myMsg.dst = MSG_BBB_HEARTBEAT;
    myMsg.type = MSG_TYPE_CLIENT_HEARTBEAT_RESPONSE;
    myMsg.src = MSG_TIVA_HEARTBEAT;
    myMsg.content[0] ='0';
    myMsg.content[1] ='0';

    do{
        if(xSemaphoreTake(hb_sem, 200) == pdTRUE)
        {
            if(isAlive[0] == 1)    {flag1 = 1;myMsg.content[0] ='1';}
            if (isAlive[1] == 1)   {flag2 = 1;myMsg.content[1] ='1';}

            xSemaphoreGive(hb_sem);
        }
        if(flag1 && flag2)  break;
        else vTaskDelay( 500 / portTICK_PERIOD_MS );
    }while(retries--);

    myMsg.timestamp = xTaskGetTickCount();
    alert_msg = msg_create_messagePacket(&myMsg);
    alert_msg.crc = 1;
    UART_send((int8_t*)&alert_msg, sizeof(msg_packet_t));

    /* Delete the task which is not responding */
    if(flag1 == 0)  vTaskDelete( noise_task_handle );
    if(flag2 == 0)  vTaskDelete( motion_task_handle );

    /* Delete the Task */
    vTaskDelete( NULL );
}

void interface_task(void *params)
{
    uint32_t notify = 0;
    msg_packet_t uart_packet;
    memset(&uart_packet, 0, sizeof(msg_packet_t));

    /* Timer for HearBeat */
    tHandle = xTimerCreate("HB_Timer", pdMS_TO_TICKS(3000) , pdTRUE,  (void*)0, vTimerCallback);

    if(tHandle == NULL)
    {
        while(1);
    }

    if((xTimerStart(tHandle, 0)) != pdTRUE)
    {
        while(1);
    }


    while(1)
    {

        /* Check if you received any notification from the sensors */
        if( xTaskNotifyWait( pdFALSE,      /* Don't clear bits on entry. */
                             ULONG_MAX,    /* Clear all bits on exit. */
                             &notify,      /* Stores the notified value. */
                             pdMS_TO_TICKS(500)) == pdPASS )
        {
            if(notify & NOISE_ALERT)
            {
                UARTprintf("NOISE_ALERT !!!\n");

                /* Send the notification to BBG */
                msg_packet_t alert_msg;
                msg_t myMsg;
                memset(&alert_msg, 0, sizeof(msg_packet_t));
                memset(&myMsg, 0, sizeof(msg_t));
                myMsg = msg_create_msgStruct(MSG_TIVA_NOISE_SENSING);
                alert_msg = msg_create_messagePacket(&myMsg);
                //UART_send((int8_t*)&alert_msg, sizeof(msg_packet_t));

            }

            if(notify & MOTION_ALERT)
            {
                UARTprintf("MOTION_ALERT !!!\n");

                /* Send the notification to BBG */
                msg_packet_t alert_msg;
                msg_t myMsg;
                memset(&alert_msg, 0, sizeof(msg_packet_t));
                memset(&myMsg, 0, sizeof(msg_t));
                myMsg = msg_create_msgStruct(MSG_TIVA_MOTION_SENSING);
                alert_msg = msg_create_messagePacket(&myMsg);
                //UART_send((int8_t*)&alert_msg, sizeof(msg_packet_t));
            }

#ifdef NOTIFY_HEARTBEAT
            if(notify & HEARTBEAT_MOTION)
            {
                UARTprintf("MOTION HeartBeat !!!\n");

                // Send the notification to BBG
                msg_packet_t alert_msg;
                msg_t myMsg;
                memset(&alert_msg, 0, sizeof(msg_packet_t));
                memset(&myMsg, 0, sizeof(msg_t));
                myMsg = msg_create_msgStruct(MSG_TIVA_HEARTBEAT);
                myMsg.dst = MSG_BBB_HEARTBEAT;
                myMsg.type = MSG_TYPE_CLIENT_HEARTBEAT_RESPONSE;
                memcpy(myMsg.content,"MOTION HB",sizeof(myMsg.content));
                alert_msg = msg_create_messagePacket(&myMsg);
                UART_send((int8_t*)&alert_msg, sizeof(msg_packet_t));
            }

            if(notify & HEARTBEAT_NOISE)
            {
                UARTprintf("NOISE HeartBeat !!!\n");

                // Send the notification to BBG
                msg_packet_t alert_msg;
                msg_t myMsg;
                memset(&alert_msg, 0, sizeof(msg_packet_t));
                memset(&myMsg, 0, sizeof(msg_t));
                myMsg = msg_create_msgStruct(MSG_TIVA_HEARTBEAT);
                myMsg.dst = MSG_BBB_HEARTBEAT;
                myMsg.type = MSG_TYPE_CLIENT_HEARTBEAT_RESPONSE;
                memcpy(myMsg.content,"NOISE HB",sizeof(myMsg.content));
                alert_msg = msg_create_messagePacket(&myMsg);
                UART_send((int8_t*)&alert_msg, sizeof(msg_packet_t));
            }
#endif
        }

        if(UART_receive((int8_t *)&uart_packet, sizeof(msg_packet_t)) == 0)
        {
            UARTprintf("%s\n",uart_packet.msg.content);
            /* Process the message */
            switch(uart_packet.msg.type)
            {
                case MSG_TYPE_CLIENT_HEARTBEAT_REQUEST:
                {
#ifdef NOTIFY_HEARTBEAT
                    xTaskNotify(noise_task_handle,HEARTBEAT_REQ1,eSetBits);
                    xTaskNotify(motion_task_handle,HEARTBEAT_REQ2,eSetBits);
#endif
                   if(xTaskCreate(hb_task, (const portCHAR *)"hb_task", MY_STACK_SIZE, NULL,
                                       tskIDLE_PRIORITY, NULL) != pdTRUE)
                   {
                       UARTprintf("HB Task Creation Failed\n");
                   }
                   break;
                }

                case MSG_TYPE_SERVER_REQUEST_TO_CLIENT:
                {
                    break;
                }

                default:
                    break;
            }
            memset(&uart_packet, 0, sizeof(msg_packet_t));
        }
        vTaskDelay( 200 / portTICK_PERIOD_MS ); // wait for two second
    }
}

void init_queue()
{
#ifdef QUEUE_TEST
    if (msg_create_FreeRTOS_queue(&noise_queue, 10, sizeof(uint32_t)) == -1)
        {UARTprintf("Error in creating Noise queue\n");flag=0;}
    if (msg_create_FreeRTOS_queue(&motion_queue, 10, sizeof(uint32_t)) == -1)
        {UARTprintf("Error in creating Noise queue\n");flag1=0;}
    if(flag && flag1)   LEDWrite(0x0F, GPIO_PIN_3);
#endif
    if (msg_create_FreeRTOS_queue(&message_queue, MAX_QUEUE_ELEMENTS, sizeof(msg_packet_t)) == -1)
        {UARTprintf("Error in creating Message queue\n");return;}
    LEDWrite(0x0F, GPIO_PIN_3);
    hb_sem = xSemaphoreCreateMutex();
    return;
}
