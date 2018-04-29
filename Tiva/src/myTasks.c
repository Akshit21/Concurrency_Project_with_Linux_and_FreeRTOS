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
extern TaskHandle_t interface_task_handle;

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
            //xTaskNotify(interface_task_handle,NOISE_ALERT,eSetBits);
        }
        vTaskDelay( 1000 / portTICK_PERIOD_MS ); // wait for one second
    }
}


/* Motion Sensor Processing PC4 */
void motion_sensor_task(void *params)
{
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
            //xTaskNotify(interface_task_handle,MOTION_ALERT,eSetBits);
        }
        vTaskDelay( 2000 / portTICK_PERIOD_MS ); // wait for two second
    }
}

void interface_task(void *params)
{
    uint32_t notify = 0;
    //uint32_t motion_count = 0;
    msg_t alert_msg;
    msg_packet_t uart_packet;
    while(1)
    {
        if(msg_receive_FreeRTOS_queue(&message_queue, &uart_packet) == 0)
        {
            UARTprintf("Header:%d\n",uart_packet.header);
            //UARTprintf("Msg: %s\n", uart_packet.msg.content);
        }

        /* Check if you received any notification from the sensors */
        if( xTaskNotifyWait( pdFALSE,      /* Don't clear bits on entry. */
                             ULONG_MAX,    /* Clear all bits on exit. */
                             &notify,      /* Stores the notified value. */
                             pdMS_TO_TICKS(500)) == pdPASS )
        {
            if(notify & NOISE_ALERT)
            {
                UARTprintf("NOISE_ALERT !!!\n");

                /* Create the message structure */
                memcpy(&alert_msg,0,sizeof(alert_msg));
                alert_msg.src = MSG_TIVA_NOISE_SENSING;
                alert_msg.dst = MSG_BBB_LOGGING;
                alert_msg.type = MSG_TYPE_CLIENT_ALERT;
                //strncpy(alert_msg.content,"NOISE ALERT",strlen("NOISE ALERT"));

                /* Create a UART Packet for BBG */
                uart_packet = msg_create_messagePacket(&alert_msg);
                //UART_send((int8_t*)&uart_packet, sizeof(msg_packet_t));

                /* Flush the data */
                memcpy(&alert_msg,0,sizeof(alert_msg));
                memcpy(&uart_packet,0,sizeof(uart_packet));
            }
            if(notify & MOTION_ALERT)
            {
                UARTprintf("MOTION_ALERT !!!\n");

                /* Create the message structure */
                memcpy(&alert_msg,0,sizeof(alert_msg));
                alert_msg.src = MSG_TIVA_MOTION_SENSING;
                alert_msg.dst = MSG_BBB_LOGGING;
                alert_msg.type = MSG_TYPE_CLIENT_ALERT;
                //strncpy(alert_msg.content,"NOISE ALERT",strlen("NOISE ALERT"));

                /* Create a UART Packet for BBG */
                uart_packet = msg_create_messagePacket(&alert_msg);
                //UART_send((int8_t*)&uart_packet, sizeof(msg_packet_t));

                /* Flush the data */
                memcpy(&alert_msg,0,sizeof(alert_msg));
                memcpy(&uart_packet,0,sizeof(uart_packet));
            }
        }
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
    return;
}
