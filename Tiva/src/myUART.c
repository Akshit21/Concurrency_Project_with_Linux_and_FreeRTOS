/*
 * myUART.c
 *
 *  Created on: Apr 22, 2018
 *      Author: akshit
 */

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

#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/comp.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom_map.h"
#include "driverlib/pin_map.h"

#include "driverlib/inc/hw_memmap.h"
#include "driverlib/inc/hw_ints.h"

#include "main.h"
#include "myUART.h"
#include "message.h"

msg_packet_t rx;
extern x_queue_t message_queue;

void UARTIntHandler(void)
{
    uint32_t status;

    /* Get the interrupt status */
    status = UARTIntStatus(UART3_BASE, true);

    /* Clear the interrupt */
    UARTIntClear(UART3_BASE, status);

    static uint32_t bytes_recvd = 0;

    /* Get the byte */
    int8_t c = (int8_t)UARTCharGetNonBlocking(UART3_BASE);
    if (c != 0xFF && c != -1)
    {
        *((int8_t*) (&rx + bytes_recvd++)) = c;
        if(bytes_recvd == sizeof(msg_packet_t))
        {
            bytes_recvd = 0;
            //msg_send_FreeRTOS_queue(&message_queue, &rx);
            xQueueSendFromISR( message_queue.queue, &rx,
                                       NULL);
            memcpy(&rx,0,sizeof(msg_packet_t));
        }
    }

}

void UART_init()
{
    /* Enable the UART module. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    /* Configure UART */
    UARTConfigSetExpClk(UART3_BASE, SYSTEM_CLOCK, BAUD_RATE,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE));

    /* Enable all interrupts */
    IntMasterEnable();

    GPIOPinConfigure(GPIO_PA4_U3RX);
    GPIOPinConfigure(GPIO_PA5_U3TX);

    /* Configure GPIO pins as UART */
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    /* Led to indicate successful UART init */
    LEDWrite(0x0F, GPIO_PIN_1);

    /* Enable the UART interrupts */
    IntEnable(INT_UART3);
    UARTIntEnable(UART3_BASE, UART_INT_RX | UART_INT_RT);
}

void UART_send(int8_t *pBuffer, uint32_t len)
{
    /* Loop till buffer is empty */
    while(len--)
    {
        UARTCharPutNonBlocking(UART3_BASE, *pBuffer++);
    }
}


void UART_receive(int8_t *pBuffer, uint32_t len)
{
    while(UARTCharsAvail(UART3_BASE) && len--)
    {
        *pBuffer++ = UARTCharGetNonBlocking(UART3_BASE);
    }
}
