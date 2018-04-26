/*
 * myUART.h
 *
 *  Created on: Apr 22, 2018
 *      Author: akshit
 */

#ifndef MYUART_H_
#define MYUART_H_

void UART_init();
void UART_send(const uint8_t *pBuffer, uint32_t len);
void UART_receive(char *pBuffer, uint32_t len);

#endif /* MYUART_H_ */
