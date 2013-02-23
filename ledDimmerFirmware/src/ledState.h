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

#define LED_PROGRAM_LENGTH 20

#define SPEED_STEPS 20

extern uint16_t nextUpdateRGBProgramCall;

void ledStateCallback();
void nextStepRGBProgram();
void updateRGBProgram();

#endif /* LEDSTATE_H_ */
