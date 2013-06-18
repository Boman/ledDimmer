/*
 * ledState.c
 *
 *  Created on: 15.12.2012
 *      Author: falko
 */

#include "ledState.h"

LED_STATE ledStates[2] = { { 0, // ledBrightness
		255, // rgbBrightness
		1, // rgbProgram
		START_SPEED, // rgbProgramSpeed
		0, // rValue
		0, // gValue
		0 // bValue
		}, { 0, // ledBrightness
				255, // rgbBrightness
				1, // rgbProgram
				START_SPEED, // rgbProgramSpeed
				0, // rValue
				0, // gValue
				0 // bValue
		} };

// time to the next update of the rgbProgram
volatile uint16_t nextUpdateRGBProgramCall = 1;
// set this variable and then call updateRGBProgram()
volatile uint8_t rgbSpeedOld = START_SPEED;
// the actual position in the rgb program
volatile uint8_t rgbProgramProgress = 0;

#define C_WHITE					255, 255, 255
#define C_BLACK					0, 0, 0
#define C_RED						255, 0, 0
#define C_GREEN					0, 255, 0
#define C_BLUE						0, 0, 255

const uint8_t rgbProgramOffsets[LED_NUM_PROGRAMS + 1] PROGMEM = { 0, // startup program
		1, // auto program
		3, // flash program
		7, // jump3
		13, // jump7
		27, // fade3
		30, // fade7
		37 };

const uint8_t rgbProgram[] PROGMEM = { C_WHITE, // startup program
		C_WHITE, C_BLACK, // auto program
		C_WHITE, C_WHITE, C_BLACK, C_BLACK, // flash program
		C_RED, C_RED, C_GREEN, C_GREEN, C_BLUE, C_BLUE, // jump3
		C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, // jump7
		C_RED, C_GREEN, C_BLUE, // fade3
		C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE // fade7
		};

const uint16_t rgbProgramTimes[] PROGMEM = { 100, // startup program
		100, 100, // auto program
		0, 100, 0, 100, // flash program
		0, 100, 0, 100, 0, 100, // jump3
		0, 100, 0, 100, 0, 100, 0, 100, 0, 100, 0, 100, 0, 100, // jump7
		100, 100, 100, // fade3
		100, 100, 100, 100, 100, 100, 100 // fade7
		};

volatile uint8_t deactivateUpdatePWM = 0;
volatile uint8_t programReady = 0;
volatile uint8_t slaveChanged = 0;

// pwm_update() needs to be called if values other than program and speed were changed
void setLightValue(uint8_t lightIDMask, uint8_t light, uint8_t value) {
	uint8_t i;
	uint8_t updatePWM = 0;
	for (i = 0; i < 2; i++) {
		if (lightIDMask & (1 << i)) {
			switch (light) {
			case LIGHT_RED:
				ledStates[i].rgbProgram = 0;
				ledStates[i].valueR = value;
				if (DEVICE_ID - 1 == i) {
					setPWM( LED_R, (uint16_t)(ledStates[i].valueR) *ledStates[i].rgbBrightness /256);
					updatePWM = 1;
				}
				break;
			case LIGHT_GREEN:
				ledStates[i].rgbProgram = 0;
				ledStates[i].valueG = value;
				if (DEVICE_ID - 1 == i) {
					setPWM( LED_G, (uint16_t)(ledStates[i].valueG) *ledStates[i].rgbBrightness / 256);
					updatePWM = 1;
				}
				break;
			case LIGHT_BLUE:
				ledStates[i].rgbProgram = 0;
				ledStates[i].valueB = value;
				if (DEVICE_ID - 1 == i) {
					setPWM( LED_B, (uint16_t)(value) *ledStates[i].rgbBrightness / 256);
					updatePWM = 1;
				}
				break;
			case LIGHT_RGB_BRIGHTNESS:
				ledStates[i].rgbBrightness = value;
				if (DEVICE_ID - 1 == i) {
					if (ledStates[DEVICE_ID - 1].rgbProgram == 0) {
						setPWM( LED_R, (uint16_t)(ledStates[i].valueR) *ledStates[i].rgbBrightness / 256);
						setPWM( LED_G, (uint16_t)(ledStates[i].valueG) *ledStates[i].rgbBrightness / 256);
						setPWM( LED_B, (uint16_t)(ledStates[i].valueB) *ledStates[i].rgbBrightness / 256);
						updatePWM = 1;
					} else {
						ledStateCallback(0);
					}
				}
				break;
			case LIGHT_PROGRAM:
				ledStates[i].rgbProgram = value;
				if (DEVICE_ID - 1 == i) {
					rgbProgramProgress = pgm_read_word(&rgbProgramOffsets[ledStates[DEVICE_ID - 1].rgbProgram] );
					nextStepRGBProgram(0);
				}
				break;
			case LIGHT_SPEED:
				ledStates[i].rgbSpeed = value;
				if (DEVICE_ID - 1 == i) {
					updateRGBProgram();
				}
				break;
			case LIGHT_LED_BRIGHTNESS:
				ledStates[i].ledBrightness = value;
				if (DEVICE_ID - 1 == i) {
					//TODO
					if (value > 127) {
						LED_OUTPUT_ON;
					} else {
						LED_OUTPUT_OFF;
					}
				}
				break;
			}
		}
		if (ledStates[DEVICE_ID - 1].rgbProgram == 0 && updatePWM != 0 && !deactivateUpdatePWM) {
			pwm_update();
		}
	}
	deactivateUpdatePWM = 0;
}

uint8_t getLightValue(uint8_t lightID, uint8_t light) {
	switch (light) {
	case LIGHT_RED:
		return ledStates[lightID].valueR;
	case LIGHT_GREEN:
		return ledStates[lightID].valueG;
	case LIGHT_BLUE:
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

const uint16_t speedSteps[SPEED_STEPS] PROGMEM = { 8192, 5792, 4096, 2896, 2048, 1448, 1024, 724, 512, 362, 256, 181, 128, 90, 64, 45, 32, 22, 16, 11 };

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
void ledStateCallback(uint8_t reduceCounter) {
	if (reduceCounter && --nextUpdateRGBProgramCall == 0) {
		// the next program step needs to be executed
		nextStepRGBProgram(1);
	}

	if (ledStates[DEVICE_ID - 1].rgbProgram > 0) { // do not update if no program chosen
		uint8_t pwmChanged = 0; // notice if pwm needs to be updated

		//bresenham

		while (2 * errR > deltaT) {
			errR += deltaT;
			ledStates[DEVICE_ID - 1].valueR += signR;
			pwmChanged = 1;
		}
		errR += deltaR;

		while (2 * errG > deltaT) {
			errG += deltaT;
			ledStates[DEVICE_ID - 1].valueG += signG;
			pwmChanged = 1;
		}
		errG += deltaG;

		while (2 * errB > deltaT) {
			errB += deltaT;
			ledStates[DEVICE_ID - 1].valueB += signB;
			pwmChanged = 1;
		}
		errB += deltaB;

		if (pwmChanged) {
			setPWM( LED_R, (uint16_t)(ledStates[DEVICE_ID - 1].valueR)*ledStates [DEVICE_ID - 1].rgbBrightness / 256);
			setPWM( LED_G, (uint16_t)(ledStates[DEVICE_ID - 1].valueG) *ledStates[DEVICE_ID - 1].rgbBrightness / 256);
			setPWM( LED_B, (uint16_t)(ledStates[DEVICE_ID - 1].valueB) *ledStates[DEVICE_ID - 1].rgbBrightness / 256);
			pwm_update();
		}
	}
}

// goes to the next step of the rgb program
void nextStepRGBProgram(uint8_t increasePrgogramProgress) {
	if (ledStates[DEVICE_ID - 1].rgbProgram == 0) {
		nextUpdateRGBProgramCall = 0xFFFF;
		return;
	}

	rgbProgramProgress +=increasePrgogramProgress ;
	if (rgbProgramProgress >= (uint8_t) pgm_read_word(&rgbProgramOffsets[ledStates[DEVICE_ID - 1].rgbProgram])) {
		programReady = 1;
		rgbProgramProgress = pgm_read_word(&rgbProgramOffsets[ledStates[DEVICE_ID - 1].rgbProgram - 1]);
	}
	nextUpdateRGBProgramCall = pgm_read_word(&rgbProgramTimes[rgbProgramProgress] ) * pgm_read_word(&speedSteps[ledStates[DEVICE_ID - 1].rgbSpeed]) / 256;

	updateRGBProgram();
}

// calculates the values for bresenham after new program step or speedchange
void updateRGBProgram() {
	// if the program step has a time of 0
	if (nextUpdateRGBProgramCall == 0) {
		ledStates[DEVICE_ID - 1].valueR = (uint8_t) pgm_read_word(& rgbProgram[3 * rgbProgramProgress]);
		ledStates[DEVICE_ID - 1].valueG = (uint8_t) pgm_read_word(& rgbProgram[3 * rgbProgramProgress+1]);
		ledStates[DEVICE_ID - 1].valueB = (uint8_t) pgm_read_word(& rgbProgram[3 * rgbProgramProgress+2]);
		setPWM( LED_R, (uint16_t)(ledStates[DEVICE_ID - 1].valueR)*ledStates [DEVICE_ID - 1].rgbBrightness / 256);
		setPWM( LED_G, (uint16_t)(ledStates[DEVICE_ID - 1].valueG) *ledStates[DEVICE_ID - 1].rgbBrightness / 256);
		setPWM( LED_B, (uint16_t)(ledStates[DEVICE_ID - 1].valueB) *ledStates[DEVICE_ID - 1].rgbBrightness / 256);
		pwm_update();
		nextStepRGBProgram(1);
		return;
	}

	if (ledStates[DEVICE_ID - 1].rgbSpeed != rgbSpeedOld) {
		nextUpdateRGBProgramCall *= (uint8_t) pgm_read_word(&speedSteps[ledStates[DEVICE_ID - 1].rgbSpeed]);
		nextUpdateRGBProgramCall /= (uint8_t) pgm_read_word(&speedSteps[rgbSpeedOld]);
		rgbSpeedOld = ledStates[DEVICE_ID - 1].rgbSpeed;
	}
	deltaT = -nextUpdateRGBProgramCall;

	uint8_t newR = pgm_read_word(& rgbProgram[3 * rgbProgramProgress]);
	signR = newR > ledStates[DEVICE_ID - 1].valueR ? 1 : -1;
	deltaR = signR == 1 ? newR - ledStates[DEVICE_ID - 1].valueR : ledStates[DEVICE_ID - 1].valueR - newR;
	errR = deltaR - nextUpdateRGBProgramCall;

	uint8_t newG = pgm_read_word(&rgbProgram[3 * rgbProgramProgress + 1]);
	signG = newG > ledStates[DEVICE_ID - 1].valueG ? 1 : -1;
	deltaG = signG == 1 ? newG - ledStates[DEVICE_ID - 1].valueG : ledStates[DEVICE_ID - 1].valueG - newG;
	errG = deltaG - nextUpdateRGBProgramCall;

	uint8_t newB = pgm_read_word(&rgbProgram[3 * rgbProgramProgress + 2]);
	signB = newB > ledStates[DEVICE_ID - 1].valueB ? 1 : -1;
	deltaB = signB == 1 ? newB - ledStates[DEVICE_ID - 1].valueB : ledStates[DEVICE_ID - 1].valueB - newB;
	errB = deltaB - nextUpdateRGBProgramCall;
}
