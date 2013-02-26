/*
 * communication.h
 *
 *  Created on: 25.02.2013
 *      Author: falko
 */

#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "config.h"
#include "uart/uart.h"

#define UART_BAUD_RATE	9600

#define RS485_INIT				(RS485_DIRECTION_PORT |= 1 << RS485_DIRECTION_PIN)
#define RS485_SEND			(RS485_DIRECTION_PORT |= 1 << RS485_DIRECTION_PIN)
#define RS485_RECEIVE		(RS485_DIRECTION_PORT &= ~(1 << RS485_DIRECTION_PIN))

#define BOOTLOADER_START_MESSAGE_TYPE				0x01
#define BOOTLOADER_HEX_MESSAGE_TYPE					0x02
#define BOOTLOADER_ACK_MESSAGE_TYPE					0x03

#define LIGHT_SET_MESSAGE_TYPE								0x07

uint16_t hex2num(const uint8_t * ascii, uint8_t num);

extern uint8_t messageBuffer0[1 + 4 + 24];
extern uint8_t messageLength0;
extern uint8_t messageType0;
extern uint8_t messageNumber0_0;
extern uint8_t messageNumber1_0;
uint8_t decodeMessage0(uint8_t c);

extern uint8_t messageBuffer1[1 + 4 + 24];
extern uint8_t messageLength1;
extern uint8_t messageType1;
extern uint8_t messageNumber1;
extern uint8_t messageNumber1_1;
uint8_t decodeMessage1(uint8_t c);

void initCommunication();

#endif /* COMMUNICATION_H_ */
