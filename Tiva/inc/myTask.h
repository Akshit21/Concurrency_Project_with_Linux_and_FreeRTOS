/*
 * myTask.h
 *
 *  Created on: Apr 22, 2018
 *      Author: akshit
 */

#ifndef MYTASK_H_
#define MYTASK_H_

/* Macros for Notifications */
#define NOISE_ALERT     (0x01)
#define MOTION_ALERT    (0x02)
#define HEARTBEAT_NOISE (0x03)
#define HEARTBEAT_MOTION (0x04)
#define HEARTBEAT_REQ1  (0x05)
#define HEARTBEAT_REQ2  (0x05)

/* Task Handlers */
void noise_sensor_task(void *params);
void motion_sensor_task(void *params);
void interface_task(void *params);
void hb_task(void *params);

/**
 * @brief Initialize Analog Comparator Module for sensors
 *
 * @param none
 *
 * @return none
 */
void AnalogComparatorInit(void);

/**
 * @brief Initialize Queue and Synchronization Modules
 *
 * @param none
 *
 * @return none
 */
void init_queue();

#endif /* MYTASK_H_ */
