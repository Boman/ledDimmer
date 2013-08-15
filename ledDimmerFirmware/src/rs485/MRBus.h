
#ifndef MRBUS_H
#define MRBUS_H


#define BUFFER_SIZE  0x14

#define UART_BAUD_RATE      57600      /* 57600 baud */
#define UART_BAUD_SELECT (F_CPU/(UART_BAUD_RATE*16l)-1)

// AVR type-specific stuff
#ifdef AT90S8515
//pin and register definitions
#define RS485PORT PORTD
#define RS485DDR DDRD
#define RS485TXEN 2 //(=PD2)
#define TX 1
#define RX 0
#define SER_CONT_REG UCR
#define SER_STAT_REG USR
#define UART_RXMASK 0x01
#define UARTPORT PIND
#define RC_ERR_MASK 0x18
#endif


//ATmega8515

#ifdef ATmega8515
#define RS485PORT PORTD
#define RS485DDR DDRD
#define RS485TXEN 2 //(=PD2)
#define TX 1
#define RX 0
#define SER_CONT_REG UCSRB
#define SER_STAT_REG UCSRA
#define UART_RXMASK 0x01
#define UARTPORT PIND
#define RC_ERR_MASK 0x18
#define OR DOR
#define UBRR UBRRL
#endif


//ATmega8
#ifdef ATmega8
#define RS485PORT PORTD
#define RS485DDR DDRD
#define RS485TXEN 2 //(=PD2)
#define TX 1
#define RX 0
#define SER_CONT_REG UCSRB
#define SER_STAT_REG UCSRA
#define UART_RXMASK 0x01
#define UARTPORT PIND
#define RC_ERR_MASK 0x18
#define OR DOR
#define UBRR UBRRL
#endif

//ATtiny2313

#ifdef ATtiny2313
#define RS485PORT PORTD
#define RS485DDR DDRD
#define RS485TXEN 2 //(=PD2)
#define TX 1
#define RX 0
#define SER_CONT_REG UCSRB
#define SER_STAT_REG UCSRA
#define UART_RXMASK 0x01
#define UARTPORT PIND
#define RC_ERR_MASK 0x18
#define OR DOR
#define UBRR UBRRL
#define TCCR0 TCCR0B
#endif

/* Packet component defines */
#define PKT_SRC   0
#define PKT_DEST  1
#define PKT_LEN   2
#define PKT_CRC_L 3
#define PKT_CRC_H 4
#define PKT_TYPE  5


// for programming CVs
#define PKT_CV 6
#define PKT_VALUE 7



/* Status Masks */
#define RX_PKT_READY 0x01
#define TX_PKT_READY 0x80
#define TX_BUF_ACTIVE 0x20


//timing defines for 16MHz
#define __10_US 0x14
#define __20_US 0x28
#define __100_US 0xC8


unsigned char dev_addr;

unsigned char rx_buffer[BUFFER_SIZE];
unsigned char rx_input_buffer[BUFFER_SIZE];
unsigned char tx_buffer[BUFFER_SIZE];
unsigned char tx_index=0;
unsigned char rx_index=0;
unsigned char state;
unsigned char mrbus_activity=0;
unsigned char loneliness=6;


//unsigned char ArbBitSend(unsigned char bit);
unsigned char xmit_packet(void);

#include "MRBus.c"

#endif
