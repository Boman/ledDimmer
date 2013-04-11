#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include "config.h"
#ifdef IR_DEVICE
#include "irmp/irmp.h"
#endif
#include "uart/uart.h"
#include "ledDimmer.h"
#include "ledState.h"
#include "communication.h"

volatile uint8_t timerCounter = 0;
volatile uint32_t centiSeconds = 0;
volatile uint8_t nextCentiSecond = 0;

#ifdef IR_DEVICE
volatile uint16_t irmpLastKeyPressCounter = 0;
#endif

// Timer 1 output compare A interrupt service routine, called every 1/15000 sec
ISR( TIMER1_COMPA_vect) {
	// Implementiere die ISR ohne zunaechst weitere IRQs zuzulassen
	TIMSK1 = 0; // deactivate Interrupt
	// Erlaube alle Interrupts (ausser OCIE1A)
	sei();

#ifdef IR_DEVICE
	(void) irmp_ISR(); // call irmp ISR
#endif

	if (timerCounter++ >= INTERRUPTS_COUNT) {
		nextCentiSecond = 1;
		timerCounter = 0;
#ifdef IR_DEVICE
		if (irmpLastKeyPressCounter != 0xFFFF) {
			irmpLastKeyPressCounter++;
		}
#endif
	}

	// IRQs global deaktivieren um die OCIE1A-IRQ wieder gefahrlos
	// aktivieren zu koennen
	cli();
	TIMSK1 = 1 << OCIE1A; // OCIE1A: Interrupt by timer compare
}

void timer1_init(void) {
	OCR1A = (F_CPU / F_INTERRUPTS) - 1; // compare value: 1/15000 of CPU frequency
	TCCR1B = (1 << WGM12) | (1 << CS10); // switch CTC Mode on, set prescaler to 1

	TIMSK1 = 1 << OCIE1A; // OCIE1A: Interrupt by timer compare
}

void lightCommand(int8_t rVal, int8_t gVal, int8_t bVal) {
//	uart_puts("ls");
//	uart_putc(num2hex((LIGHT_ALL_R & 0xF0) >> 4));
//	uart_putc(num2hex(LIGHT_ALL_R & 0x0F));
//	uart_putc(num2hex((rVal & 0xF0) >> 4));
//	uart_putc(num2hex(rVal & 0x0F));
//	uart1_puts("ls");
//	RS485_SEND;
//	uart1_putc(num2hex((LIGHT_ALL_R & 0xF0) >> 4));
//	uart1_putc(num2hex(LIGHT_ALL_R & 0x0F));
//	uart1_putc(num2hex((rVal & 0xF0) >> 4));
//	uart1_putc(num2hex(rVal & 0x0F));
//	setLightValue(LIGHT_ALL_R, rVal);
//
//	uart_puts("ls");
//	uart_putc(num2hex((LIGHT_ALL_G & 0xF0) >> 4));
//	uart_putc(num2hex(LIGHT_ALL_G & 0x0F));
//	uart_putc(num2hex((gVal & 0xF0) >> 4));
//	uart_putc(num2hex(gVal & 0x0F));
//	setLightValue(LIGHT_ALL_G, gVal);
//
//	uart_puts("ls");
//	uart_putc(num2hex((LIGHT_ALL_B & 0xF0) >> 4));
//	uart_putc(num2hex(LIGHT_ALL_B & 0x0F));
//	uart_putc(num2hex((bVal & 0xF0) >> 4));
//	uart_putc(num2hex(bVal & 0x0F));
//	setLightValue(LIGHT_ALL_B, bVal);

	setLightValue(3, LIGHT_R, rVal);
	setLightValue(3, LIGHT_G, gVal);
	setLightValue(3, LIGHT_B, bVal);
}

int16_t main() {
	uint16_t c;

	void (*bootloader)(void) = 0x1F000; // Achtung Falle: Hier Word-Adresse

#ifdef IR_DEVICE
	uint8_t irmpEvent = 0;
	uint16_t irmpSameKeyPressCounter = 0;
	uint8_t irmpLastKey = 0;
	uint8_t irmpNewKey = 0;
	uint8_t lastRGBBrightness = 0;
	uint8_t tmpValue;
	IRMP_DATA irmp_data;
	irmp_init(); // initialize irmp
#endif

	initCommunication();
	RS485_RECEIVE;

	timer1_init(); // initialize timer 1

	initDimmer();

	sei();

	while (1) {
		//process UART0 Input
		c = uart_getc();
		if (!(c & UART_NO_DATA)) {
#ifdef MASTER
			uint8_t t = decodeMessage0((uint8_t) c);
			switch (t) {
			case BOOTLOADER_START_MESSAGE_TYPE:
				if (messageNumber0[0] == 1) {
					uart_puts("start Bootloader");
					_delay_ms(1000);
					bootloader();
				}
				break;
			case LIGHT_SET_MESSAGE_TYPE:
				//TODO setLightValue(messageNumber0[0], messageNumber0[1]);
				uart_puts("Set Light");
				RS485_SEND;
				uart1_putc('l');
				uart1_putc('s');
				uart1_putc(num2hex(messageNumber0[0] >> 4));
				uart1_putc(num2hex(messageNumber0[0] && 0x0F));
				uart1_putc(num2hex(messageNumber0[1] >> 4));
				uart1_putc(num2hex(messageNumber0[1] && 0x0F));
				break;
			}
#endif
#ifdef SLAVE
#endif
		}

		//process UART1 Input
		c = uart1_getc();
		if (!(c & UART_NO_DATA)) {
			uint8_t t = decodeMessage1((uint8_t) c);
#ifdef MASTER
			uart_putc('c');
			uart_putc(c);
			switch (t) {
			}
#endif
#ifdef SLAVE
			switch (t) {
				case BOOTLOADER_START_MESSAGE_TYPE:
				if (messageNumber1[0] == 2) {
					uart_puts("start Bootloader");
					_delay_ms(1000);
					bootloader();
				}
				break;
				case LIGHT_SET_MESSAGE_TYPE:
				setLightValue(messageNumber1[0], messageNumber1[1]);
				break;
			}
#endif
		}

		// process IR Input
#ifdef IR_DEVICE
		if (irmp_get_data(&irmp_data) && // data received
				irmp_data.protocol == IRMP_NEC_PROTOCOL && // NEC protocol
				irmp_data.address == IR_ADRESS) // matching adress
		{
			irmpNewKey = irmp_data.command;
		} else {
			irmpNewKey = 0;
		}

		if (irmpEvent == 1) {
			irmpEvent = 2;
		}

		if (irmpEvent == 3) {
			irmpEvent = 0;
		}

		if (irmpEvent == 0 && irmpNewKey != 0) {
			irmpEvent = 1; // a new key was pressed
			irmpLastKey = irmpNewKey;
			irmpLastKeyPressCounter = 0;
			irmpSameKeyPressCounter = 0;
		}

		if (irmpEvent == 2 && irmpNewKey == 0 && irmpLastKeyPressCounter > 20) {
			irmpEvent = 3;
		}

		if (irmpEvent == 4 && irmpNewKey == 0 && irmpLastKeyPressCounter > 20) {
			irmpEvent = 0;
		}

		if ((irmpEvent == 2 || irmpEvent == 4) && irmpNewKey != 0 && irmpLastKey == irmpNewKey) {
			irmpSameKeyPressCounter += irmpLastKeyPressCounter;
			irmpLastKeyPressCounter = 0;
			if (irmpSameKeyPressCounter > 100) {
				irmpEvent = 4;
			}
		}

		switch (irmpEvent) {
		case 1:
			switch (irmpLastKey) {
			case IR_RGB_BRIGHTER:
				tmpValue = getLightValue(Device_ID - 1, LIGHT_RGB_BRIGHTNESS);
				if (tmpValue < 255 - 16) {
					tmpValue += 16;
				}
				setLightValue(3, LIGHT_RGB_BRIGHTNESS, tmpValue);
				break;
			case IR_RGB_DARKER:
				tmpValue = getLightValue(Device_ID - 1, LIGHT_RGB_BRIGHTNESS);
				if (tmpValue >= 16) {
					tmpValue -= 16;
				}
				setLightValue(3, LIGHT_RGB_BRIGHTNESS, tmpValue);
				break;

			case IR_ON_OFF:
				tmpValue = getLightValue(Device_ID - 1, LIGHT_RGB_BRIGHTNESS);
				setLightValue(3, LIGHT_RGB_BRIGHTNESS, lastRGBBrightness);
				lastRGBBrightness = tmpValue;
				break;

				// color buttons
			case IR_COLOR_RED:
				lightCommand(255, 0, 0);
				break;
			case IR_COLOR_GREEN:
				lightCommand(0, 255, 0);
				break;
			case IR_COLOR_BLUE:
				lightCommand(0, 0, 255);
				break;
			case IR_COLOR_WHITE:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_DARK_RED:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_DARK_GREEN:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_DARK_BLUE:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_LIGHT_PINK:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_ORANGE:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_CYAN:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_MIDNIGHT_BLUE:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_PINK:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_LIGHT_YELLOW:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_JUNGLE_GREEN:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_VIOLET:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_SKY_BLUE:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_YELLOW:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_COBALT:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_MAGENTA:
				lightCommand(255, 255, 255);
				break;
			case IR_COLOR_LIGHT_BLUE:
				lightCommand(255, 255, 255);
				break;

			case IR_DIY3:
				break;
			case IR_DIY4:
				break;
			case IR_DIY5:
				break;
			case IR_DIY6:
				break;

			case IR_AUTO:
				//TODO setLightValue(LIGHT_ALL_PROGRAM, 1);
				break;
			case IR_FLASH:
				lightCommand(255, 255, 255);
				break;
			case IR_JUMP3:
				lightCommand(255, 255, 255);
				break;
			case IR_JUMP7:
				lightCommand(255, 255, 255);
				break;
			case IR_FADE3:
				lightCommand(255, 255, 255);
				break;
			case IR_FADE7:
				lightCommand(255, 255, 255);
				break;

			}
			break;
		default:
			break;
		}

#endif

// update Time
		if (nextCentiSecond) {
			nextCentiSecond = 0;
			centiSeconds++;
			ledStateCallback();
//			if (centiSeconds % 512 == 1) {
//				uart_puts("\ngithub.com/Boman/ledDimmer\n");
//			}
		}
	}

	return 0;
}
