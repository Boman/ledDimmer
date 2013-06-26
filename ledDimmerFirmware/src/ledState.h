/*
 * ledState.h
 *
 *  Created on: 14.02.2013
 *  Author: falko
 */

#ifndef LEDSTATE_H_
#define LEDSTATE_H_

#include <stdint.h>
#include <avr/pgmspace.h>
#include "config.h"
#include "ledDimmer.h"

#define LIGHT_RED                     0x02
#define LIGHT_GREEN                   0x03
#define LIGHT_BLUE                    0x04
#define LIGHT_RGB_BRIGHTNESS          0x05
#define LIGHT_LED_BRIGHTNESS          0x06
#define LIGHT_PROGRAM                 0x07
#define LIGHT_SPEED                   0x08

typedef struct {
    //general settings for the white led
    uint8_t ledBrightness;

    // general settings for the rgb leds
    uint8_t rgbBrightness;
    uint8_t rgbProgram;
    uint8_t rgbSpeed;
    uint8_t valueR;
    uint8_t valueG;
    uint8_t valueB;
} LED_STATE;

extern volatile uint8_t deactivateUpdatePWM;
extern volatile uint8_t programReady;
extern volatile uint8_t slaveChanged;

void setLightValue(uint8_t lightIDMask, uint8_t light, uint8_t value);
uint8_t getLightValue(uint8_t lightID, uint8_t light);

#define LED_NUM_PROGRAMS              7
//#define LED_PROGRAM_LENGTH          20

#define SPEED_STEPS                   20
#define START_SPEED                   (SPEED_STEPS / 2 - 2)

void ledStateCallback(uint8_t reduceCounter);
void nextStepRGBProgram(uint8_t increasePrgogramProgress);
void updateRGBProgram();

#endif /* LEDSTATE_H_ */
