/*
 * myUART.h
 *
 *  Created on: Apr 22, 2018
 *      Author: akshit
 */

#ifndef MYUART_H_
#define MYUART_H_

/**
 * @brief Initialize UART Module for communication
 *
 * @param none
 *
 * @return none
 */
void UART_init();

/**
 * @brief UART TX Module for communication
 *
 * @param pBuffer - pointer to the buffer data to be sent
 *        len - len of the buffer
 *
 * @return none
 */
void UART_send(int8_t *pBuffer, uint32_t len);

/**
 * @brief UART RX Module for communication
 *
 * @param pBuffer - pointer to the buffer data to be sent
 *        len - len of the buffer
 *
 * @return 0 - Success
 *         -1 - Error
 */
int8_t UART_receive(int8_t *pBuffer, uint32_t len);

#endif /* MYUART_H_ */
