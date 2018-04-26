/*
 * socket.h
 *
 *  Created on: Apr 22, 2018
 *      Author: akshit
 */


uint32_t client_init();
void client_socket(void);
void vApplicationIPNetworkEventHook(eIPCallbackEvent_t eNetworkEvent);
