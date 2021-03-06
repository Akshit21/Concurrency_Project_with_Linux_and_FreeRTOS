/*
 *@file main.c
 *
 *@brief Main File to start the program
 *
 *  Created on: Apr 19, 2018
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

#include "main.h"
#include "myTask.h"
#include "myUART.h"
#include "priorities.h"
#include "socket.h"
#include "NetworkInterface.h"
#include "message.h"

/* Task Handles */
TaskHandle_t interface_task_handle;
TaskHandle_t noise_task_handle;
TaskHandle_t motion_task_handle;

int main(void)
{
    int8_t ret = 0;

    /* Initialize system clock to 120 MHz */
    uint32_t MY_SYSTEM_CLOCK = ROM_SysCtlClockFreqSet(
                               (SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
                                SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),
                                SYSTEM_CLOCK);

    ASSERT(MY_SYSTEM_CLOCK == SYSTEM_CLOCK);

    /* GPIO Configuration */
    PinoutSet(false, false);

    /* UART Configuration for Debug Print */
    UARTStdioConfig(0, BAUD_RATE, SYSTEM_CLOCK);

    /* UART Configuration for BBG */
    if(UART_init() != 0)
        ret = -1;

    /* Analog Comparator Configuration */
    if(AnalogComparatorInit() != 0)
        ret = -1;

    /* Init Queue */
    if(init_queue() != 0)
        ret = -1;

#ifdef SOCKET
    /* Initialize FreeRTOS Socket and TCP */
    client_init();
#endif

    /* Main Task Handler */
    if(xTaskCreate(noise_sensor_task, (const portCHAR *)"noise_sensor_task", MY_STACK_SIZE, NULL,
                   tskIDLE_PRIORITY + PRIO_MY_TASK1, &noise_task_handle) != pdTRUE)
    {
        UARTprintf("[ERROR] Task 1 Creation Failed\n");
        ret = -1;
    }

    /* Create the task 2 */
    if(xTaskCreate(motion_sensor_task, (const portCHAR *)"motion_sensor_task", MY_STACK_SIZE, NULL,
                   tskIDLE_PRIORITY + PRIO_MY_TASK2, &motion_task_handle) != pdTRUE)
    {
        UARTprintf("[ERROR] Task 2 Creation Failed\n");
        ret = -1;
    }

    if(xTaskCreate(interface_task, (const portCHAR *)"interface_task", MY_STACK_SIZE, NULL,
                   tskIDLE_PRIORITY + PRIO_MY_TASK3, &interface_task_handle) != pdTRUE)
    {
        UARTprintf("[ERROR] Task 3 Creation Failed\n");
        ret = -1;
    }

    /* Prepare Startup Log MSG */
    msg_packet_t startup_msg;
    msg_t myMsg;
    memset(&startup_msg, 0, sizeof(msg_packet_t));
    memset(&myMsg, 0, sizeof(msg_t));
    myMsg.id = 1;
    myMsg.src = MSG_TIVA_SOCKET;
    myMsg.dst = MSG_BBB_LOGGING;
    myMsg.type = MSG_TYPE_LOG;

    /* Send the UART MSG */
    if(ret == 0)
    {
        UARTprintf("[LOG] STARTUP_TEST SUCCESS\n");
        memcpy(myMsg.content,"[S] S",sizeof("STP S"));
        startup_msg = msg_create_messagePacket(&myMsg);
        UART_send((int8_t*)&startup_msg, sizeof(msg_packet_t));
    }
    else
    {
        UARTprintf("[ERROR] STARTUP_TEST FAILED\n");
        memcpy(myMsg.content,"[S] F",sizeof("STP S"));
        startup_msg = msg_create_messagePacket(&myMsg);
        UART_send((int8_t*)&startup_msg, sizeof(msg_packet_t));

        return -1;
    }

    /* Start the Scheduler */
    vTaskStartScheduler();

    return 0;
}

/*  ASSERT() Error function
 *
 *  failed ASSERTS() from driverlib/debug.h are executed in this function
 */
void __error__(char *pcFilename, uint32_t ui32Line)
{
    // Place a breakpoint here to capture errors until logging routine is finished
    while (1)
    {
    }
}


