/*
 *@file myUART.c
 *
 *@brief UART operation functions
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

/* UART IRQ Handler */
void UARTIntHandler(void)
{
    uint32_t status;

    /* Get the interrupt status */
    status = UARTIntStatus(UART4_BASE, true);

    /* Clear the interrupt */
    UARTIntClear(UART4_BASE, status);

    static uint32_t bytes_recvd = 0;

    /* Get the byte */
    int8_t c = (int8_t)UARTCharGetNonBlocking(UART4_BASE);
    if (c != 0xFF && c != -1)
    {
        *((int8_t*) (&rx) + bytes_recvd) = c;
        bytes_recvd ++;

        if(bytes_recvd == sizeof(msg_packet_t))
        {
            bytes_recvd = 0;
            /* Copy into the buffer */
            memcpy(&rx, 0, sizeof(msg_packet_t));
        }
    }

}

/* Initialize UART module */
int8_t UART_init()
{
    /* Enable the UART module. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART4);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);

    /* Configure UART */
    UARTConfigSetExpClk(UART4_BASE, SYSTEM_CLOCK, BAUD_RATE,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE));

    /* Enable all interrupts */
    IntMasterEnable();

    GPIOPinConfigure(GPIO_PK0_U4RX);
    GPIOPinConfigure(GPIO_PK1_U4TX);

    /* Configure GPIO pins as UART */
    GPIOPinTypeUART(GPIO_PORTK_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /* Led to indicate successful UART init */
    LEDWrite(0x0F, GPIO_PIN_1);

    /* Enable the UART interrupts */
    //IntEnable(INT_UART4);
    //UARTIntEnable(UART4_BASE, UART_INT_RX | UART_INT_RT);

    return 0;
}

/* UART TX */
void UART_send(int8_t *pBuffer, uint32_t len)
{
    int8_t *temp = pBuffer;
    /* Loop till buffer is empty */
    while(len--)
    {
        UARTCharPutNonBlocking(UART4_BASE, *temp++);
    }
}

/* UART RX */
int8_t UART_receive(int8_t *pBuffer, uint32_t len)
{
    int8_t c, flag = 0, *temp = pBuffer;
    /* Read Entire Buffer */
    while(UARTCharsAvail(UART4_BASE) && len)
    {
         c = (int8_t)UARTCharGetNonBlocking(UART4_BASE);
         if ( (c != 0xFF) && (c != -1))
             {*temp++ = c;len--;}

         flag = 1;
    }
    if (flag)
        return 0;
    return -1;
}
