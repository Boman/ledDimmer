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
#include "config.h"
#include "ledDimmer.h"

#define LIGHT_R										0x02
#define LIGHT_G										0x03
#define LIGHT_B										0x04
#define LIGHT_RGB_BRIGHTNESS		0x05
#define LIGHT_LED_BRIGHTNESS		0x06
#define LIGHT_PROGRAM						0x07
#define LIGHT_SPEED							0x08

void setLightValue(uint8_t lightIDMask, uint8_t light, uint8_t value);
uint8_t getLightValue(uint8_t lightID, uint8_t light);

#define LED_NUM_EXTRA_PROGRAMS			3
#define LED_PROGRAM_LENGTH					20

#define SPEED_STEPS				20
#define START_SPEED				(SPEED_STEPS / 2 - 2)

void ledStateCallback();
void nextStepRGBProgram();
void updateRGBProgram();

#endif /* LEDSTATE_H_ */
