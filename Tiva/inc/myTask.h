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
void interface_task(void *params);
void AnalogComparatorInit(void);
void init_queue();

#endif /* MYTASK_H_ */
