/*
 * @file ethernet.c
 *
 * @brief Ethernet Utility Functions for TCP/IP
 *
 *  Created on: Apr 27, 2018
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

/* Interrupt Handler */
void EthernetIntHandler(void)
{
    uint32_t temp;
    /*  Read and Clear the interrupt */
    temp = EMACIntStatus(EMAC0_BASE, true);

    EMACIntClear(EMAC0_BASE, temp);

    /* Notify the waiting task  */
    processReceive();
}

/* Network Initialization */
void network_init()
{
    uint8_t ethMACaddr[6];
    uint32_t ui32User0,ui32User1;

    FlashUserGet(&ui32User0, &ui32User1);
    if((ui32User0 == 0xffffffff) || (ui32User1 == 0xffffffff))
    {
        /* Error Condition */
        /* Setting Mac address which is on board */
        static const uint8_t ethMACaddr1[6] = {0x00, 0x1A, 0xB6, 0x03, 0x2E, 0x43};
        memcpy(ethMACaddr,ethMACaddr1,sizeof(ethMACaddr1));
    }
    else
    {
        ethMACaddr[0] = ((ui32User0 >> 0) & 0xff);
        ethMACaddr[1] = ((ui32User0 >> 8) & 0xff);
        ethMACaddr[2] = ((ui32User0 >> 16) & 0xff);
        ethMACaddr[3] = ((ui32User1 >> 0) & 0xff);
        ethMACaddr[4] = ((ui32User1 >> 8) & 0xff);
        ethMACaddr[5] = ((ui32User1 >> 16) & 0xff);
    }

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
   uint16_t i;
   for (i = 0; i < NUM_TX_DESCRIPTORS; i++)
   {
       ethTxDescriptor[i].ui32Count = (DES1_TX_CTRL_SADDR_INSERT);
       ethTxDescriptor[i].pvBuffer1 = ethTxBuffer[i];
       ethTxDescriptor[i].DES3.pLink = (i == (NUM_TX_DESCRIPTORS - 1)) ? ethTxDescriptor : &ethTxDescriptor[i+1];
       ethTxDescriptor[i].ui32CtrlStatus = (DES0_TX_CTRL_LAST_SEG | DES0_TX_CTRL_FIRST_SEG |
                                              DES0_TX_CTRL_INTERRUPT | DES0_TX_CTRL_CHAINED |
                                              DES0_TX_CTRL_IP_ALL_CKHSUMS);
   }

   for (i = 0; i < NUM_RX_DESCRIPTORS; i++)
   {
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
   uint16_t index;
   for (index = 0; index < NUM_RX_DESCRIPTORS; index++)
   {
       ethRxDescriptor[index].ui32CtrlStatus |= DES0_RX_CTRL_OWN;
   }

   /* Enable Ethernet MAC TX/RX */
   EMACTxEnable(EMAC0_BASE);
   EMACRxEnable(EMAC0_BASE);

   /* Set Ethernet interrupt priority */
   IntPrioritySet(INT_EMAC0, 0xE0);

   /* Enable Ethernet interrupt */
   IntEnable(INT_EMAC0);
   EMACIntEnable(EMAC0_BASE, EMAC_INT_RECEIVE);
}

/* Ethernet Network Transmit API */
void network_tx(uint8_t *pBuffer, uint32_t len)
{
    ethTxDescIndex += 1;

    /* Basic Sanity Check */
    if (ethTxDescIndex >= NUM_TX_DESCRIPTORS)
        ethTxDescIndex = 0;

    /* Wait for Tx Data */
    while (ethTxDescriptor[ethTxDescIndex].ui32CtrlStatus & DES0_TX_CTRL_OWN);

    /* Fill the Tx Buffer with the input Data */
    uint32_t i;
    for(i = 0; i < len; i++)
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
uint32_t network_rx(uint8_t *pBuffer, uint32_t len)
{
    uint32_t packet_len = 0;

    /* Make sure if we own the receive descriptor */
    if(!(ethRxDescriptor[ethRxDescIndex].ui32CtrlStatus & DES0_RX_CTRL_OWN))
    {
        if((!(ethRxDescriptor[ethRxDescIndex].ui32CtrlStatus & DES0_RX_STAT_ERR)) &&
          (ethRxDescriptor[ethRxDescIndex].ui32CtrlStatus & DES0_RX_STAT_LAST_DESC))
        {
            /* Received Packet length */
            packet_len = ((ethRxDescriptor[ethRxDescIndex].ui32CtrlStatus
                         & DES0_RX_STAT_FRAME_LENGTH_M) >> DES0_RX_STAT_FRAME_LENGTH_S);
            /* Process the frame i.e copying into the buffer parameter */
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

    return packet_len;
}

/* RX packet size */
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
