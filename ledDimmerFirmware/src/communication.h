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

#define UART_BAUD_RATE	       9600
#define UART_BAUD_RATE_BIDI	   38400

#define RS485_INIT				(RS485_DIRECTION_DDR |= 1 << RS485_DIRECTION_PIN)
#define RS485_SEND			(RS485_DIRECTION_PORT |= 1 << RS485_DIRECTION_PIN)
#define RS485_RECEIVE		(RS485_DIRECTION_PORT &= ~(1 << RS485_DIRECTION_PIN))
#define RS485_END_SEND	(UCSR1B |= (1 << TXCIE1))

#define BOOTLOADER_START_MESSAGE_TYPE				0x01
#define BOOTLOADER_HEX_MESSAGE_TYPE					0x02
#define BOOTLOADER_ACK_MESSAGE_TYPE					0x03

#define INFO_ALIVE_MESSAGE_TYPE									0x05

#define LIGHT_SET_MESSAGE_TYPE									0x07

#define ENOCEAN_MESSAGE_TYPE									0x09

uint8_t num2hex(uint8_t num);
uint16_t hex2num(const uint8_t * ascii, uint8_t num);

extern uint8_t messageBuffer0[1 + 4 + 24];
extern uint8_t messageLength0;
extern uint8_t messageType0;
extern uint8_t messageNumber0[2];
uint8_t decodeMessage0(uint8_t c);

extern uint8_t messageBuffer1[1 + 4 + 24];
extern uint8_t messageLength1;
extern uint8_t messageType1;
extern uint8_t messageNumber1[2];
uint8_t decodeMessage1(uint8_t c);

#ifdef MASTER
extern volatile uint16_t rs485Timer;
#endif

void rs485_wait_over();

#define CHECK_FAILED    0x07
#define OK              0xff

// enocean msg structure
struct EnoceanMsg {
	uint8_t h_seq;
	uint8_t length;
	uint8_t org;
	uint8_t data[4];
	uint32_t id;
	uint8_t status;
};

/* prototypes */
uint8_t enoceanDecodeMsg(struct EnoceanMsg* msg);

void initCommunication();

#endif /* COMMUNICATION_H_ */
