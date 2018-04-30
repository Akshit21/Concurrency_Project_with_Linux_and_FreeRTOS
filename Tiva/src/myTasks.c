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

#define QUEUE_TEST

/* Queue Handle */
x_queue_t queue_handle;

SemaphoreHandle_t hb_sem;
uint8_t queue_flag = 1;

uint32_t motion_count = 0, avg = 0;

/* task handles */
extern TaskHandle_t interface_task_handle;
extern TaskHandle_t noise_task_handle;
extern TaskHandle_t motion_task_handle;

/* Timer handle */
TimerHandle_t tHandle[2];

/* Heart Beat */
uint8_t isAlive[2] = {0};

/* @ref:  https://cs.indstate.edu/~cbasavaraj/cs559/the_c_programming_language_2.pdf */
/* Reverse function */
static void reverse(char s[]) {
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* @ref:  https://cs.indstate.edu/~cbasavaraj/cs559/the_c_programming_language_2.pdf */
/* Itoa function */
static void itoa(int n, char s[]) {
    int i, sign;

    if ((sign = n) < 0)
         n = -n;
    i = 0;

    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}


/* Heart beat checker */
void vTimerCallback(TimerHandle_t timerHandle)
{
    if(timerHandle == tHandle[0] )
    {
        if(xSemaphoreTake(hb_sem, 100) == pdTRUE)
        {
            isAlive[0] = 0;
            isAlive[1] = 0;
            xSemaphoreGive(hb_sem);
        }
    }
    if(timerHandle == tHandle[1])
    {
        avg = motion_count;
        motion_count = 0;
    }
}

void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    signed char *pcTaskName )
{
    printf("Stack Overflow\n");
    while(1);
}

/* Init Analog Comparator module */
int8_t AnalogComparatorInit(void)
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

    return 0;
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
            UARTprintf("[DEBUG] NOISE !!!!\n");
#endif

            UARTprintf("[DEBUG] NOISE !!!!\n");

            /* Send msg over queue */
#ifdef QUEUE_TEST
            if (queue_flag)
            {
                /* Create a msg packet */
                msg_packet_t try;
                memset(&try, 0, sizeof(try));
                try.crc = 10;
                try.header = 2;
                try.msg.src = MSG_TIVA_NOISE_SENSING;
                try.msg.dst = MSG_TIVA_SOCKET;
                //memcpy(try.msg.timestamp,xTaskGetTickCount(),sizeof(try.msg.timestamp));
                try.msg.type = MSG_TYPE_CLIENT_LOG;
                try.msg.id = 1;
                //memcpy(try.msg.content, "NOISE", sizeof("NOISE"));

                /* Send the packet */
                if( msg_send_FreeRTOS_queue( &queue_handle, &try) != 0 )
                   {UARTprintf("[ERROR] Noise Send Queue Fail\n");}
            }
#endif
            /* Notify the interface task about an event */
            //xTaskNotify(interface_task_handle,NOISE_ALERT,eSetBits);
            xTaskNotifyGive(interface_task_handle);
        }

        static uint8_t count = 0;
        /* Update the HeartBeat Flag */
        if(xSemaphoreTake(hb_sem, 200) == pdTRUE)
        {
            isAlive[0] = 1;
            xSemaphoreGive(hb_sem);
            count++;
            if(count == 15)
            {
                UARTprintf("[LOG] HeartBeat MSG from Noise\n");
                count = 0;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // wait for one second
    }

    /* Delete the Task */
    vTaskDelete( NULL );
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
            UARTprintf("[DEBUG] STRANGER ALERT !!!\n");
#endif
            UARTprintf("[DEBUG] STRANGER ALERT !!!\n");

            /* Increment the motion count */
            motion_count++;
            /* Send msg over queue */
#ifdef QUEUE_TEST
            if (queue_flag)
            {
                /* Create the msg Packet */
                msg_packet_t try;
                memset(&try, 0, sizeof(try));
                try.crc = 10;
                try.header = 2;
                try.msg.src = MSG_TIVA_MOTION_SENSING;
                try.msg.dst = MSG_TIVA_SOCKET;
                //memcpy(try.msg.timestamp,xTaskGetTickCount(),sizeof(try.msg.timestamp));
                try.msg.type = MSG_TYPE_CLIENT_LOG;
                try.msg.id = 1;
                //memcpy(try.msg.content, "motio", sizeof("motio"));

                /* Send the packet */
                if( msg_send_FreeRTOS_queue( &queue_handle, &try) != 0 )
                   {UARTprintf("[ERROR] Motion Send queue Failed\n");};
            }
#endif
            /* Notify the interface task about an event */
            //xTaskNotify(interface_task_handle,MOTION_ALERT,eSetBits);
            xTaskNotifyGive(interface_task_handle);
        }

        static uint8_t count = 0;
        /* Update the HeartBeat Flag */
        if(xSemaphoreTake(hb_sem, 200) == pdTRUE)
        {
            isAlive[1] = 1;
            xSemaphoreGive(hb_sem);
            count++;
            if(count == 15)
            {
                UARTprintf("[LOG] HeartBeat MSG from Motion\n");
                count = 0;
            }
        }
        vTaskDelay( pdMS_TO_TICKS(2000) ); // wait for two second
    }

    /* Delete the Task */
    vTaskDelete( NULL );
}

/* Heart Beat Task handler */
void hb_task(void *params)
{
    uint8_t retries = 3, flag1 = 0, flag2 = 0;

    /* Create the message packet */
    msg_packet_t alert_msg;
    msg_t myMsg;
    memset(&alert_msg, 0, sizeof(req_t));
    memset(&myMsg, 0, sizeof(msg_t));
    myMsg.id = 1;
    myMsg.dst = MSG_BBB_HEARTBEAT;
    myMsg.type = MSG_TYPE_CLIENT_HEARTBEAT_RESPONSE;
    myMsg.src = MSG_TIVA_HEARTBEAT;
    myMsg.content[0] ='0';
    myMsg.content[1] ='0';

    /* Check the HB flags */
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

    alert_msg = msg_create_messagePacket(&myMsg);

    /* Send the HB Response */
    UART_send((int8_t*)&alert_msg, sizeof(msg_packet_t));

    /* Delete the task which is not responding */
    if(flag1 == 0)  vTaskDelete( noise_task_handle );
    if(flag2 == 0)  vTaskDelete( motion_task_handle );

    /* Delete the Task */
    vTaskDelete( NULL );
}

/* Main Interface Task Hander */
void interface_task(void *params)
{
    //uint32_t notify = 0;
#define ECHO
#ifdef ECHO
    msg_packet_t uart_packet;
#else
    req_packet_t uart_packet;
#endif
    memset(&uart_packet, 0, sizeof(req_packet_t));

    /* Timer for HearBeat */
    tHandle[0] = xTimerCreate("HB_Timer", pdMS_TO_TICKS(3000) , pdTRUE,  (void*)0, vTimerCallback);
    tHandle[1] = xTimerCreate("Comm", pdMS_TO_TICKS(30000) , pdTRUE,  (void*)0, vTimerCallback);

    if(tHandle[0] == NULL || tHandle[1] == NULL)
    {
        while(1);
    }

    /* Start the HB timer */
    if((xTimerStart(tHandle[0], 0)) != pdTRUE)
    {
        UARTprintf("[ERROR] Timer1 Start Error\n");
        while(1);
    }

    if((xTimerStart(tHandle[1], 0)) != pdTRUE)
    {
        UARTprintf("[ERROR] Timer2 Start Error\n");
        while(1);
    }

    while(1)
    {
        /* Check if you received any notification from the sensors */
        if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500)))
        {
            msg_packet_t sensor_packet;
            memset(&sensor_packet,0,sizeof(msg_packet_t));

            if( msg_receive_FreeRTOS_queue( &queue_handle, &sensor_packet) == 0 )
            {
                UARTprintf("[DEBUG] QUEUE MSG: %s\n", sensor_packet.msg.src == MSG_TIVA_NOISE_SENSING?"NOISE":"MOTION");
                /* Send the notification to BBG */
                msg_packet_t alert_msg;
                msg_t myMsg;
                memset(&alert_msg, 0, sizeof(msg_packet_t));
                memset(&myMsg, 0, sizeof(msg_t));
                myMsg = msg_create_msgStruct(sensor_packet.msg.src);
                alert_msg = msg_create_messagePacket(&myMsg);

                /* Send the msg to BBG */
                UARTprintf("[LOG] Sending Alert to BBG\n");
                UARTprintf("[DEBUG] HEADER: %d\t CRC:%d\n", alert_msg.header, alert_msg.crc);
                UARTprintf("[DEBUG] SRC: %d\t DST: %d\n", alert_msg.msg.src, alert_msg.msg.dst);
                UARTprintf("[DEBUG] ID: %d\t TYPE: %d\n", alert_msg.msg.id, alert_msg.msg.type);
                UARTprintf("[DEBUG] MSG: %s\n", alert_msg.msg.content);
                UART_send((int8_t*)&alert_msg, sizeof(msg_packet_t));
            }

        }

        /* Check if there are any UART messages */
        if(UART_receive((int8_t *)&uart_packet, sizeof(uart_packet)) == 0)
        {
            if(uart_packet.header == USER_PACKET_HEADER)
            {

                UARTprintf("[LOG] Recvd Valid UART Packet\n");
                UARTprintf("[DEBUG] HEADER: %d\t CRC:%d\n", uart_packet.header, uart_packet.crc);
                UARTprintf("[DEBUG] SRC: %d\t DST: %d\n", uart_packet.msg.src, uart_packet.msg.dst);
                UARTprintf("[DEBUG] ID: %d\t TYPE: %d\n", uart_packet.msg.id, uart_packet.msg.type);
                UARTprintf("[DEBUG] MSG: %s\n", uart_packet.msg.content);
                /* Process the message */
                switch(uart_packet.msg.type)
                {
                    case MSG_TYPE_CLIENT_HEARTBEAT_REQUEST:
                    {
                       if(xTaskCreate(hb_task, (const portCHAR *)"hb_task", MY_STACK_SIZE, NULL,
                                           tskIDLE_PRIORITY, NULL) != pdTRUE)
                       {
                           UARTprintf("[ERROR] HB Task Creation Failed\n");
                       }
                       break;
                    }

                    case MSG_TYPE_SERVER_REQUEST_TO_CLIENT:
                    {
                        msg_t resp;
                        resp.src = MSG_TIVA_SOCKET;
                        resp.dst = MSG_BBB_COMMAND;
                        resp.id = 1;
                        resp.type = MSG_TYPE_CLIENT_RESPONSE_TO_SERVER;
                        itoa(avg,resp.content);
                        msg_packet_t alert_msg = msg_create_messagePacket(&resp);

                        /* Send response for the request */
                        UARTprintf("[LOG] Sending Response Packet to BBG\n");
                        UART_send((int8_t*)&alert_msg, sizeof(msg_packet_t));
                        break;
                    }

                    default:
                        break;
                }
                memset(&uart_packet, 0, sizeof(uart_packet));
            }
            else
            {
                UARTprintf("[ERROR] Received Invalid UART Packet\n");
            }
        }
        //vTaskDelay( 200 / portTICK_PERIOD_MS ); // wait for two second
    }

    /* Clean up */
    xTimerDelete( tHandle, 200 );
    msg_destroy_FreeRTOS_queue(&queue_handle);
    vSemaphoreDelete( hb_sem );
    /* Delete the Task */
    vTaskDelete( NULL );
}

/* Initialize queue and synchronization modules */
int8_t init_queue()
{
    int8_t ret = msg_create_FreeRTOS_queue(&queue_handle, MAX_QUEUE_ELEMENTS, sizeof(msg_packet_t));

    if (ret == -1)
    {
        UARTprintf("[ERROR] Error in creating Message queue\n");
        queue_flag = 0;
        return -1;
    }

    hb_sem = xSemaphoreCreateMutex();
    if(hb_sem == NULL)     return -1;

    LEDWrite(0x0F, GPIO_PIN_3);
    return 0;
}
