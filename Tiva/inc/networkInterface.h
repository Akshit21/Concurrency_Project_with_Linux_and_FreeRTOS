/*
 * networkInterface.h
 *
 *  Created on: Apr 22, 2018
 *      Author: akshit
 */

#ifndef NETWORKINTERFACE_H_
#define NETWORKINTERFACE_H_

void EthernetIntHandler(void);
void processReceivedPacket();
void network_init();
void network_tx(int8_t *pBuffer, uint32_t len);
void network_rx(int8_t *pBuffer, uint32_t len);
uint32_t packet_size();
void vNetworkInterfaceProcess(void *params);
BaseType_t xNetworkInterfaceInitialise();
BaseType_t xNetworkInterfaceOutput(NetworkBufferDescriptor_t *const pxDescriptor, BaseType_t xReleaseAfterSend);

#endif /* NETWORKINTERFACE_H_ */
