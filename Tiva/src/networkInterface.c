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

#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/gpio.h"
#include "driverlib/comp.h"
#include "driverlib/rom_map.h"
#include "drivelib/emac.h"

#include "driverlib/inc/hw_memmap.h"

#include "main.h

/* TX/RX Descriptors */
#define NUM_TX_DESCRIPTORS (3)
#define NUM_RX_DESCRIPTORS (3)

tEMACDMADescriptor ethRxDescriptor[NUM_TX_DESCRIPTORS];
tEMACDMADescriptor ethTxDescriptor[NUM_RX_DESCRIPTORS];

uint32_t ethRxDescIndex;
uint32_t ethTxDescIndex;

/* Network buffer sizes */
#define NETWORK_RX_BUF_SIZE (1536)
#define NETWORK_TX_BUF_SIZE (1536)

uint8_t ethRxBuffer[NUM_RX_DESCRIPTORS][NETWORK_RX_BUF_SIZE];
uint8_t ethTxBuffer[NUM_TX_DESCRIPTORS][NETWORK_TX_BUF_SIZE];

/* TIVA MAC Address */
const static uint8_t ethMACaddr[6] = {0x00, 0x1A, 0xB6, 0x03, 0x2E, 0x43};

/* Network Process Task Handle */
TaskHandle_t xNetworkInterfaceProcessHandle = NULL;

/* Interrupt Handler */
void EthernetIntHandler(void)
{
    uint32_t temp;
    /*  Read and Clear the interrupt */
    temp = EMACIntStatus(EMAC0_BASE, true);
    EMACIntClear(EMAC0_BASE, temp);

    /* Process the packet */
    processReceivedPacket();
}

/* Processing Received Packet */
void processReceivedPacket()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xNetworkInterfaceProcessHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* Network Initialization */
void network_init()
{
    /* Enable Ethernet Modules */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EMAC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EPHY0);
    SysCtlPeripheralReset(SYSCTL_PERIPH_EMAC0);
    SysCtlPeripheralReset(SYSCTL_PERIPH_EPHY0);

    /* Wait for MAC to start */
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_EMAC0));

    /* Configure internal PHY */
    EMACPHYConfigSet(EMAC0_BASE, (EMAC_PHY_TYPE_INTERNAL |
                     EMAC_PHY_INT_MDIX_EN | EMAC_PHY_AN_100B_T_FULL_DUPLEX));

   /* Initialize the Ethernet MAC and set DMA mode */
   EMACReset(EMAC0_BASE);
   EMACInit(EMAC0_BASE,
            SYSTEM_CLOCK,
            EMAC_BCONFIG_MIXED_BURST | EMAC_BCONFIG_PRIORITY_FIXED,
            4, 4, 0);

   /* Configure Ethernet MAC */
   EMACConfigSet(EMAC0_BASE,
                (EMAC_CONFIG_FULL_DUPLEX | EMAC_CONFIG_CHECKSUM_OFFLOAD |
                 EMAC_CONFIG_7BYTE_PREAMBLE | EMAC_CONFIG_IF_GAP_96BITS |
                 EMAC_CONFIG_USE_MACADDR0 | EMAC_CONFIG_SA_FROM_DESCRIPTOR |
                 EMAC_CONFIG_BO_LIMIT_1024),
                (EMAC_MODE_RX_STORE_FORWARD | EMAC_MODE_TX_STORE_FORWARD |
                 EMAC_MODE_TX_THRESHOLD_64_BYTES | EMAC_MODE_RX_THRESHOLD_64_BYTES), 0);

   /* Init DMA descriptors */
   for (uint16_t i = 0; i < NUM_TX_DESCRIPTORS; i++) {
       ethTxDescriptor[i].ui32Count = (DES1_TX_CTRL_SADDR_INSERT);
       ethTxDescriptor[i].pvBuffer1 = ethTxBuffer[i];
       ethTxDescriptor[i].DES3.pLink = (i == (NUM_TX_DESCRIPTORS - 1)) ?
                                          ethTxDescriptor : &ethTxDescriptor[i+1];
       ethTxDescriptor[i].ui32CtrlStatus = (DES0_TX_CTRL_LAST_SEG | DES0_TX_CTRL_FIRST_SEG |
                                              DES0_TX_CTRL_INTERRUPT | DES0_TX_CTRL_CHAINED |
                                              DES0_TX_CTRL_IP_ALL_CKHSUMS);
   }

   for (int i = 0; i < NUM_RX_DESCRIPTORS; i++) {
       ethRxDescriptor[i].ui32CtrlStatus = 0;
       ethRxDescriptor[i].ui32Count = (DES1_RX_CTRL_CHAINED |
                                        (NETWORK_RX_BUF_SIZE << DES1_RX_CTRL_BUFF1_SIZE_S));
       ethRxDescriptor[i].pvBuffer1 = ethRxBuffer[i];
       ethRxDescriptor[i].DES3.pLink = (i == (NUM_RX_DESCRIPTORS - 1)) ?
               ethRxDescriptor : &ethRxDescriptor[i+1];
    }

   EMACRxDMADescriptorListSet(EMAC0_BASE, ethRxDescriptor);
   EMACTxDMADescriptorListSet(EMAC0_BASE, ethTxDescriptor);

   ethRxDescIndex = 0;
   ethTxDescIndex = 0;

   /* Program the hardware with MAC address */
   EMACAddrSet(EMAC0_BASE, 0, ethMACaddr);

   /* Wait for link to be active */
   while((EMACPHYRead(EMAC0_BASE, 0, EPHY_BMSR) & EPHY_BMSR_LINKSTAT) == 0) {}

   /* Set MAC filtering options */
   EMACFrameFilterSet(EMAC0_BASE,
                     (EMAC_FRMFILTER_SADDR |
                      EMAC_FRMFILTER_PASS_MULTICAST |
                      EMAC_FRMFILTER_PASS_NO_CTRL));

   /* Clear pending Interrupts */
   EMACIntClear(EMAC0_BASE, EMACIntStatus(EMAC0_BASE, false));

   /* Mark the receive descriptors as available to the DMA to start
        the receive processing. */
   for (uint16_t index = 0; index < NUM_RX_DESCRIPTORS; index++)
   {
       ethRxDescriptor[index].ui32CtrlStatus |= DES0_RX_CTRL_OWN;
   }

   /* Enable Ethernet MAC TX/RX */
   EMACTxEnable(EMAC0_BASE);
   EMACRxEnable(EMAC0_BASE);

   /* Set Ethernet interrupt priority */
   IntPrioritySet(INT_EMAC0, (7<<5));

   /* Enable Ethernet interrupt */
   IntEnable(INT_EMAC0);
   EMACIntEnable(EMAC0_BASE, EMAC_INT_RECEIVE);
}

/* Ethernet Network Transmit API */
void network_tx(int8_t *pBuffer, uint32_t len)
{
    ethTxDescIndex += 1;

    /* Basic Sanity Check */
    if (ethTxDescIndex >= NUM_TX_DESCRIPTORS)
        ethTxDescIndex = 0;

    /* Wait for Tx Data */
    while (ethTxDescriptor[ethTxDescIndex].ui32CtrlStatus & DES0_TX_CTRL_OWN);

    /* Fill the Tx Buffer with the input Data */
    for(uint32_t i = 0; i < len; i++)
    {
        ethTxBuffer[ethTxDescIndex][i] = pBuffer[i];
    }

    /* Create a packet with buffer and tell TX to start */
    ethTxDescriptor[ethTxDescIndex].ui32Count = (DES1_TX_CTRL_BUFF1_SIZE_M &
                                                     (len << DES1_TX_CTRL_BUFF1_SIZE_S));

    ethTxDescriptor[ethTxDescIndex].pvBuffer1 = ethTxBuffer[ethTxDescIndex];
    ethRxDescriptor[ethTxDescIndex].pvBuffer1 = ethTxBuffer[ethTxDescIndex];
    ethTxDescriptor[ethTxDescIndex].ui32CtrlStatus =
            (DES0_TX_CTRL_LAST_SEG | DES0_TX_CTRL_FIRST_SEG |
             DES0_TX_CTRL_INTERRUPT | DES0_TX_CTRL_IP_ALL_CKHSUMS |
             DES0_TX_CTRL_CHAINED | DES0_TX_CTRL_OWN);

    /* Start DMA Polling */
    EMACTxDMAPollDemand(EMAC0_BASE);
}

/* Ethernet Network Receive API */
void network_rx(int8_t *pBuffer, uint32_t len)
{
    /* Check if we own the receive descriptor */
    if(!(ethRxDescriptor[ethRxDescIndex].ui32CtrlStatus & DES0_RX_CTRL_OWN))
    {
        if((!(ethRxDescriptor[ethRxDescIndex].ui32CtrlStatus & DES0_RX_STAT_ERR)) &&
          (ethRxDescriptor[ethRxDescIndex].ui32CtrlStatus & DES0_RX_STAT_LAST_DESC))
        {
            /* Process the frame */
            memcpy(pBuffer, ethRxDescriptor[ethRxDescIndex].pvBuffer1, len);
        }

        ethRxDescriptor[ethRxDescIndex].ui32CtrlStatus = DES0_RX_CTRL_OWN;

        /* Move on to next descriptor */
        ethRxDescIndex += 1;

        /* Basic Sanity Check */
        if (ethRxDescIndex >= NUM_RX_DESCRIPTORS)
        {
            ethRxDescIndex = 0;
        }
    }
}

uint32_t packet_size()
{
    /* Check if we own the receive descriptor */
    if((!(ethRxDescriptor[ethRxDescIndex].ui32CtrlStatus & DES0_RX_CTRL_OWN)) &&
      (!(ethRxDescriptor[ethRxDescIndex].ui32CtrlStatus & DES0_RX_STAT_ERR)) &&
      (ethRxDescriptor[ethRxDescIndex].ui32CtrlStatus & DES0_RX_STAT_LAST_DESC))
    {
        return  ((ethRxDescriptor[ethRxDescIndex].ui32CtrlStatus
                & DES0_RX_STAT_FRAME_LENGTH_M) >> DES0_RX_STAT_FRAME_LENGTH_S);
    }

    return 0;
}

/* Network Interface Task Handler */
void vNetworkInterfaceProcess(void *params)
{
    NetworkBufferDescriptor_t *pxBufferDescriptor;
    size_t xBytesRecieved;
    IPStackEvent_t xRxEvent;

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        xBytesRecieved = packet_size();

        if (xBytesRecieved)
        {
            pxBufferDescriptor = pxGetNetworkBufferWithDescriptor(xBytesRecieved, 0);

            if (pxBufferDescriptor != NULL)
            {
                pxBufferDescriptor->xDataLength = network_rx(pxBufferDescriptor->pucEthernetBuffer, xBytesRecieved);
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
    vTaskDelay(200);

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
