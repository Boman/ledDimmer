#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "uart/uart.h"

#define UART_BAUD_RATE	9600

#define ACK_MESSAGE_TYPE                      0x01
#define SET_LIGHT_MESSAGE_TYPE            0x02
#define BOOT_MESSAGE_TYPE                    0x06
#define HEX_MESSAGE_TYPE                      0x07

unsigned char messageBuffer[11];

unsigned char decodeMessage(unsigned char c) {
	messageBuffer[messageBuffer[10]] = c;
	messageBuffer[10]++;
	switch (messageBuffer[0]) {
	case SET_LIGHT_MESSAGE_TYPE:
		if (messageBuffer[10] == 3) {
			messageBuffer[10] = 0;
			return SET_LIGHT_MESSAGE_TYPE;
		}
		break;
	case BOOT_MESSAGE_TYPE:
		if (messageBuffer[10] == 2) {
			messageBuffer[10] = 0;
			return BOOT_MESSAGE_TYPE;
		}
		break;
	case HEX_MESSAGE_TYPE:
		if (messageBuffer[10] > 2 && messageBuffer[10] == messageBuffer[1]) {
			messageBuffer[10] = 0;
			return HEX_MESSAGE_TYPE;
		}
		break;
	default:
		messageBuffer[10] = 0;
		break;
	}
	return 0;
}

int main() {
	unsigned int c;
	void (*bootloader)(void) = 0x1F000; // Achtung Falle: Hier Word-Adresse

	messageBuffer[10] = 0;

	DDRA |= (1 << 5) | (1 << 6) | (1 << 7);
	PORTA &= ~((1 << 5) | (1 << 6) | (1 << 7));

	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU));
	sei();

	uart_puts_P("A3");

	for (;;) {
		c = uart_getc();
		if (!(c & UART_NO_DATA)) {
			switch (decodeMessage((unsigned char) c)) {
			case BOOT_MESSAGE_TYPE:
				if (messageBuffer[1] == 0x01) {
					uart_puts("goto Bootloader");
					uart_putc(ACK_MESSAGE_TYPE);
					_delay_ms(1000);
					bootloader();
				}
				break;
			case SET_LIGHT_MESSAGE_TYPE:
				if (messageBuffer[1] >= 1 && messageBuffer[1] <= 3) {
					uart_puts("\n\rSet Light");
					if (messageBuffer[2] == 0) {
						PORTA &= ~(1 << 7);
						uart_puts("\n\rOff");
					} else {
						PORTA |= 1 << 7;
						uart_puts("\n\rOn");
					}
				}
				uart_putc(ACK_MESSAGE_TYPE);
				break;
			case HEX_MESSAGE_TYPE:
				for (int i = 2; i < messageBuffer[1]; ++i) {
					uart_putc(messageBuffer[i]);
				}
				uart_putc(ACK_MESSAGE_TYPE);
				break;
			}
		}
	}
	return 0;
}
