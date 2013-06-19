/*
 * communication.c
 *
 *  Created on: 25.02.2013
 *      Author: falko
 */

#include "communication.h"
#include "uart/uart.h"

uint16_t hex2num(const uint8_t * ascii, uint8_t num) {
	uint8_t i;
	uint16_t val = 0;

	for (i = 0; i < num; i++) {
		uint8_t c = ascii[i];

		/* Hex-Ziffer auf ihren Wert abbilden */
		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'A' && c <= 'F')
			c -= 'A' - 10;
		else if (c >= 'a' && c <= 'f')
			c -= 'a' - 10;

		val = 16 * val + c;
	}

	return val;
}

uint8_t messageBuffer0[1 + 4 + 24];
uint8_t messageLength0;
uint8_t messageType0;
uint8_t messageNumber0[2];

uint8_t decodeMessage(uint8_t c) {
	messageBuffer0[0]++;
	messageBuffer0[messageBuffer0[0]] = c;

	switch (messageBuffer0[0]) {
	case 1:
		switch (messageBuffer0[1]) {
		case 'b':
			break;
		default:
			messageBuffer0[0] = 0;
			break;
		}
		break;
	case 2:
		switch (messageBuffer0[1]) {
		case 'b':
			switch (messageBuffer0[2]) {
			case 'a':
				messageType0 = BOOTLOADER_ACK_MESSAGE_TYPE;
				messageLength0 = 2;
				break;
			case 'h':
				messageType0 = BOOTLOADER_HEX_MESSAGE_TYPE;
				break;
			case 's':
				messageType0 = BOOTLOADER_START_MESSAGE_TYPE;
				messageLength0 = 4;
				break;
			default:
				messageBuffer0[0] = 0;
				break;
			}
			break;
		default:
			messageBuffer0[0] = 0;
			break;
		}
		break;
	case 4:
		messageNumber0[0] = hex2num(&messageBuffer0[3], 2);
		if (messageType0 == BOOTLOADER_HEX_MESSAGE_TYPE) {
			messageLength0 += messageNumber0[0];
		}
		break;
	case 6:
		messageNumber0[1] = hex2num(&messageBuffer0[5], 2);
		break;
	}
	if (messageBuffer0[0] > 1 && messageBuffer0[0] == messageLength0) {
		messageBuffer0[0] = 0;
		return messageType0;
	}
	return 0;
}

void initCommunication() {
	messageBuffer0[0] = 0;

#ifdef MASTER
	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU));
#endif

#ifdef SLAVE
	uart1_init(UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU));
#endif
	RS485_INIT;
}
