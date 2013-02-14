#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include "irmp/irmp.h"
#include "uart/uart.h"
#include "ledDimmer.h"

#define UART_BAUD_RATE	115200

#define BOOTLOADER_START_MESSAGE_TYPE				0x01
#define BOOTLOADER_HEX_MESSAGE_TYPE					0x02
#define BOOTLOADER_ACK_MESSAGE_TYPE					0x03
#define SET_LIGHT_MESSAGE_TYPE								0x04

static uint16_t hex2num(const uint8_t * ascii, uint8_t num) {
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

uint8_t messageBuffer[1 + 4 + 24];
uint8_t messageLength;
uint8_t messageType;
uint8_t messageNumber;

uint8_t decodeMessage(uint8_t c) {
	messageBuffer[0]++;
	messageBuffer[messageBuffer[0]] = c;
	switch (messageBuffer[0]) {
	case 1:
		if (messageBuffer[1] != 'b') {
			messageBuffer[0] = 0;
		}
		break;
	case 2:
		if (messageBuffer[1] == 'b') {
			if (messageBuffer[2] == 's') {
				messageType = BOOTLOADER_START_MESSAGE_TYPE;
				messageLength = 4;
			} else if (messageBuffer[2] == 'h') {
				messageType = BOOTLOADER_HEX_MESSAGE_TYPE;
				messageLength = 4;
			} else if (messageBuffer[2] == 'a') {
				messageType = BOOTLOADER_ACK_MESSAGE_TYPE;
				messageLength = 2;
			}
		} else {
			messageBuffer[0] = 0;
		}
		break;
	case 4:
		messageNumber = hex2num(&messageBuffer[3], 2);
		if (messageType == BOOTLOADER_HEX_MESSAGE_TYPE) {
			messageLength += messageNumber;
		}
		break;
	}
	if (messageBuffer[0] > 1 && messageBuffer[0] == messageLength) {
		messageBuffer[0] = 0;
		return messageType;
	}
	return 0;
}

volatile uint8_t timerCounter = 0;
volatile uint32_t centiSeconds = 0;

// Timer 1 output compare A interrupt service routine, called every 1/15000 sec
ISR(TIMER1_COMPA_vect) {
	(void) irmp_ISR(); // call irmp ISR
	// call other timer interrupt routines...
	if (timerCounter++ >= 150) {
		timerCounter = 0;
		centiSeconds++;
	}
}

void timer1_init(void) {
	OCR1A = (F_CPU / F_INTERRUPTS) - 1; // compare value: 1/15000 of CPU frequency
	TCCR1B = (1 << WGM12) | (1 << CS10); // switch CTC Mode on, set prescaler to 1

	TIMSK1 = 1 << OCIE1A; // OCIE1A: Interrupt by timer compare
}

int main() {
	unsigned int c;
	void (*bootloader)(void) = 0x1F000; // Achtung Falle: Hier Word-Adresse

	messageBuffer[0] = 0;

	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU));

	irmp_init(); // initialize irmp
	timer1_init(); // initialize timer 1

	initDimmer();

	sei();

	setPWM(LED_R, 0);
	setPWM(LED_G, 0);
	setPWM(LED_B, 0);

	pwm_update();

	_delay_ms(2000);

	uint8_t b = 0;
	while (1) {
		if (centiSeconds % 30 == 0) {
			setPWM(LED_B, b++);
			pwm_update();
		}
//		for (uint8_t r = 0; r < 256; ++r) {
//			setPWM(LED_R, r);
//			_delay_ms(10);
//		}
//		for (uint8_t r = 0; r < 256; ++r) {
//			setPWM(LED_R, 255 - r);
//			_delay_ms(10);
//		}
	}

//	uart_puts("\ngithub.com/Boman/ledDimmer\n");
//
//	while (1) {
//		c = uart_getc();
//		if (!(c & UART_NO_DATA)) {
//			uint8_t t = decodeMessage((unsigned char) c);
//			switch (t) {
//			case BOOTLOADER_START_MESSAGE_TYPE:
//				if (messageNumber == 1) {
//					uart_puts("start Bootloader");
//					_delay_ms(1000);
//					bootloader();
//				}
//				break;
//			case SET_LIGHT_MESSAGE_TYPE:
//				if (messageBuffer[1] >= 1 && messageBuffer[1] <= 3) {
//					uart_puts("Set Light");
//					if (messageBuffer[2] == 0) {
//						PORTA &= ~(1 << 7);
//						uart_puts("Off");
//					} else {
//						PORTA |= 1 << 7;
//						uart_puts("On");
//					}
//				}
//				break;
//			}
//		}
//	}

	return 0;
}
