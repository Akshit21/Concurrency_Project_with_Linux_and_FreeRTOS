/*
 * socket.h
 *
 *  Created on: Apr 22, 2018
 *      Author: akshit
 */
#ifndef SOCKET_H_
#define SOCKET_H_

/**
 * @brief Initialize local IP Module for communication
 *
 * @param None
 *
 * @return 0 - Success
 *         -1 - Error
 */
uint32_t client_init();

/**
 * @brief Initialize Client Socket Module for communication
 *
 * @param None
 *
 * @return None
 */
void client_socket(void);

#endif
