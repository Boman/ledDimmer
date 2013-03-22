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
	TIMSK1 = 0; // deactivate Interrupt
	// Erlaube alle Interrupts (ausser OCIE1A)
	sei();

#ifdef IR_DEVICE
	(void) irmp_ISR(); // call irmp ISR
#endif

	if (timerCounter++ >= INTERRUPTS_COUNT) {
		nextCentiSecond = 1;
		timerCounter = 0;
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

int main() {
	uint16_t c;

	void (*bootloader)(void) = 0x1F000; // Achtung Falle: Hier Word-Adresse

#ifdef IR_DEVICE
	uint8_t on = 1;
	uint8_t irmpSameKeyCounter = 0;
	IRMP_DATA irmp_data;
	irmp_init(); // initialize irmp
#endif

	initCommunication();

	timer1_init(); // initialize timer 1

	initDimmer();

	sei();

	setPWM(LED_R, 127);
	setPWM(LED_G, 0);
	setPWM(LED_B, 0);
	pwm_update();

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
				setLightValue(messageNumber0[0], messageNumber0[1]);
				uart_puts("Set Light");
				RS485_SEND;
				uart1_putc('s');
				uart1_putc('l');
				uart1_putc(num2hex(messageNumber0[0] >> 4));
				uart1_putc(num2hex(messageNumber0[0] && 0x0F));
				uart1_putc(num2hex(messageNumber0[1] >> 4));
				uart1_putc(num2hex(messageNumber0[1] && 0x0F));
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
#endif
#ifdef SLAVE
#endif
		}

		//process UART1 Input
		c = uart1_getc();
		if (!(c & UART_NO_DATA)) {
#ifdef SLAVE
			if(c!='a'&&c!='b') {
				uart1_putc(c);
			}
#endif
			uint8_t t = decodeMessage1((uint8_t) c);
#ifdef MASTER
			uart_putc('c');
			uart_putc(c);
			switch (t) {
			}
#endif
#ifdef SLAVE
			switch (t) {
				case LIGHT_SET_MESSAGE_TYPE:
				setLightValue(messageNumber0[0], messageNumber0[1]);
				uart1_puts("Set Light");
				break;
			}
#endif
		}

		//process IR Input
#ifdef IR_DEVICE
		if (irmp_get_data(&irmp_data)) {
			if (irmp_data.protocol == IRMP_NEC_PROTOCOL && // NEC-Protokoll
					irmp_data.address == IR_ADRESS) // Adresse 0x1234
			{
				if ((irmp_data.flags & IRMP_FLAG_REPETITION) && irmpSameKeyCounter > 0) {
					--irmpSameKeyCounter;
				} else {
					switch (irmp_data.command) {
					case IR_ON_OFF:
						uart_puts("on/off");
						if (on) {
							setLightValue(1, 0);
							on = 0;
						} else {
							setLightValue(1, 255);
							on = 1;
						}
						break;
					}
				}
			}
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
