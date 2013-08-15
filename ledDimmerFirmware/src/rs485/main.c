/*
	MRBus developed by Nathan D. Holmes,
	www.ndholmes.com, licensed under the GNU GPL v2.
	
	This program copyrighted by Michael Prader,
	South Tyrol, Italy.
	Licensed unter the Creative Commons Attribution-Noncommercial-Share Alike Version 3.0 license
	
	Dummy main.c, that includes more or less all possibilities of handling data on the RS485 MRBus.
	
*/

#define ATmega8  // select device type from MRBus.h and MRBus.c. Different devices have different pins, and also might
                // have different interrupt vector names

// Set the priority of this type of node here: 0 = highest pr.
unsigned char priority = 6;


//#define F_CPU            16000000      /* 16Mhz */

#include <avr/io.h>
#include <util/crc16.h>
#include <avr/eeprom.h>
#include <inttypes.h>
#include <avr/interrupt.h>



/********  INCLUDE HERE  *************/
#include "MRBus.h"



/* EEPROM map */
#define EE_DEVICE_ADDR    	0x00


#define EE_INPUT1_ADDR  	0x10
#define EE_INPUT2_ADDR	    0x11

#define EE_INPUT1_PKTTYPE	0x20
#define EE_INPUT2_PKTTYPE	0x21

#define EE_INPUT1_BYTE	    0x30
#define EE_INPUT2_BYTE	    0x31


unsigned char quartersecs = 0;

void init(void);
void PktHandler(void);

ISR(TIMER1_OVF_vect)
{

	quartersecs++;


	if (quartersecs > 20 ) {
		//roughly every 5 seconds
		// changed=0x01;  // could be used for periodic sending of packets
		quartersecs = 0;
	}
}



int main(void)
{
	unsigned char i,t;
	
	state = 0x00;
	

	init();
	
	i=0;
	while (i < BUFFER_SIZE) //initialize all buffers to 0x00
	{
		tx_buffer[i]=0x00;
		rx_buffer[i]=0x00;
		rx_input_buffer[i]=0x00;
		i++;
	}
	sei();
	
	if (!(eeprom_is_ready())) eeprom_busy_wait();
	dev_addr = eeprom_read_byte(EE_DEVICE_ADDR);
	
	
	//this executes only at first startup, when EEPROM is 0xFF
	//"new" devices will respond to 0x00;
	if (0xFF == dev_addr) {
		if (!(eeprom_is_ready())) eeprom_busy_wait();
		eeprom_write_byte(EE_DEVICE_ADDR, 0x00);
	}
	
	// BE SURE TO ENABLE BROWN-OUT DETECTION: EEprom might be corrupted at low voltages!


	while(1)
	{
		
		i = 0;
		
		if (state & RX_PKT_READY) PktHandler();
				
			
		/* we want to send a packet populate data buffer with some data */
		if (!(state & (TX_BUF_ACTIVE | TX_PKT_READY))) {
			tx_buffer[PKT_SRC] = dev_addr;
			tx_buffer[PKT_DEST] = 0xFF; // sample: make a broadcast
			tx_buffer[PKT_LEN] = 7;  //length is 7 (6 bytes header and CRC)
			tx_buffer[PKT_TYPE] = 'P'; //sample packet type
			
			tx_buffer[6] = 0;  //sample data
			
			state = state | TX_PKT_READY;
		}
		
		
		while (state & TX_PKT_READY)
		{
			i = xmit_packet(); // try trasmitting a packet
			if (i)
			{
			    // transmission failed: wait 10 ms
				//delay 10 milliseconds
				for(t=0; t<100; t++)
				{
					TCNT0=0x00;
					while(TCNT0 < __100_US)
					{
						if (state & RX_PKT_READY) PktHandler();
					}
				}
				loneliness--;
			} else {
			    // control over bus assumed: enable UDRE interrupt
				cli();
				state = state & (~TX_PKT_READY);
				state = state | TX_BUF_ACTIVE;
				SER_CONT_REG = _BV(RXEN) | _BV(TXEN) | _BV(UDRIE);
				sei();
			}
		}
	}
}


void PktHandler(void)
{

	uint16_t i, adresse;
	unsigned char crc_low, crc_high, byte;
	unsigned short crc;
	crc = 0x0000;

	/*************** PACKET FILTER ***************/

	/* Loopback Test - did we send it? */
	if (rx_buffer[PKT_SRC] == dev_addr)
	{
		goto	PktIgnore;
	}

	/* Destination Test - is this for us? */
	if (rx_buffer[PKT_DEST] != 0xFF && rx_buffer[PKT_DEST] != dev_addr)
	{
		goto	PktIgnore;
	}

	/* CRC16 Test - is the packet intact? */
	for(i=0; i < rx_buffer[PKT_LEN]; i++)
	{
		if ((i != PKT_CRC_H) && (i != PKT_CRC_L))
		{
			crc = _crc16_update(crc, rx_buffer[i]);
		}
	}
	
	crc_low = crc & 0xFF;
	crc_high = crc >> 8;
	
	if((crc_low != rx_buffer[PKT_CRC_L]) || (crc_high != rx_buffer[PKT_CRC_H])) goto PktError;


	/*************** END PACKET FILTER ***************/

	/*************** PACKET SUCCESS - PROCESS HERE ***************/
	if ('A' == rx_buffer[PKT_TYPE]) {
		tx_buffer[PKT_SRC] = dev_addr;
		tx_buffer[PKT_DEST] = rx_buffer[PKT_SRC];
		tx_buffer[PKT_LEN] = 6;
		tx_buffer[PKT_TYPE] = 'a';
		
		state |= TX_PKT_READY;
		goto PktExit;
	} else if ('R' == rx_buffer[PKT_TYPE]) {
		tx_buffer[PKT_SRC] = dev_addr;
		tx_buffer[PKT_DEST] = rx_buffer[PKT_SRC];
		tx_buffer[PKT_LEN] = 8;
		tx_buffer[PKT_TYPE] = 'r';
		tx_buffer[PKT_CV] = rx_buffer[PKT_CV];
		adresse = rx_buffer[PKT_CV] & 0xFF;
		
		tx_buffer[PKT_VALUE] = eeprom_read_byte((uint8_t *)adresse);
		state |= TX_PKT_READY;
		goto PktExit;
	} else if ('W' == rx_buffer[PKT_TYPE]) {
		
		adresse = rx_buffer[PKT_CV] & 0xFF;
		if (!eeprom_is_ready()) eeprom_busy_wait();
		eeprom_write_byte((uint8_t *)adresse, rx_buffer[PKT_VALUE]);
		tx_buffer[PKT_VALUE] = rx_buffer[PKT_VALUE];
		
		if (!eeprom_is_ready()) eeprom_busy_wait();
		dev_addr = eeprom_read_byte(EE_DEVICE_ADDR);
		
		tx_buffer[PKT_SRC] = dev_addr;
		tx_buffer[PKT_DEST] = rx_buffer[PKT_SRC];
		tx_buffer[PKT_LEN] = 0x08;
		tx_buffer[PKT_TYPE] = 'w';
		tx_buffer[PKT_CV] = rx_buffer[PKT_CV];
		
		state |= TX_PKT_READY;
		goto PktExit;
//	} else if ('X' == rx_buffer[PKT_TYPE]) {  // example for another type of packet
	    // put own code here
	  
	} else {  //
		for (i = 0; i<2; i++)
		{
			adresse = i+EE_INPUT1_ADDR;
			byte = eeprom_read_byte((uint8_t *)adresse);
			if (rx_buffer[PKT_SRC] == byte && byte != 0xFF)
			{
				if (rx_buffer[PKT_TYPE] != eeprom_read_byte((uint8_t *)(i+EE_INPUT1_PKTTYPE))) continue;
				
				byte = eeprom_read_byte((uint8_t *)(i+EE_INPUT1_BYTE));
				switch(i)
				{
					case 0:  // get specified byte
					 //nextAPBeast = rx_buffer[byte] ;
					  break;
					
					case 1:  // get specified byte
						//nextAPBwest = rx_buffer[byte] ;
						break;
											
					default: break;
				}
			}
		}
		
	}

PktError:

	/*************** PACKET ERROR ***************/
PktIgnore:
PktExit:
	state &= ~RX_PKT_READY;
	return;
}

void init(void)
{
// set baud rate
	UBRRH=0x00;
	UBRR=UART_BAUD_SELECT;
	
	RS485DDR = _BV(RS485TXEN) | _BV(TX); //set TXen pin as output
	RS485PORT = 0x00;	// and low
	
	// Format: 8-N-1
	SER_CONT_REG = (_BV(TXEN) | _BV(RXEN) | _BV(RXCIE));
	SER_STAT_REG = 0x00;
	
	UCSRC = 0x00 | _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);
	
	// prescaler is 8, timer counts 1/2 microseconds
	TCCR0 |= _BV(CS01);
	
	/* prescaler is 64 for 16bit timer */
	TCCR1A = 0x00;
	TCCR1B = 0x00 | _BV(CS11) | _BV(CS10);
	TIMSK = _BV(TOIE1);
}
