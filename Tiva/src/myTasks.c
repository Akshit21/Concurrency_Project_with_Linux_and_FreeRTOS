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

TaskHandle_t dummyHandle;

/* Noise Sensor Processing */
void noise_sensor_task(void *params)
{
    while(1)
    {
        if(ComparatorValueGet(COMP_BASE, 0)) /* If there is noise */
        {
#ifdef MY_DEBUG  /* Blink LED if there is noise */
            static uint32_t blink_led = GPIO_PIN_0;
            blink_led ^= (GPIO_PIN_0);
            LEDWrite(0x0F, blink_led);
            UARTprintf("NOISE !!!!\n");
#endif
            /* We can use task notify or queue send */
            //xTaskNotify(dummyHandle,NOISE_ALERT,eSetBits);
            UARTprintf("NOISE !!!!\n");
        }
        vTaskDelay( 1000 / portTICK_PERIOD_MS ); // wait for one second
    }
}


/* Motion Sensor Processing */
void motion_sensor_task(void *params)
{
    while(1)
    {
        if(ComparatorValueGet(COMP_BASE, 1)) /* If there is motion */
        {
#ifdef MY_DEBUG  /* Blink LED if there is motion */
            static uint32_t blink_led = GPIO_PIN_1;
            blink_led ^= (GPIO_PIN_1);
            LEDWrite(0x0F, blink_led);
            UARTprintf("STRANGER ALERT !!!\n");
#endif
            /* We can use task notify or queue send */
            //xTaskNotify(dummyHandle,MOTION_ALERT,eSetBits);
            UARTprintf("STRANGER ALERT !!!\n");
        }
        vTaskDelay( 1000 / portTICK_PERIOD_MS ); // wait for one second
    }
}

/* Dummy Notification processor handler */
void dummy_task(void *params)
{
    uint32_t notify = 0;

    while(1)
    {
        if( xTaskNotifyWait( pdFALSE,      /* Don't clear bits on entry. */
                             ULONG_MAX,    /* Clear all bits on exit. */
                             &notify,      /* Stores the notified value. */
                             pdMS_TO_TICKS(1000)) == pdPASS )
        {
            if(notify & NOISE_ALERT)
            {
                /* Handle Noise Sensor Processing */
                UARTprintf("DUMMY NOISE !!!!\n");
            }

            if(notify & MOTION_ALERT)
            {
                /* Handle Motion Sensor Processing */
                UARTprintf("DUMMY STRANGER ALERT !!!\n");
            }
        }
    }
}
