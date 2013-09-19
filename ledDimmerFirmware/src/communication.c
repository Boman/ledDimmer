/*
 * communication.c
 *
 *  Created on: 25.02.2013
 *      Author: falko
 */

#include "communication.h"
#include "uart/uart.h"

uint8_t num2hex(uint8_t num) {
	if (num < 10) {
		return '0' + num;
	} else {
		return 'A' + num - 10;
	}
}

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

uint8_t decodeMessage0(uint8_t c) {
	messageBuffer0[0]++;
	messageBuffer0[messageBuffer0[0]] = c;
	// MASTER Protocol
#ifdef MASTER
	switch (messageBuffer0[0]) {
	case 1:
		switch (messageBuffer0[1]) {
		case 'b':
		case 'l':
		case 'i':
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
				messageLength0 = 4;
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
		case 'l':
			switch (messageBuffer0[2]) {
			case 's':
				messageType0 = LIGHT_SET_MESSAGE_TYPE;
				messageLength0 = 6;
				break;
			default:
				messageBuffer0[0] = 0;
				break;
			}
			break;
		case 'i':
			switch (messageBuffer0[2]) {
			case 'a':
				messageType0 = INFO_ALIVE_MESSAGE_TYPE;
				messageLength0 = 2;
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
#ifdef SLAVE
#endif
	// SLAVE Protocol
	switch (messageBuffer0[0]) {
	case 1:
		switch (messageBuffer0[1]) {
		case 0xa5:
			break;
		default:
			messageBuffer0[0] = 0;
			break;
		}
		break;
	case 2:
		switch (messageBuffer0[1]) {
		case 0xa5:
			switch (messageBuffer0[2]) {
			case 0x5a:
				messageType0 = ENOCEAN_MESSAGE_TYPE;
				messageLength0 = 14;
				break;
			default:
				messageBuffer0[0] = 0;
				break;
			}
			break;
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
#endif
	return 0;
}

uint8_t messageBuffer1[1 + 4 + 24];
uint8_t messageLength1;
uint8_t messageType1;
uint8_t messageNumber1[2];

uint8_t decodeMessage1(uint8_t c) {
	messageBuffer1[0]++;
	messageBuffer1[messageBuffer1[0]] = c;
	// MASTER Protocol
#ifdef MASTER
	switch (messageBuffer1[0]) {
	case 1:
		switch (messageBuffer1[1]) {
		case 'b':
		case 'l':
			break;
		default:
			messageBuffer1[0] = 0;
			break;
		}
		break;
	case 2:
		switch (messageBuffer1[1]) {
		case 'b':
			switch (messageBuffer1[2]) {
			case 'a':
				messageType1 = BOOTLOADER_ACK_MESSAGE_TYPE;
				messageLength1 = 2;
				break;
			case 'h':
				messageType1 = BOOTLOADER_HEX_MESSAGE_TYPE;
				messageLength1 = 4;
				break;
			case 's':
				messageType1 = BOOTLOADER_START_MESSAGE_TYPE;
				messageLength1 = 4;
				break;
			default:
				messageBuffer1[0] = 0;
				break;
			}
			break;
		case 'l':
			switch (messageBuffer1[2]) {
			case 's':
				messageType1 = LIGHT_SET_MESSAGE_TYPE;
				messageLength1 = 6;
				break;
			default:
				messageBuffer1[0] = 0;
				break;
			}
			break;
		default:
			messageBuffer1[0] = 0;
			break;
		}
		break;
	case 4:
		messageNumber1[0] = hex2num(&messageBuffer1[3], 2);
		if (messageType1 == BOOTLOADER_HEX_MESSAGE_TYPE) {
			messageLength1 += messageNumber1[0];
		}
		break;
	case 6:
		messageNumber1[1] = hex2num(&messageBuffer1[5], 2);
		break;
	}
	if (messageBuffer1[0] > 1 && messageBuffer1[0] == messageLength1) {
		messageBuffer1[0] = 0;
		return messageType1;
	}
#endif
	// SLAVE Protocol
#ifdef SLAVE
	switch (messageBuffer1[0]) {
		case 1:
		switch (messageBuffer1[1]) {
			case 'b':
			case 'l':
			case 'i':
			break;
			default:
			messageBuffer1[0] = 0;
			break;
		}
		break;
		break;
		case 2:
		switch (messageBuffer1[1]) {
			case 'b':
			switch (messageBuffer1[2]) {
				case 'a':
				messageType1 = BOOTLOADER_ACK_MESSAGE_TYPE;
				messageLength1 = 2;
				break;
				case 'h':
				messageType1 = BOOTLOADER_HEX_MESSAGE_TYPE;
				messageLength1 = 4;
				break;
				case 's':
				messageType1 = BOOTLOADER_START_MESSAGE_TYPE;
				messageLength1 = 4;
				break;
				default:
				messageBuffer1[0] = 0;
				break;
			}
			break;
			case 'l':
			switch (messageBuffer1[2]) {
				case 's':
				messageType1 = LIGHT_SET_MESSAGE_TYPE;
				messageLength1 = 6;
				break;
				default:
				messageBuffer1[0] = 0;
				break;
			}
			break;
			case 'i':
			switch (messageBuffer1[2]) {
				case 'a':
				messageType1 = INFO_ALIVE_MESSAGE_TYPE;
				messageLength1 = 2;
				break;
				default:
				messageBuffer1[0] = 0;
				break;
			}
			break;
			default:
			messageBuffer1[0] = 0;
			break;
		}
		break;
		case 4:
		messageNumber1[0] = hex2num(&messageBuffer1[3], 2);
		if (messageType1 == BOOTLOADER_HEX_MESSAGE_TYPE) {
			messageLength1 += messageNumber1[0];
		}
		break;
		case 6:
		messageNumber1[1] = hex2num(&messageBuffer1[5], 2);
		break;
	}
	if (messageBuffer1[0] > 1 && messageBuffer1[0] == messageLength1) {
		messageBuffer1[0] = 0;
		return messageType1;
	}
#endif
	return 0;
}

volatile uint16_t rs485Timer = 0;

void rs485_wait_over() {
	uart1_allow_send();
}

uint8_t enoceanDecodeMsg(struct EnoceanMsg* msg) {
	uint8_t checksum, calc_checksum, byte;
	uint8_t synch = 0;
	uint8_t id[4];

	// get H_SEQ/LENGTH byte
	byte = messageBuffer0[3];
	// extract H_SEQ
	msg->h_seq = (byte & 0x70) >> 5;
	// extract LENGTH
	msg->length = byte & 0x1f;

	// get ORG byte
	msg->org = messageBuffer0[4];
	// get DATA bytes
	msg->data[0] = messageBuffer0[5];
	msg->data[1] = messageBuffer0[6];
	msg->data[2] = messageBuffer0[7];
	msg->data[3] = messageBuffer0[8];
	// get ID bytes
	id[0] = messageBuffer0[9];
	id[1] = messageBuffer0[10];
	id[2] = messageBuffer0[11];
	id[3] = messageBuffer0[12];
	// set ID as unsigned 32bit integer
	msg->id = 0x00;
	msg->id |= (uint32_t) id[0] << 24;
	msg->id |= (uint32_t) id[1] << 16;
	msg->id |= (uint32_t) id[2] << 8;
	msg->id |= (uint32_t) id[3];
	// get STATUS byte
	msg->status = messageBuffer0[13];
	// get CHECKSUM byte
	checksum = messageBuffer0[14];

	// calculate CHECKSUM
	calc_checksum = byte + msg->org + msg->data[0] + msg->data[1] + msg->data[2]
			+ msg->data[3] + id[0] + id[1] + id[2] + id[3] + msg->status;
	// compare CHECKSUM and return
	if (checksum == calc_checksum)
		return (OK);
	else
		return (CHECK_FAILED);
}

void initCommunication() {
	messageBuffer0[0] = 0;
	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU));
#ifdef SLAVE
	// pull up for enocean
	PORTD &= 1 << PD2;
#endif

	messageBuffer1[0] = 0;
	uart1_init(UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU));
	RS485_INIT;
}
