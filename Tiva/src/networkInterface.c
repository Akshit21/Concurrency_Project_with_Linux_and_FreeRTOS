/*
 * networkInterface.c
 *
 *  Created on: Apr 22, 2018
 *      Author: akshit
 */

/* @Ref: Ben's lecture slides
 *       TI Software Doc: http://www.ti.com/lit/ug/spmu298d/spmu298d.pdf
 *       https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/index.html
 * */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
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
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"

#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/gpio.h"
#include "driverlib/comp.h"
#include "driverlib/rom_map.h"
#include "driverlib/emac.h"
#include "driverlib/interrupt.h"
#include "driverlib/flash.h"

#include "driverlib/inc/hw_memmap.h"
#include "driverlib/inc/hw_emac.h"
#include "driverlib/inc/hw_ints.h"

#include "main.h"
#include "NetworkInterface.h"

/* Network Process Task Handle */
TaskHandle_t xNetworkInterfaceProcessHandle = NULL;

void xNetworkInterfaceProcess(void *params);

void processReceive()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xNetworkInterfaceProcessHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* @ref https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/Embedded_Ethernet_Porting.html */
/* Network Interface Task Handler */
void vNetworkInterfaceProcess(void *params)
{
    NetworkBufferDescriptor_t *pxBufferDescriptor;
    uint32_t xBytesRecieved;
    IPStackEvent_t xRxEvent;

    while(1)
    {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        xBytesRecieved = packet_size();

        if (xBytesRecieved)
        {
            pxBufferDescriptor = pxGetNetworkBufferWithDescriptor(xBytesRecieved, 0);

            if (pxBufferDescriptor != NULL)
            {
                pxBufferDescriptor->xDataLength = network_rx(pxBufferDescriptor->pucEthernetBuffer, xBytesRecieved);

                /* The event about to be sent to the TCP/IP is an Rx event. */
                xRxEvent.eEventType = eNetworkRxEvent;
                xRxEvent.pvData = (void *)pxBufferDescriptor;

                if (xSendEventStructToIPTask(&xRxEvent, 0) == pdFALSE)
                {
                    vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
                    iptraceETHERNET_RX_EVENT_LOST();
                }
                else
                {
                    iptraceNETWORK_INTERFACE_RECEIVE();
                }
            }
            else
            {
                iptraceETHERNET_RX_EVENT_LOST();
            }
        }
    }
}

/* Initialize network interface tcp/ip */
BaseType_t xNetworkInterfaceInitialise()
{
    xTaskCreate(vNetworkInterfaceProcess, "EMAC", configMINIMAL_STACK_SIZE*10, NULL, configMAX_PRIORITIES - 1, &xNetworkInterfaceProcessHandle );

    /* Initialize Ethernet MAC and PHY layer */
    network_init();

    return pdPASS;
}

/* Send message from FreeRTOS to Ethernet layer */
BaseType_t xNetworkInterfaceOutput(NetworkBufferDescriptor_t *const pxDescriptor, BaseType_t xReleaseAfterSend)
{
    /* Transmit with Ethernet driver */
    network_tx(pxDescriptor->pucEthernetBuffer, pxDescriptor->xDataLength);

    /* Call trace function */
    iptraceNETWORK_INTERFACE_TRANSMIT();

    /* Release FreeRTOS Ethernet buffer */
    if (xReleaseAfterSend != pdFALSE)
    {
        vReleaseNetworkBufferAndDescriptor(pxDescriptor);
    }

    return pdTRUE;
}
