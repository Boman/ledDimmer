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

#ifdef MASTER
#define UART_BAUD_RATE	9600
#elif defined SLAVE
#define UART_BAUD_RATE	38400
#else
#error "choose Target MASTER or SLAVE"
#endif

#define RS485_INIT				(RS485_DIRECTION_DDR |= 1 << RS485_DIRECTION_PIN)
#define RS485_SEND			(RS485_DIRECTION_PORT |= 1 << RS485_DIRECTION_PIN)
#define RS485_RECEIVE		(RS485_DIRECTION_PORT &= ~(1 << RS485_DIRECTION_PIN))
#define RS485_END_SEND	(UCSR1B |= (1 << TXCIE1))

#define BOOTLOADER_START_MESSAGE_TYPE				0x01
#define BOOTLOADER_HEX_MESSAGE_TYPE					0x02
#define BOOTLOADER_ACK_MESSAGE_TYPE					0x03

uint16_t hex2num(const uint8_t * ascii, uint8_t num);

extern uint8_t messageBuffer0[1 + 4 + 24];
extern uint8_t messageLength0;
extern uint8_t messageType0;
extern uint8_t messageNumber0[2];
uint8_t decodeMessage(uint8_t c);

void initCommunication();

#endif /* COMMUNICATION_H_ */
