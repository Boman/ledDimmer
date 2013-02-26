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

// Timer 1 output compare A interrupt service routine, called every 1/15000 sec
ISR( TIMER1_COMPA_vect) {
	// Implementiere die ISR ohne zunaechst weitere IRQs zuzulassen
	//TIMSK1 = 0; // deactivate Interrupt
	// Erlaube alle Interrupts (ausser OCIE1A)
	//sei();

#ifdef IR_DEVICE
	(void) irmp_ISR(); // call irmp ISR
#endif

	if (timerCounter++ >= 150) {
		nextCentiSecond = 1;
		timerCounter = 0;
	}

	// IRQs global deaktivieren um die OCIE1A-IRQ wieder gefahrlos
	// aktivieren zu koennen
	//cli();
	//TIMSK1 = 1 << OCIE1A; // OCIE1A: Interrupt by timer compare
}

void timer1_init(void) {
	OCR1A = (F_CPU / F_INTERRUPTS) - 1; // compare value: 1/15000 of CPU frequency
	TCCR1B = (1 << WGM12) | (1 << CS10); // switch CTC Mode on, set prescaler to 1

	TIMSK1 = 1 << OCIE1A; // OCIE1A: Interrupt by timer compare
}

int main() {
	uint16_t c;

	void (*bootloader)(void) = 0x1F000; // Achtung Falle: Hier Word-Adresse

#ifdef IR_DEVICE
	IRMP_DATA irmp_data;
	irmp_init(); // initialize irmp
#endif

	initCommunication();

#ifdef MASTER

#endif

	timer1_init(); // initialize timer 1

	initDimmer();

	sei();

	setPWM(LED_R, 127);
	setPWM(LED_G, 0);
	setPWM(LED_B, 0);
	pwm_update();

	while (1) {
		// update Time
		if (nextCentiSecond) {
			nextCentiSecond = 0;
			centiSeconds++;
			ledStateCallback();
			if (centiSeconds % 500 == 1) {
				uart_puts("\ngithub.com/Boman/ledDimmer\n");
			}
		}

		//process UART0 Input
		c = uart_getc();
		if (!(c & UART_NO_DATA)) {
			uint8_t t = decodeMessage0((uint8_t) c);
			switch (t) {
			case BOOTLOADER_START_MESSAGE_TYPE:
				if (messageNumber0_0 == 1) {
					uart_puts("start Bootloader");
					_delay_ms(1000);
					bootloader();
				}
				break;
			case LIGHT_SET_MESSAGE_TYPE:
				setLightValue(messageNumber0_0, messageNumber1_0);
				uart_puts("Set Light");
//				if (messageBuffer0[1] >= 1 && messageBuffer0[1] <= 3) {
//					if (messageBuffer0[2] == 0) {
//						uart_puts("Off");
//					} else if (messageBuffer0[2] == 1) {
//						uart_puts("On");
//					} else if (messageBuffer0[2] == 1) {
//						uart_puts("2 On");
//					}
//				}
				break;
			}
		}

		//process UART1 Input
		c = uart1_getc();
		if (!(c & UART_NO_DATA)) {
			uint8_t t = decodeMessage1((unsigned char) c);
			switch (t) {
			}
		}

		//process IR Input
#ifdef IR_DEVICE
		if (irmp_get_data(&irmp_data)) {
			uart_puts("ir");
			uart_putc('0' + irmp_data.protocol);
			uart_putc(':');
			uart_putc('0' + (irmp_data.address / 10000) % 10);
			uart_putc('0' + (irmp_data.address / 1000) % 10);
			uart_putc('0' + (irmp_data.address / 100) % 10);
			uart_putc('0' + (irmp_data.address / 10) % 10);
			uart_putc('0' + irmp_data.address % 10);
			uart_putc(':');
			uart_putc('0' + (irmp_data.command / 10000) % 10);
			uart_putc('0' + (irmp_data.command / 1000) % 10);
			uart_putc('0' + (irmp_data.command / 100) % 10);
			uart_putc('0' + (irmp_data.command / 10) % 10);
			uart_putc('0' + irmp_data.command % 10);
			if (irmp_data.protocol == IRMP_NEC_PROTOCOL && // NEC-Protokoll
					irmp_data.address == 0x1234) // Adresse 0x1234
					{
				switch (irmp_data.command) {
				case 0x0001:
					break; // Taste 1
				}
			}
		}
#endif
	}

	return 0;
}
