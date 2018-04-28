/*
 * socket.c
 *
 *  Created on: Apr 22, 2018
 *      Author: akshit
 */

/* @Ref : https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/index.html
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

#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/gpio.h"
#include "driverlib/comp.h"
#include "driverlib/rom_map.h"
#include "driverlib/flash.h"

#include "driverlib/inc/hw_memmap.h"

#include "main.h"
#include "socket.h"
#include "NetworkInterface.h"

Socket_t my_client_socket;
SemaphoreHandle_t my_bin_sem;

UBaseType_t uxRand()    {return 1;}

const static uint8_t client_mac[6] = {0x00, 0x1A, 0xB6, 0x03, 0x2E, 0x43};
const static uint8_t client_ip[4] = {192,168,2,162};
const static uint8_t client_netmask[4] = {255, 255, 255, 0};
const static uint8_t client_dns[4] = {208, 67, 222, 222};
const static uint8_t client_gateway[4] = {192,168,2,1};

/* Ping hook for FreeRTOS+TCP */
void vApplicationPingReplyHook(ePingReplyStatus_t status, uint16_t id)
{
    LEDWrite(0x0F, GPIO_PIN_2);
    return;
}

/* Network Event Hook */
void vApplicationIPNetworkEventHook(eIPCallbackEvent_t eNetworkEvent)
{
    /* Check this was a network up event */
    if( eNetworkEvent == eNetworkUp )
    {
#ifdef MY_DEBUG
        /* The network is up and configured.  Print out the configuration,
        which may have been obtained from a DHCP server. */
        uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
        int8_t cBuffer[ 16 ];
        FreeRTOS_GetAddressConfiguration( &ulIPAddress,
                                          &ulNetMask,
                                          &ulGatewayAddress,
                                          &ulDNSServerAddress );

        /* Convert the IP address to a string then print it out. */
        FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
        UARTprintf( "IP Address: %s\r\n", cBuffer );

        /* Convert the net mask to a string then print it out. */
        FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
        UARTprintf( "Subnet Mask: %s\r\n", cBuffer );

        /* Convert the IP address of the gateway to a string then print it out. */
        FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
        UARTprintf( "Gateway IP Address: %s\r\n", cBuffer );

        /* Convert the IP address of the DNS server to a string then print it out. */
        FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer );
        UARTprintf( "DNS server IP Address: %s\r\n", cBuffer );

        LEDWrite(0x0F, GPIO_PIN_1);
#endif
        /* Indicate network on with LED or posting a semaphore */
        xSemaphoreGive(my_bin_sem);
    }
}

/* @ref https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/TCP_Networking_Tutorial_TCP_Client_and_Server.html */
/* Tiva Client Socket configuration */
void client_socket(void)
{
    struct freertos_sockaddr xRemoteAddress;
    static const TickType_t xTimeOut = pdMS_TO_TICKS(4000);

    /* Create a client socket */
    my_client_socket = FreeRTOS_socket(FREERTOS_AF_INET,
                                       FREERTOS_SOCK_STREAM,
                                       FREERTOS_IPPROTO_TCP);

    /* Check if we got a valid socket handler */
    ASSERT(my_client_socket != FREERTOS_INVALID_SOCKET);

    /* Set send and receive time outs. */
    FreeRTOS_setsockopt( my_client_socket,
                         0,
                         FREERTOS_SO_RCVTIMEO,
                         &xTimeOut,
                         sizeof(xTimeOut));

    FreeRTOS_setsockopt( my_client_socket,
                         0,
                         FREERTOS_SO_SNDTIMEO,
                         &xTimeOut,
                         sizeof(xTimeOut));

    /* Bind the socket */
    FreeRTOS_bind(my_client_socket, NULL, sizeof(struct freertos_sockaddr));

    /* BBG address */
    xRemoteAddress.sin_addr = FreeRTOS_inet_addr_quick(192,168,0,167);
    xRemoteAddress.sin_port = FreeRTOS_htons(1500);

    while(1)
    {
        if( FreeRTOS_connect(my_client_socket, &xRemoteAddress, sizeof(xRemoteAddress)) == 0 )
        {
            UARTprintf("TCP/IP connection Successful\n");
            break;
        }
    }
    /* Initiate graceful shutdown. */
    FreeRTOS_shutdown( my_client_socket, FREERTOS_SHUT_RDWR );

    vTaskDelay(250);

    /* The socket has shut down and is safe to close. */
    FreeRTOS_closesocket( my_client_socket );
}

uint32_t client_init()
{
    FreeRTOS_IPInit(client_ip, client_netmask, client_gateway, client_dns, client_mac);

#ifdef MY_DEBUG
    uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
    int8_t cBuffer[ 16 ];
    FreeRTOS_GetAddressConfiguration( &ulIPAddress,
                                      &ulNetMask,
                                      &ulGatewayAddress,
                                      &ulDNSServerAddress );

    /* Convert the IP address to a string then print it out. */
    FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
    UARTprintf( "IP Address: %s\r\n", cBuffer );

    /* Convert the net mask to a string then print it out. */
    FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
    UARTprintf( "Subnet Mask: %s\r\n", cBuffer );

    /* Convert the IP address of the gateway to a string then print it out. */
    FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
    UARTprintf( "Gateway IP Address: %s\r\n", cBuffer );

    /* Convert the IP address of the DNS server to a string then print it out. */
    FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer );
    UARTprintf( "DNS server IP Address: %s\r\n", cBuffer );
#endif
    my_bin_sem = xSemaphoreCreateBinary();

    /* Wait for configuration (indicate using semaphore) */
    xSemaphoreTake(my_bin_sem, portMAX_DELAY);
    client_socket();

    LEDWrite(0x0F, GPIO_PIN_0);
    return 1;
}
