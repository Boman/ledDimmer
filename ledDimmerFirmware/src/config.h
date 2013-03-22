/*
 * config.h
 *
 *  Created on: 17.12.2012
 *      Author: falko
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#ifdef MASTER
#define Device_ID 1
#define IR_DEVICE
#elif defined SLAVE
#define  Device_ID 2
#else
#error "choose Target MASTER or SLAVE"
#endif

#define RS485_DIRECTION_DDR		DDRB
#define RS485_DIRECTION_PORT		PORTB
#define RS485_DIRECTION_PIN			PB4

#define F_INTERRUPTS					15000
#define INTERRUPTS_COUNT		 (F_INTERRUPTS / 100)

#define IR_ADRESS						 0xFF00

// Command Codes from the IR-Remote-Control
#define IR_RGB_DARKER							92
#define IR_RGB_BRIGHTER						93

#define IR_PLAY											65
#define IR_ON_OFF									64

#define IR_COLOR_RED							88
#define IR_COLOR_GREEN						89
#define IR_COLOR_BLUE							69
#define IR_COLOR_WHITE						68
#define IR_COLOR_DARK_RED				84
#define IR_COLOR_DARK_GREEN			85
#define IR_COLOR_DARK_BLUE				73
#define IR_COLOR_LIGHT_PINK				72
#define IR_COLOR_ORANGE					80
#define IR_COLOR_CYAN							81
#define IR_COLOR_MIDNIGHT_BLUE		77
#define IR_COLOR_PINK							76
#define IR_COLOR_LIGHT_YELLOW		28
#define IR_COLOR_JUNGLE_GREEN		29
#define IR_COLOR_VIOLET						30
#define IR_COLOR_SKY_BLUE					31
#define IR_COLOR_YELLOW					24
#define IR_COLOR_COBALT						25
#define IR_COLOR_MAGENTA					26
#define IR_COLOR_LIGHT_BLUE				27

#define IR_RED_BRIGHTER						20
#define IR_GREEN_BRIGHTER					21
#define IR_BLUE_BRIGHTER						22
#define IR_RED_DARKER							16
#define IR_GREEN_DARKER						17
#define IR_BLUE_DARKER							18

#define IR_QUICKER									23
#define IR_SLOWER									19

#define IR_DIY1											12
#define IR_DIY2											13
#define IR_DIY3											14
#define IR_DIY4											8
#define IR_DIY5											9
#define IR_DIY6											10

#define IR_AUTO										15
#define IR_FLASH										11
#define IR_JUMP3										4
#define IR_JUMP7										5
#define IR_FADE3										6
#define IR_FADE7										7

#endif /* CONFIG_H_ */
