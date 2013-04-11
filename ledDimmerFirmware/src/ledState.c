/*
 * ledState.c
 *
 *  Created on: 15.12.2012
 *      Author: falko
 */

#include "ledState.h"

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

LED_STATE ledStates[2] = { { 0, 255, 1, START_SPEED, 255, 255, 255 }, { 0, 255, 1, START_SPEED, 255,
		255, 255 } };

// pwm_update() needs to be called if values other than program and speed were changed
void setLightValue(uint8_t lightIDMask, uint8_t light, uint8_t value) {
	uint8_t i;
	uint8_t updatePWM = 0;
	for (i = 0; i < 2; i++) {
		if (lightIDMask & (1 << i)) {
			switch (light) {
			case LIGHT_R:
				ledStates[i].rgbProgram = 0;
				ledStates[i].valueR = value;
				if (Device_ID - 1 == i) {
					setPWM(LED_R, (uint16_t)(ledStates[i].valueR) *ledStates[i].rgbBrightness /256);
					updatePWM = 1;
				}
				break;
			case LIGHT_G:
				ledStates[i].rgbProgram = 0;
				ledStates[i].valueG = value;
				if (Device_ID - 1 == i) {
					setPWM(LED_G, (uint16_t)(ledStates[i].valueG) *ledStates[i].rgbBrightness /256);
					updatePWM = 1;
				}
				break;
			case LIGHT_B:
				ledStates[i].rgbProgram = 0;
				ledStates[i].valueB = value;
				if (Device_ID - 1 == i) {
					setPWM(LED_B, (uint16_t)(value) *ledStates[i].rgbBrightness /256);
					updatePWM = 1;
				}
				break;
			case LIGHT_RGB_BRIGHTNESS:
				ledStates[i].rgbBrightness = value;
				if (Device_ID - 1 == i) {
					setPWM(LED_R, (uint16_t)(ledStates[i].valueR) *ledStates[i].rgbBrightness /256);
					setPWM(LED_G, (uint16_t)(ledStates[i].valueG) *ledStates[i].rgbBrightness /256);
					setPWM(LED_B, (uint16_t)(ledStates[i].valueB) *ledStates[i].rgbBrightness /256);
					updatePWM = 1;
				}
				break;
			case LIGHT_PROGRAM:
				if (Device_ID - 1 == i) {
				}
				break;
			}
		}
		if (ledStates[Device_ID - 1].rgbProgram == 0 && updatePWM != 0) {
			pwm_update();
		}
	}
}

uint8_t getLightValue(uint8_t lightID, uint8_t light) {
	switch (light) {
	case LIGHT_R:
		return ledStates[lightID].valueR;
	case LIGHT_G:
		return ledStates[lightID].valueG;
	case LIGHT_B:
		return ledStates[lightID].valueB;
	case LIGHT_RGB_BRIGHTNESS:
		return ledStates[lightID].rgbBrightness;
	case LIGHT_LED_BRIGHTNESS:
		return ledStates[lightID].ledBrightness;
	case LIGHT_PROGRAM:
		return ledStates[lightID].rgbProgram;
	case LIGHT_SPEED:
		return ledStates[lightID].rgbSpeed;
	}
	return 0;
}

// time to the next update of the rgbProgram
uint16_t nextUpdateRGBProgramCall = 1;
// set this variable and then call updateRGBProgram()
uint8_t rgbSpeedOld = START_SPEED;
// the actual position in the rgb program
uint8_t rgbProgramProgress = 0;

const uint16_t speedSteps[SPEED_STEPS] PROGMEM = { 8192, 5792, 4096, 2896, 2048, 1448, 1024, 724,
		512, 362, 256, 181, 128, 90, 64, 45, 32, 22, 16, 11 };

// time between each rgb program update in centiseconds
uint16_t rgbUpdateIntervals[LED_PROGRAM_LENGTH] = { 100, 200, 100, 100, 100, 100, 0 };
// the iteration of the rgb led brightnesses
int8_t rgbProgram[3 * LED_PROGRAM_LENGTH] = { 0, 0, 0, //
		0, 255, 255, //
		0, 255, 255, //
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

// the callback method called 100 times per second
void ledStateCallback() {
	if (--nextUpdateRGBProgramCall == 0) {
		// the next program step needs to be executed
		nextStepRGBProgram();
	}

	if (ledStates[Device_ID - 1].rgbProgram >= LED_NUM_EXTRA_PROGRAMS) { // do not update if no program choosen
		uint8_t pwmChanged = 0; // notice if pwm needs to be updated

		//bresenham

		while (2 * errR > deltaT) {
			errR += deltaT;
			ledStates[Device_ID - 1].valueR += signR;
			pwmChanged = 1;
		}
		errR += deltaR;

		while (2 * errG > deltaT) {
			errG += deltaT;
			ledStates[Device_ID - 1].valueG += signG;
			pwmChanged = 1;
		}
		errG += deltaG;

		while (2 * errB > deltaT) {
			errB += deltaT;
			ledStates[Device_ID - 1].valueB += signB;
			pwmChanged = 1;
		}
		errB += deltaB;

		if (pwmChanged) {
			setPWM(LED_R, ledStates[Device_ID - 1].valueR);
			setPWM(LED_G, ledStates[Device_ID - 1].valueG);
			setPWM(LED_B, ledStates[Device_ID - 1].valueB);
			pwm_update();
		}
	}
}

// goes to the next step of the rgb program
void nextStepRGBProgram() {
	++rgbProgramProgress;
	if (rgbProgramProgress >= LED_PROGRAM_LENGTH || rgbUpdateIntervals[rgbProgramProgress] == 0) {
		rgbProgramProgress = 0;
		if (rgbUpdateIntervals[0] == 0) {
			nextUpdateRGBProgramCall = 0xFFFF;
			return;
		}
	}
	nextUpdateRGBProgramCall = rgbUpdateIntervals[rgbProgramProgress]
			* pgm_read_word(&speedSteps[ledStates[Device_ID - 1].rgbSpeed]) / 256;

	updateRGBProgram();
}

void updateRGBProgram() {
	if (ledStates[Device_ID - 1].rgbSpeed != rgbSpeedOld) {
		nextUpdateRGBProgramCall *= speedSteps[ledStates[Device_ID - 1].rgbSpeed];
		nextUpdateRGBProgramCall /= speedSteps[rgbSpeedOld];
	}
	deltaT = -nextUpdateRGBProgramCall;

	uint8_t newR = rgbProgram[3 * rgbProgramProgress];
	signR = newR > ledStates[Device_ID - 1].valueR ? 1 : -1;
	deltaR =
			signR == 1 ?
					newR - ledStates[Device_ID - 1].valueR : ledStates[Device_ID - 1].valueR - newR;
	errR = deltaR - nextUpdateRGBProgramCall;

	uint8_t newG = rgbProgram[3 * rgbProgramProgress + 1];
	signG = newG > ledStates[Device_ID - 1].valueG ? 1 : -1;
	deltaG =
			signG == 1 ?
					newG - ledStates[Device_ID - 1].valueG : ledStates[Device_ID - 1].valueG - newG;
	errG = deltaG - nextUpdateRGBProgramCall;

	uint8_t newB = rgbProgram[3 * rgbProgramProgress + 2];
	signB = newB > ledStates[Device_ID - 1].valueB ? 1 : -1;
	deltaB =
			signB == 1 ?
					newB - ledStates[Device_ID - 1].valueB : ledStates[Device_ID - 1].valueB - newB;
	errB = deltaB - nextUpdateRGBProgramCall;
}
