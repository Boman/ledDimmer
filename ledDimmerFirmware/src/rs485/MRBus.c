/*
	This program is a port from PIC to AVR GCC of
	the project "MRBus" developed by Nathan D. Holmes;
	see http://www.ndholmes.com for more information.
	
	This program copyrighted by
	Michael Prader, South Tyrol, Italy
	Licensed under the Creative Commons Attribution-Noncommercial-Share Alike 3.0
		
*/

/* 	Version 1.0, last revision 28.12.2005, known to be working (despite of the UDRE question!)
	- 	conditional compilation (because of different interrupt vectors) currently
		supporting "ATmega8515" and "ATtiny2313".
		Please define them at the very beginning of your source file (e.g. "#define ATtiny2313"),
		or add additional settings specific to your device in the bus.h file and at the bottom of this file.
	
	Version 1.0a 05.01.2006
	-	added interrupt clearing {cli();} before manipulation of RXEN or TXEN.
	
	Version 1.0a 06.01.2006
	-	updated bus.c, bus.h, imdt.c & interface.c:
		Now only Timer0 is used for the delay periods. 16-bit Timer1 produces an overflow
		every 4th second, the variable "changed" (now declared as global variable) is set, thus forcing an
		update packet to be sent.
		
	Version 1.0b 07.01.2006
	-	Eliminated a major bug regarding transmission:
		The TX-complete interrupt now handles all necessary steps to set the device back into RX mode.
		
	Version 1.1, 23.10.2006
	-	Added support for the ATmega8 device in bus.h
	
	Version 1.1a, 07.11.2007
	-	modified arbitration bit sending routine to be conform to MRBus specifications,
		where the bus is left floating when 1 is transmitted (by disabling the
		transmitter RS485TXEN).
		
    Version 1.1b, 13.05.2008
    -   modified interrupt routines for new avr-gcc.
    
*/

//#include "bus.h"


unsigned char ArbBitSend(unsigned char bit)
{
	char slice;
	char tmp=0;

	cli();
	
	//RS485TX is already 0
	if (bit)
	{
		RS485PORT &= ~_BV(RS485TXEN);
	}
	else {
		RS485PORT |= _BV(RS485TXEN);
	}

	for (slice=0; slice<10; slice++)
	{
		if (slice > 2)
		{
			if (UARTPORT & UART_RXMASK) tmp=1;
			if (tmp ^ bit)
			{
				cli();
				RS485PORT &= ~_BV(RS485TXEN);
				SER_CONT_REG |= _BV(RXEN) | _BV(RXCIE);
				sei();
				return(1);
			}
			
		}
		// delay_us(20);
		TCNT0 = 0x00;
		while (TCNT0 < __20_US);
	}
	return(0);
}


unsigned char xmit_packet(void)
{
	unsigned char status;
	unsigned char address;
	unsigned char i,t;
	uint16_t crc16_value;
	
	address = tx_buffer[PKT_SRC];
	
	if (state & TX_BUF_ACTIVE) return(1);
	
	// First calculate CRC16
	crc16_value=0x0000;
	
	for(i=0;i<tx_buffer[PKT_LEN];i++)
	{
		if ((i != PKT_CRC_H) && (i != PKT_CRC_L))
		{
			crc16_value = _crc16_update(crc16_value, tx_buffer[i]);
		}
	}
	tx_buffer[PKT_CRC_L] = (crc16_value & 0xFF);
	tx_buffer[PKT_CRC_H] = ((crc16_value >> 8) & 0xFF);
		
	mrbus_activity=0;
	
	/* Begin of arbitration sequence */
	//delay 2 milliseconds
	for(t=0; t<20; t++)
	{
		TCNT0=0x00;
		while(TCNT0 < __100_US);
	}
	
	
	if (mrbus_activity) return(1);
	
	status = ((loneliness+priority)*10) + (address & 0x0F);
	
	RS485PORT &= ~_BV(RS485TXEN);
	RS485PORT &= ~_BV(TX);
	// Clear driver enable
	// set xmit pin low
	
	RS485DDR &= ~_BV(RX);
	// Set as input
	
	cli();
	SER_CONT_REG &= ~(_BV(RXEN) | _BV(TXEN)); // Serial Port disable
	sei();
	
	for (i=0; i<0x14; i++)
	{
		// delay_us(20);
		TCNT0 = 0x00;
		while(TCNT0 < __20_US);
		if (0 == (UARTPORT & UART_RXMASK))
		{
			cli();
			SER_CONT_REG |= _BV(RXEN) | _BV(RXCIE);
			sei();
			return(1);
		}
	}

	/* Now, wait calculated time from above */
	for (i=0; i<status; i++)
	{
		// delay_us(10);
		TCNT0 = 0x00;
		while(TCNT0 < __10_US);
		if (0 == (UARTPORT & UART_RXMASK))
		{
			cli();
			SER_CONT_REG |= _BV(RXEN) | _BV(RXCIE);
			sei();
			return(1);
		}
	}
	
	/* Arbitration Sequence - 4800 bps */
	/* Start Bit */
	
	
	if (ArbBitSend(0)) return(1);
	
	
	for (i=0; i<4; i++)
	{
		status = ArbBitSend(address & 0x01);
		address = address/2;
		
		if (status == 1) return(1);
	
	}

	if (ArbBitSend(0)) return(1);
	
	for (i=0; i<4; i++)
	{
		status = ArbBitSend(address & 0x01);
		address = address/2;

		if (status == 1) return(1);
	}

	/* Stop Bit */
	if (ArbBitSend(1)) return(1);

	
	/* Control over bus is assumed */
	RS485PORT |= (_BV(TX) | _BV(RS485TXEN));
		
	tx_index=0;
	
	return(0);
}


#ifdef ATmega8515
ISR(USART_RX_vect) //Receive Routine
{
	mrbus_activity = 1;
	if (SER_STAT_REG & RC_ERR_MASK)
	{
		// Handle framing errors - these are likely arbitration bytes
		rx_index = UDR;
		rx_index = 0; // Reset receive buffer
	}
	else
	{
		rx_input_buffer[rx_index++] = UDR;
		if ((rx_index > 5) && ((rx_index >= rx_input_buffer[PKT_LEN]) || (rx_index >= BUFFER_SIZE) ))
		{
			unsigned char i;
			rx_index = 0;
			if(!(state & RX_PKT_READY))
			{
				if (rx_input_buffer[PKT_LEN] > BUFFER_SIZE) rx_input_buffer[PKT_LEN] = BUFFER_SIZE;
				
				for(i=0; i < rx_input_buffer[PKT_LEN]; i++)
				{
					rx_buffer[i] = rx_input_buffer[i];
				}
				state = state | RX_PKT_READY;
				mrbus_activity = 2;
			}
		}
		if (rx_index >= BUFFER_SIZE)
		{
			rx_index = BUFFER_SIZE-1;
		}
	}
}

ISR(USART_UDRE_vect) //Transmit Routine
{
	if (tx_index == 1) SER_CONT_REG |= _BV(TXCIE);
	
	if ((tx_index >= BUFFER_SIZE) || (tx_index >= tx_buffer[PKT_LEN]))
	{	// Transmit is complete: terminate
		SER_CONT_REG &= ~_BV(UDRIE);
		loneliness = 6;
	}
	else
	{
		UDR = tx_buffer[tx_index++];
	}
}


ISR(USART_TX_vect)
{
	// Transmit is complete: terminate
	RS485PORT &= ~_BV(RS485TXEN);
	SER_CONT_REG |= _BV(RXEN) | _BV(RXCIE);
	SER_CONT_REG &= ~_BV(TXCIE);
	state = state & (~TX_BUF_ACTIVE);
}
#endif

/************** Routines for ATmega8  ******************************/

#ifdef ATmega8
ISR(USART_RXC_vect) //Receive Routine
{
	mrbus_activity = 1;
	if (SER_STAT_REG & RC_ERR_MASK)
	{
		// Handle framing errors - these are likely arbitration bytes
		rx_index = UDR;
		rx_index = 0; // Reset receive buffer
	}
	else
	{
		rx_input_buffer[rx_index++] = UDR;
		if ((rx_index > 5) && ((rx_index >= rx_input_buffer[PKT_LEN]) || (rx_index >= BUFFER_SIZE) ))
		{
			unsigned char i;
			rx_index = 0;
			if(!(state & RX_PKT_READY))
			{
				if (rx_input_buffer[PKT_LEN] > BUFFER_SIZE) rx_input_buffer[PKT_LEN] = BUFFER_SIZE;
				
				for(i=0; i < rx_input_buffer[PKT_LEN]; i++)
				{
					rx_buffer[i] = rx_input_buffer[i];
				}
				state = state | RX_PKT_READY;
				mrbus_activity = 2;
			}
		}
		if (rx_index >= BUFFER_SIZE)
		{
			rx_index = BUFFER_SIZE-1;
		}
	}
}

ISR(USART_UDRE_vect) //Transmit Routine
{
	if (tx_index == 1) SER_CONT_REG |= _BV(TXCIE);
	
	if ((tx_index >= BUFFER_SIZE) || (tx_index >= tx_buffer[PKT_LEN]))
	{	// Transmit is complete: terminate
		SER_CONT_REG &= ~_BV(UDRIE);
		loneliness = 6;
	}
	else
	{
		UDR = tx_buffer[tx_index++];
	}
}


ISR(USART_TXC_vect)
{
	// Transmit is complete: terminate
	RS485PORT &= ~_BV(RS485TXEN);
	SER_CONT_REG |= _BV(RXEN) | _BV(RXCIE);
	SER_CONT_REG &= ~_BV(TXCIE);
	state = state & (~TX_BUF_ACTIVE);
}
#endif

/************** Routines for ATtiny2313 ****************************/




#ifdef ATtiny2313

ISR(SIG_USART0_TX)
{
	// Transmit is complete: terminate
	RS485PORT &= ~_BV(RS485TXEN);
	SER_CONT_REG |= _BV(RXEN) | _BV(RXCIE);
	SER_CONT_REG &= ~_BV(TXCIE);
	state &= ~TX_BUF_ACTIVE;
	
}

ISR(SIG_USART0_UDRE) //Transmit Routine
{
	if (tx_index == 1) SER_CONT_REG |= _BV(TXCIE);
	
	if ((tx_index >= BUFFER_SIZE) || (tx_index >= tx_buffer[PKT_LEN]))
	{
		SER_CONT_REG &= ~_BV(UDRIE);
		loneliness = 6;
	}
	else
	{
		UDR = tx_buffer[tx_index++];
	}
}

ISR(SIG_USART0_RX) //Receive Routine
{
	mrbus_activity = 1;
	
	if (SER_STAT_REG & RC_ERR_MASK)
	{
		// Handle framing errors - these are likely arbitration bytes
		rx_index = UDR;
		rx_index = 0; // Reset receive buffer
	}
	else
	{
		rx_input_buffer[rx_index++] = UDR;
		if ((rx_index > 5) && ((rx_index >= rx_input_buffer[PKT_LEN]) || (rx_index >= BUFFER_SIZE) ))
		{
			unsigned char i;
			rx_index = 0;
			if(!(state & RX_PKT_READY))
			{
				if (rx_input_buffer[PKT_LEN] > BUFFER_SIZE) rx_input_buffer[PKT_LEN] = BUFFER_SIZE;
				
				for(i=0; i < rx_input_buffer[PKT_LEN]; i++)
				{
					rx_buffer[i] = rx_input_buffer[i];
				}
				state = state | RX_PKT_READY;
				mrbus_activity = 2;
			}
		}
		if (rx_index >= BUFFER_SIZE)
		{
			rx_index = BUFFER_SIZE-1;
		}
	}
	

}
#endif
