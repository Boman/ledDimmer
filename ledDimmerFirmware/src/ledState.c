/*
 * ledState.c
 *
 *  Created on: 15.12.2012
 *      Author: falko
 */

#include "ledState.h"

//general settings for the white led
uint8_t ledBrightness = 255;

// general settings for the rgb leds
uint8_t rgbBrightness = 255;
uint8_t valueR = 0;
uint8_t valueG = 0;
uint8_t valueB = 0;

// time to the next update of the rgbProgram
uint16_t nextUpdateRGBProgramCall = 1;
// speed of the program running
uint8_t rgbSpeed = SPEED_STEPS / 2 - 2;
// set this variable and then call updateRGBProgram()
uint8_t rgbSpeedNew = SPEED_STEPS / 2 - 2;
// the actual position in the rgb program
uint8_t rgbProgramPointer = 0;

const uint16_t speedSteps[SPEED_STEPS] PROGMEM = { 8192, 5792, 4096, 2896, 2048, 1448, 1024, 724,
		512, 362, 256, 181, 128, 90, 64, 45, 32, 22, 16, 11 };

// time between each rgb program update in centiseconds
uint16_t rgbUpdateIntervals[LED_PROGRAM_LENGTH] = { 100, 100, 100, 100, 100, 0 };
// the iteration of the rgb led brightnesses
int8_t rgbProgram[3 * LED_PROGRAM_LENGTH] = { 0, 0, 0, //
		0, 63, 0, //
		0, 127, 63, //
		63, 63, 0, //
		63, 0, 0 };

// bresenham variables
int16_t deltaT;
int8_t signR;
int16_t errR;
uint8_t deltaR;
int8_t signG;
int16_t errG;
uint8_t deltaG;
int8_t signB;
int16_t errB;
uint8_t deltaB;

void setLightValue(uint8_t light, uint8_t value) {
	switch (light) {
	case LIGHT_ALL_R:
		rgbUpdateIntervals[0] = 0;
		valueR = value;
		setPWM(LED_R, valueR);
		pwm_update();
		break;
	case LIGHT_ALL_G:
		rgbUpdateIntervals[0] = 0;
		valueG = value;
		setPWM(LED_G, valueG);
		pwm_update();
		break;
	case LIGHT_ALL_B:
		rgbUpdateIntervals[0] = 0;
		valueB = value;
		setPWM(LED_B, valueB);
		pwm_update();
		break;
	}
}

// the callback method called 100 times per second
void ledStateCallback() {
	if (--nextUpdateRGBProgramCall == 0) {
		nextStepRGBProgram();
	}
	if (rgbUpdateIntervals[0] != 0) {
		uint8_t pwmChanged = 0;

		//bresenham

		while (2 * errR > deltaT) {
			errR += deltaT;
			valueR += signR;
			pwmChanged = 1;
		}
		errR += deltaR;

		while (2 * errG > deltaT) {
			errG += deltaT;
			valueG += signG;
			pwmChanged = 1;
		}
		errG += deltaG;

		while (2 * errB > deltaT) {
			errB += deltaT;
			valueB += signB;
			pwmChanged = 1;
		}
		errB += deltaB;

		if (pwmChanged) {
			setPWM(LED_R, valueR);
			setPWM(LED_G, valueG);
			setPWM(LED_B, valueB);
			pwm_update();
		}
	}
}

// goes to the next step of the rgb program
void nextStepRGBProgram() {
	++rgbProgramPointer;
	if (rgbProgramPointer >= LED_PROGRAM_LENGTH || rgbUpdateIntervals[rgbProgramPointer] == 0) {
		rgbProgramPointer = 0;
		if (rgbUpdateIntervals[0] == 0) {
			nextUpdateRGBProgramCall = 0xFFFF;
			return;
		}
	}
	nextUpdateRGBProgramCall = rgbUpdateIntervals[rgbProgramPointer]
			* pgm_read_word(&speedSteps[rgbSpeed]) / 256;

	updateRGBProgram();
}

void updateRGBProgram() {
	if (rgbSpeed != rgbSpeedNew) {
		nextUpdateRGBProgramCall *= speedSteps[rgbSpeedNew] / speedSteps[rgbSpeed];
	}
	deltaT = -nextUpdateRGBProgramCall;

	uint8_t newR = rgbProgram[3 * rgbProgramPointer];
	signR = newR > valueR ? 1 : -1;
	deltaR = signR == 1 ? newR - valueR : valueR - newR;
	errR = deltaR - nextUpdateRGBProgramCall;

	uint8_t newG = rgbProgram[3 * rgbProgramPointer + 1];
	signG = newG > valueG ? 1 : -1;
	deltaG = signG == 1 ? newG - valueG : valueG - newG;
	errG = deltaG - nextUpdateRGBProgramCall;

	uint8_t newB = rgbProgram[3 * rgbProgramPointer + 2];
	signB = newB > valueB ? 1 : -1;
	deltaB = signB == 1 ? newB - valueB : valueB - newB;
	errB = deltaB - nextUpdateRGBProgramCall;
}
