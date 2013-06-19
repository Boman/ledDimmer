#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <util/delay.h>
#include "uart/uart.h"
#include "communication.h"

void program_page(uint32_t page, uint8_t *buf);
void parse_hex_input(uint8_t c);
int main();

#define BOOT_UART_BAUD_RATE     9600     /* Baudrate */
#define START_SIGN              ':'      /* Hex-Datei Zeilenstartzeichen */

#define BOOTLOADER_START_MESSAGE_TYPE				0x01
#define BOOTLOADER_HEX_MESSAGE_TYPE					0x02
#define BOOTLOADER_ACK_MESSAGE_TYPE					0x03

/* Zustände des Bootloader-Programms */
#define BOOT_STATE_EXIT	        0
#define BOOT_STATE_PARSER       1

/* Zustände des Hex-File-Parsers */
#define PARSER_STATE_START      0
#define PARSER_STATE_SIZE       1
#define PARSER_STATE_ADDRESS    2
#define PARSER_STATE_TYPE       3
#define PARSER_STATE_DATA       4
#define PARSER_STATE_CHECKSUM   5
#define PARSER_STATE_ERROR      6

void program_page(uint32_t page, uint8_t *buf) {
	uint16_t i;
	uint8_t sreg;

	/* Disable interrupts */
	sreg = SREG;
	cli();

	eeprom_busy_wait ();

	boot_page_erase(page);
	boot_spm_busy_wait (); /* Wait until the memory is erased. */

	for (i = 0; i < SPM_PAGESIZE; i += 2) {
		/* Set up little-endian word. */
		uint16_t w = *buf++;
		w += (*buf++) << 8;

		boot_page_fill(page + i, w);
	}

	boot_page_write(page);
	/* Store buffer in flash page.		*/boot_spm_busy_wait(); /* Wait until the memory is written.*/

	/* Reenable RWW-section again. We need this if we want to jump back */
	/* to the application after bootloading. */boot_rww_enable ();

	/* Re-enable interrupts (if they were ever enabled). */
	SREG = sreg;
}

// boolean-defintion for software-flags
typedef enum {
	FALSE, TRUE
} bool_t;

/* Intel-HEX Zieladresse */
uint16_t hex_addr = 0,
/* Zu schreibende Flash-Page */
flash_page = 0,
/* Intel-HEX Checksumme zum Überprüfen des Daten */
hex_check = 0,
/* Positions zum Schreiben in der Datenpuffer */
flash_cnt = 0;
/* temporäre Variable */
uint8_t temp,
/* Flag zum steuern des Programmiermodus */
boot_state = BOOT_STATE_PARSER,
/* Empfangszustandssteuerung */
parser_state = PARSER_STATE_START,
/* Flag zum ermitteln einer neuen Flash-Page */
flash_page_flag = 1,
/* Datenpuffer für die Hexdaten*/
flash_data[SPM_PAGESIZE],
/* Position zum Schreiben in den HEX-Puffer */
hex_cnt = 0,
/* Puffer für die Umwandlung der ASCII in Binärdaten */
hex_buffer[5],
/* Intel-HEX Datenlänge */
hex_size = 0,
/* Zähler für die empfangenen HEX-Daten einer Zeile */
hex_data_cnt = 0,
/* Intel-HEX Recordtype */
hex_type = 0,
/* empfangene HEX-Checksumme */
hex_checksum = 0;

/**
 * Parser for the received HEX-file content
 */
void parse_hex_input(uint8_t c) {
	/* Programmzustand: Parser */
	if (boot_state == BOOT_STATE_PARSER) {
		switch (parser_state) {
		/* Warte auf Zeilen-Startzeichen */
		case PARSER_STATE_START:
			if (c == START_SIGN) {
				parser_state = PARSER_STATE_SIZE;
				hex_cnt = 0;
				hex_check = 0;
			}
			break;
			/* Parse Datengröße */
		case PARSER_STATE_SIZE:
			hex_buffer[hex_cnt++] = c;
			if (hex_cnt == 2) {
				parser_state = PARSER_STATE_ADDRESS;
				hex_cnt = 0;
				hex_size = (uint8_t) hex2num(hex_buffer, 2);
				hex_check += hex_size;
			}
			break;
			/* Parse Zieladresse */
		case PARSER_STATE_ADDRESS:
			hex_buffer[hex_cnt++] = c;
			if (hex_cnt == 4) {
				parser_state = PARSER_STATE_TYPE;
				hex_cnt = 0;
				hex_addr = hex2num(hex_buffer, 4);
				hex_check += (uint8_t) hex_addr;
				hex_check += (uint8_t) (hex_addr >> 8);
				if (flash_page_flag) {
					flash_page = hex_addr - hex_addr % SPM_PAGESIZE;
					flash_page_flag = 0;
				}
			}
			break;
			/* Parse Zeilentyp */
		case PARSER_STATE_TYPE:
			hex_buffer[hex_cnt++] = c;
			if (hex_cnt == 2) {
				hex_cnt = 0;
				hex_data_cnt = 0;
				hex_type = (uint8_t) hex2num(hex_buffer, 2);
				hex_check += hex_type;
				switch (hex_type) {
				case 0:
					parser_state = PARSER_STATE_DATA;
					break;
				case 1:
					parser_state = PARSER_STATE_CHECKSUM;
					break;
				default:
					parser_state = PARSER_STATE_DATA;
					break;
				}
			}
			break;
			/* Parse Flash-Daten */
		case PARSER_STATE_DATA:
			hex_buffer[hex_cnt++] = c;
			if (hex_cnt == 2) {
				//uart_putc('.');
				hex_cnt = 0;
				flash_data[flash_cnt] = (uint8_t) hex2num(hex_buffer, 2);
				hex_check += flash_data[flash_cnt];
				flash_cnt++;
				hex_data_cnt++;
				if (hex_data_cnt == hex_size) {
					parser_state = PARSER_STATE_CHECKSUM;
					hex_data_cnt = 0;
					hex_cnt = 0;
				}
				/* Puffer voll -> schreibe Page */
				if (flash_cnt == SPM_PAGESIZE) {
					//uart_puts("P");
					_delay_ms(100);
					program_page((uint16_t) flash_page, flash_data);
					memset(flash_data, 0xFF, sizeof(flash_data));
					flash_cnt = 0;
					flash_page_flag = 1;
				}
			}
			break;
			/* Parse Checksumme */
		case PARSER_STATE_CHECKSUM:
			hex_buffer[hex_cnt++] = c;
			if (hex_cnt == 2) {
				hex_checksum = (uint8_t) hex2num(hex_buffer, 2);
				hex_check += hex_checksum;
				hex_check &= 0x00FF;
				/* Dateiende -> schreibe Restdaten */
				if (hex_type == 1) {
					//uart_puts("P");
					_delay_ms(100);
					program_page((uint16_t) flash_page, flash_data);
					boot_state = BOOT_STATE_EXIT;
				}
				/* Überprüfe Checksumme -> muss '0' sein */
				if (hex_check == 0)
					parser_state = PARSER_STATE_START;
				else
					parser_state = PARSER_STATE_ERROR;
			}
			break;
			/* Parserfehler (falsche Checksumme) */
		case PARSER_STATE_ERROR:
			//uart_putc('#');
			LED_ON(LED_RED);
			break;
		default:
			break;
		}
	}
}

int main() {
	/* Empfangenes Zeichen + Statuscode */
	uint16_t c = 0;

	/* Funktionspointer auf 0x0000 */
	void (*start)(void) = 0x0000;

	LEDS_INIT;
	LED_ON(LED_GREEN);

	initCommunication();
	RS485_RECEIVE;

	/* Füllen der Puffer mit definierten Werten */
	memset(hex_buffer, 0x00, sizeof(hex_buffer));
	memset(flash_data, 0xFF, sizeof(flash_data));

	/* Interrupt Vektoren verbiegen */
	temp = MCUCR;
	MCUCR = temp | (1 << IVCE);
	MCUCR = temp | (1 << IVSEL);

	/* Einstellen der Baudrate und aktivieren der Interrupts */
	uart_init(UART_BAUD_SELECT(BOOT_UART_BAUD_RATE,F_CPU));
	sei();

#ifdef MASTER
	uart_puts("bs01");
#endif
#ifdef SLAVE
	uart1_puts("bs02");
#endif

	do {
#ifdef MASTER
		c = uart_getc();
#endif
#ifdef SLAVE
		c = uart1_getc();
#endif
		if (!(c & UART_NO_DATA)) {
			uart_puts("bscc");
			switch (decodeMessage((uint8_t) c)) {
			case BOOTLOADER_HEX_MESSAGE_TYPE:
				for (uint8_t i = 5; i < 5 + messageNumber0[0]; ++i) {
					LED_ON(LED_BLUE);
					parse_hex_input(messageBuffer0[i]);
//					uart_puts("bs");
//					uart_putc('0' + boot_state);
//					uart_putc('0' + parser_state);
					LED_OFF(LED_BLUE);
				}
#ifdef MASTER
				uart_puts("ba");
#endif
#ifdef SLAVE
				uart1_puts("ba");
#endif
				break;
			case BOOTLOADER_START_MESSAGE_TYPE:
				if (messageNumber0[0] == 0) {
					boot_state = BOOT_STATE_EXIT;
				} else if (messageNumber0[0] == Device_ID) {
#ifdef MASTER
					uart_puts("bs01");
#endif
#ifdef SLAVE
					uart1_puts("bs02");
#endif
				}
				break;
			}
		}
	} while (boot_state != BOOT_STATE_EXIT);

	_delay_ms(1000);

	/* Interrupt Vektoren wieder gerade biegen */
	temp = MCUCR;
	MCUCR = temp | (1 << IVCE);
	MCUCR = temp & ~(1 << IVSEL);

	/* Reset */
	start();

	return 0;
}
