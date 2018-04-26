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

#include "driverlib/inc/hw_memmap.h"
#include "driverlib/inc/hw_ints.h"

#include "main.h"

void UARTIntHandler(void)
{
    uint32_t status;

    /* Get the interrupt status */
    status = UARTIntStatus(UART1_BASE, true);

    /* Clear the interrupt */
    UARTIntClear(UART1_BASE, status);

    /* Loop while there are characters in the receive FIFO */
    /* ECHO */
    while(UARTCharsAvail(UART1_BASE))
    {
        UARTCharPutNonBlocking(UART1_BASE,UARTCharGetNonBlocking(UART1_BASE));
        static uint32_t blink_led = GPIO_PIN_0;
        blink_led ^= (GPIO_PIN_0);
        LEDWrite(0x0F, blink_led);
    }
}

void UART_init()
{
    /* Enable the UART module. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    /* Configure UART */
    UARTConfigSetExpClk(UART1_BASE, SYSTEM_CLOCK, BAUD_RATE,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE));

    /* Enable all interrupts */
    IntMasterEnable();

    /* Configure GPIO pins as UART */
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /* Enable the UART interrupts */
    IntEnable(INT_UART1);
    UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);

    /* Led to indicate successful UART init */
    static uint32_t blink_led = GPIO_PIN_1;
    LEDWrite(0x0F, blink_led);
}

void UART_send(const uint8_t *pBuffer, uint32_t len)
{
    /* Loop till buffer is empty */
    while(len--)
    {
        UARTCharPutNonBlocking(UART1_BASE, *pBuffer++);
    }
}


void UART_receive(char *pBuffer, uint32_t len)
{
    while(UARTCharsAvail(UART1_BASE) && len--)
    {
        *pBuffer++ = UARTCharGetNonBlocking(UART1_BASE);
    }
}
