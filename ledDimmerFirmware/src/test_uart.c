/*************************************************************************
 Title:    example program for the Interrupt controlled UART library
 Author:   Peter Fleury <pfleury@gmx.ch>   http://jump.to/fleury
 File:     $Id: test_uart.c,v 1.5 2012/09/14 17:59:08 peter Exp $
 Software: AVR-GCC 3.4, AVRlibc 1.4
 Hardware: any AVR with built-in UART, tested on AT90S8515 at 4 Mhz

 DESCRIPTION:
 This example shows how to use the UART library uart.c

 *************************************************************************/
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "uart/uart.h"

/* 9600 baud */
#define UART_BAUD_RATE      9600

#include "irmp/irmp.h"

#ifndef F_CPU
#error F_CPU unkown
#endif

void timer1_init(void) {
#if defined (__AVR_ATtiny45__) || defined (__AVR_ATtiny85__)                // ATtiny45 / ATtiny85:
#if F_CPU >= 16000000L
	OCR1C = (F_CPU / F_INTERRUPTS / 8) - 1; // compare value: 1/15000 of CPU frequency, presc = 8
	TCCR1 = (1 << CTC1) | (1 << CS12);// switch CTC Mode on, set prescaler to 8
#else
	OCR1C = (F_CPU / F_INTERRUPTS / 4) - 1; // compare value: 1/15000 of CPU frequency, presc = 4
	TCCR1 = (1 << CTC1) | (1 << CS11) | (1 << CS10);// switch CTC Mode on, set prescaler to 4
#endif

#else                                                                       // ATmegaXX:
	OCR1A = (F_CPU / F_INTERRUPTS) - 1; // compare value: 1/15000 of CPU frequency
	TCCR1B = (1 << WGM12) | (1 << CS10); // switch CTC Mode on, set prescaler to 1
#endif

#ifdef TIMSK1
	TIMSK1 = 1 << OCIE1A; // OCIE1A: Interrupt by timer compare
#else
	TIMSK = 1 << OCIE1A; // OCIE1A: Interrupt by timer compare
#endif
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Timer 1 output compare A interrupt service routine, called every 1/15000 sec
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifdef TIM1_COMPA_vect                                                      // ATtiny84
ISR(TIM1_COMPA_vect)
#else
ISR(TIMER1_COMPA_vect)
#endif
{
	(void) irmp_ISR(); // call irmp ISR
	// call other timer interrupt routines...
}

int main(void) {
	unsigned int c;
	char buffer[7];
	int num = 134;
	IRMP_DATA irmp_data;

	DDRA |= (1 << 7);
	PORTA &= ~(1 << 7);

	irmp_init(); // initialize irmp
	timer1_init(); // initialize timer 1

	/*
	 *  Initialize UART library, pass baudrate and AVR cpu clock
	 *  with the macro
	 *  UART_BAUD_SELECT() (normal speed mode )
	 *  or
	 *  UART_BAUD_SELECT_DOUBLE_SPEED() ( double speed mode)
	 */
	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU));

	/*
	 * now enable interrupt, since UART library is interrupt controlled
	 */sei();

	/*
	 *  Transmit string to UART
	 *  The string is buffered by the uart library in a circular buffer
	 *  and one character at a time is transmitted to the UART using interrupts.
	 *  uart_puts() blocks if it can not write the whole string to the circular
	 *  buffer
	 */
	uart_puts("String stored in SRAM\n");

	/*
	 * Transmit string from program memory to UART
	 */
	uart_puts_P("String stored in FLASH\n");

	/*
	 * Use standard avr-libc functions to convert numbers into string
	 * before transmitting via UART
	 */
	itoa(num, buffer, 10); // convert interger into string (decimal format)
	uart_puts(buffer); // and transmit string to UART

	/*
	 * Transmit single character to UART
	 */
	uart_putc('\r');

	for (;;) {
		/*
		 * Get received character from ringbuffer
		 * uart_getc() returns in the lower byte the received character and
		 * in the higher byte (bitmask) the last receive error
		 * UART_NO_DATA is returned when no data is available.
		 *
		 */
		c = uart_getc();
		if (c & UART_NO_DATA) {
			/*
			 * no data available from UART
			 */
		} else {
			/*
			 * new data available from UART
			 * check for Frame or Overrun error
			 */
			if (c & UART_FRAME_ERROR) {
				/* Framing Error detected, i.e no stop bit detected */
				uart_puts_P("UART Frame Error: ");
			}
			if (c & UART_OVERRUN_ERROR) {
				/*
				 * Overrun, a character already present in the UART UDR register was
				 * not read by the interrupt handler before the next character arrived,
				 * one or more received characters have been dropped
				 */
				uart_puts_P("UART Overrun Error: ");
			}
			if (c & UART_BUFFER_OVERFLOW) {
				/*
				 * We are not reading the receive buffer fast enough,
				 * one or more received character have been dropped
				 */
				uart_puts_P("Buffer overflow error: ");
			}
			/*
			 * send received character back
			 */
			uart_putc((unsigned char) c);
			if (c == 'n') {
				PORTA |= 1 << 7;
			} else if (c == 'f') {
				PORTA &= ~(1 << 7);
			}
		}

		if (irmp_get_data(&irmp_data)) {
			if (irmp_data.protocol == 2 && irmp_data.address == -256 && irmp_data.command == 92) {
				PORTA |= 1 << 7;
				uart_puts("on\r\n");
			} else if (irmp_data.protocol == 2 && irmp_data.address == -256
					&& irmp_data.command == 93) {
				PORTA &= ~(1 << 7);
				uart_puts("off\r\n");
			}
			itoa(irmp_data.protocol, buffer, 10); // convert interger into string (decimal format)
			uart_puts(buffer); // and transmit string to UART
			uart_puts(";");
			itoa(irmp_data.address, buffer, 10); // convert interger into string (decimal format)
			uart_puts(buffer); // and transmit string to UART
			uart_puts(";");
			itoa(irmp_data.command, buffer, 10); // convert interger into string (decimal format)
			uart_puts(buffer); // and transmit string to UART
			uart_puts(";");
			uart_puts("\r\n");
			// ir signal decoded, do something here...
			// irmp_data.protocol is the protocol, see irmp.h
			// irmp_data.address is the address/manufacturer code of ir sender
			// irmp_data.command is the command code
			// irmp_protocol_names[irmp_data.protocol] is the protocol name (if enabled, see irmpconfig.h)
		}
	}
}
