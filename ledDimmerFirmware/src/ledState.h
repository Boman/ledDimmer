/*
 * ledState.h
 *
 *  Created on: 14.02.2013
 *      Author: falko
 */

#ifndef LEDSTATE_H_
#define LEDSTATE_H_

#include <stdint.h>
#include <avr/pgmspace.h>
#include "ledDimmer.h"

#define LIGHT_ALL_R						1
#define LIGHT_ALL_G						2
#define LIGHT_ALL_B						3
#define LIGHT_ALL_INTENSITY			4
#define LIGHT_ALL_LED					5
#define LIGHT_ALL_PROGRAM			6
#define LIGHT_ALL_SPEED				7

#define LIGHT_1_R							11
#define LIGHT_1_G							12
#define LIGHT_1_B							13
#define LIGHT_1_LED						14
#define LIGHT_1_INTENSITY			15
#define LIGHT_1_PROGRAM				16
#define LIGHT_1_SPEED					17

#define LIGHT_2_R							21
#define LIGHT_2_G							22
#define LIGHT_2_B							23
#define LIGHT_2_LED						24
#define LIGHT_2_INTENSITY			25
#define LIGHT_2_PROGRAM				26
#define LIGHT_2_SPEED					27

void setLightValue(uint8_t light, uint8_t value);

#define LED_PROGRAM_LENGTH 20

#define SPEED_STEPS 20

void ledStateCallback();
void nextStepRGBProgram();
void updateRGBProgram();

#endif /* LEDSTATE_H_ */
