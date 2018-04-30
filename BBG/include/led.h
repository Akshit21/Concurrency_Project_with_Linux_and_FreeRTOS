/***************************************************************************************************
* Copyright (C) 2017 by Akshit Shah, Shuting Guo
*
* Redistribution, modification or use of this software in source or binary
* forms is permitted as long as the files maintain this copyright. Users are
* permitted to modify this and use it to learn about the field of embedded
* software. Akshit Shah, Shuting Guo, Prof Alex Fosdick and the University of Colorado are
* not liable for any misuse of this material
***************************************************************************************************/
/***************************************************************************************************
* @author : Akshit Shah, Shuting Guo
* @date : 02/22/2018
*
* @file : led.h
* @brief : This header file provides an abstraction of LED APIs and initialization
           variables.
***************************************************************************************************/
#ifndef LED_H_
#define LED_H_

extern char ledPath[];

/**
* @brief Function to turn on the led
*
* @param ledAddr Led address path for beaglebone
*
* @return Status SUCCES/ERROR
*/
Status_t ledOn(char *ledAddr);

/**
* @brief Function to turn off the led
*
* @param ledAddr Led address path for beaglebone
*
* @return Status SUCCES/ERROR
*/
Status_t ledOff(char *ledAddr);

/**
* @brief Function to blink the led
*
* @param void
*
* @return void
*/
void blinkLED(void);

#endif /* LED_H_ */
