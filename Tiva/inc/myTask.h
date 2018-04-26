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

void noise_sensor_task(void *params);
void motion_sensor_task(void *params);
void dummy_task(void *params);

#endif /* MYTASK_H_ */
