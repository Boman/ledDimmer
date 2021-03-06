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

#ifdef MASTER
uint8_t disablePollSlave = 0;
#endif

#ifdef SLAVE
uint16_t changedLight = 0;
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

#ifdef MASTER
	if (rs485Timer) {
		rs485Timer--;
		if (rs485Timer == 0) {
			rs485_wait_over();
		}
	}
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

void lightCommand(int8_t adressMask, int8_t lightType, int8_t value) {
#ifdef MASTER
	//send on uart 1
	uart1_puts("ls");
	uart1_putc(num2hex(adressMask));
	uart1_putc(num2hex(lightType));
	uart1_putc(num2hex((value & 0xF0) >> 4));
	uart1_putc(num2hex(value & 0x0F));
	//send on uart 0
	uart_puts("ls");
	uart_putc(num2hex(adressMask));
	uart_putc(num2hex(lightType));
	uart_putc(num2hex((value & 0xF0) >> 4));
	uart_putc(num2hex(value & 0x0F));
#endif

#ifdef SLAVE
	// remember the changed light when master asks for change
	changedLight |= 1 << lightType;
#endif

	setLightValue(adressMask, lightType, value);
}

void rgbCommand(int8_t rVal, int8_t gVal, int8_t bVal) {
	deactivateUpdatePWM = 1;
	lightCommand(3, LIGHT_RED, rVal);
	deactivateUpdatePWM = 1;
	lightCommand(3, LIGHT_GREEN, gVal);
	lightCommand(3, LIGHT_BLUE, bVal);
}

void onProgramCompletion() {

}

uint8_t min(uint8_t a, uint8_t b) {
	if (a < b) {
		return a;
	}
	return b;
}

uint8_t max(uint8_t a, uint8_t b) {
	if (a > b) {
		return a;
	}
	return b;
}

int16_t main() {
	uint16_t c;
	uint8_t i, j;

	void (*bootloader)(void) = 0x1F000; // Achtung Falle: Hier Word-Adresse

#ifdef IR_DEVICE
	uint8_t irmpEvent = 0;
	uint16_t irmpSameKeyPressCounter = 0;
	uint16_t irmpSameKeyPressProcessedCounter = 0;
	uint8_t irmpLastKey = 0;
	uint8_t irmpNewKey = 0;
	uint8_t lastRGBBrightness = 0;
	uint8_t tmpValue;
	IRMP_DATA irmp_data;
	irmp_init(); // initialize irmp
#endif

#ifdef SLAVE
	struct EnoceanMsg msg;
#endif

	initCommunication();
	RS485_RECEIVE;

	timer1_init(); // initialize timer 1

	initDimmer();
	nextStepRGBProgram(0);
	LED_OUTPUT_INIT;

	sei();

	setLightValue(1 << DEVICE_ID, LIGHT_LED_BRIGHTNESS, 255);

	while (1) {
		// process UART0 Input
		c = uart_getc();
		if (!(c & UART_NO_DATA)) {
			uint8_t t = decodeMessage0((uint8_t) c);
#ifdef MASTER
			switch (t) {
			case BOOTLOADER_START_MESSAGE_TYPE:
				if (messageNumber0[0] == DEVICE_ID) {
					uart_puts("ba");
					cli();
					// disable timer interrupt
					TIMSK0 = 0;
					TIMSK1 = 0;
					TIMSK2 = 0;
					TIMSK3 = 0;
					UCSR0B = 0;
					UCSR1B = 0;
					_delay_ms(1000);
					bootloader();
				} else {
					disablePollSlave = 1;
					uart1_puts("bs");
					uart1_putc(num2hex((messageNumber0[0] & 0xF0) >> 4));
					uart1_putc(num2hex(messageNumber0[0] & 0x0F));
				}
				break;
			case BOOTLOADER_HEX_MESSAGE_TYPE:
				uart1_puts("bh");
				// sending the byte containing the length of the message + sending the message
				for (uint8_t i = 3; i < 5 + messageNumber0[0]; ++i) {
					uart1_putc(messageBuffer0[i]);
				}
				break;
			case LIGHT_SET_MESSAGE_TYPE:
				uart1_puts("ls");
				uart1_putc(num2hex((messageNumber0[0] & 0xF0) >> 4));
				uart1_putc(num2hex(messageNumber0[0] & 0x0F));
				uart1_putc(num2hex((messageNumber0[1] & 0xF0) >> 4));
				uart1_putc(num2hex(messageNumber0[1] & 0x0F));
				setLightValue((messageNumber0[0] & 0xF0) >> 4,
						messageNumber0[0] & 0x0F, messageNumber0[1]);
				break;
			case INFO_ALIVE_MESSAGE_TYPE:
				for (i = LIGHT_RED; i < LIGHT_SPEED; ++i) {
					for (j = 0; j < 2; ++j) {
						deactivateUpdatePWM = 1;
						lightCommand(1 << j, i, getLightValue(j, i));
					}
				}
				break;
			}
#endif
#ifdef SLAVE
			switch (t) {
				case ENOCEAN_MESSAGE_TYPE:
				if (enoceanDecodeMsg(&msg) == OK) {
					if (msg.id == ENOCEAN_ID) {
						if (msg.org == 0x05) { //PTM switch msg?
							if (msg.status & 0x10) { //NU=1? (Button pressed?)
								if ((msg.data[0] & 0x20) == 0x00) { //UD=0? (I-Button)
									switch ((msg.data[0] & 0xc0) >> 6) {
										case 0:
										if (getLightValue(LED1_ADRESS,
														LIGHT_LED_BRIGHTNESS) != 255) {
											lightCommand(1 << LED1_ADRESS,
													LIGHT_LED_BRIGHTNESS, 255);
										}
										break;
										case 1:
										if (getLightValue(LED2_ADRESS,
														LIGHT_LED_BRIGHTNESS) != 255) {
											lightCommand(1 << LED2_ADRESS,
													LIGHT_LED_BRIGHTNESS, 255);
										}
										break;
										default:
										break;
									}
								} else if (msg.data[0] & 0x20) { //UD=1? (O-Button)
									switch ((msg.data[0] & 0xc0) >> 6) {
										case 0:
										if (getLightValue(LED1_ADRESS,
														LIGHT_LED_BRIGHTNESS) != 0) {
											lightCommand(1 << LED1_ADRESS,
													LIGHT_LED_BRIGHTNESS, 0);
										}
										break;
										case 1:
										if (getLightValue(LED2_ADRESS,
														LIGHT_LED_BRIGHTNESS) != 0) {
											lightCommand(1 << LED2_ADRESS,
													LIGHT_LED_BRIGHTNESS, 0);
										}
										break;
										default:
										break;
									}
								}
							} else if (((msg.status & 0x10) >> 4) == 0x00) { //NU=0? (Button released?)
								if (((msg.data[0] & 0xf0) >> 4) == 0x00) { //0 Buttons & PR=0
								}
							}
						}
					}
				} else {
				}
				break;
			}
#endif
		}

		// process UART1 Input
		c = uart1_getc();
		if (!(c & UART_NO_DATA)) {
			uint8_t t = decodeMessage1((uint8_t) c);
#ifdef MASTER
			switch (t) {
			case LIGHT_SET_MESSAGE_TYPE:
				uart_puts("ls");
				uart_putc(num2hex((messageNumber1[0] & 0xF0) >> 4));
				uart_putc(num2hex(messageNumber1[0] & 0x0F));
				uart_putc(num2hex((messageNumber1[1] & 0xF0) >> 4));
				uart_putc(num2hex(messageNumber1[1] & 0x0F));
				setLightValue((messageNumber1[0] & 0xF0) >> 4,
						messageNumber1[0] & 0x0F, messageNumber1[1]);
				break;
			case BOOTLOADER_START_MESSAGE_TYPE:
				uart_puts("bs");
				uart_putc(num2hex((messageNumber1[0] & 0xF0) >> 4));
				uart_putc(num2hex(messageNumber1[0] & 0x0F));
				break;
			case BOOTLOADER_ACK_MESSAGE_TYPE:
				uart_puts("ba");
				break;
			}
#endif
#ifdef SLAVE
			switch (t) {
				case BOOTLOADER_START_MESSAGE_TYPE:
				if (messageNumber1[0] == DEVICE_ID) {
					uart1_puts("ba");
					cli();
					// disable timer interrupt
					TIMSK0 = 0;
					TIMSK1 = 0;
					TIMSK2 = 0;
					TIMSK3 = 0;
					UCSR0B = 0;
					UCSR1B = 0;
					_delay_ms(1000);
					bootloader();
				}
				break;
				case INFO_ALIVE_MESSAGE_TYPE:
				//TODO add the code for wall switch changement
				if(changedLight != 0) {

					for (i = LIGHT_RED; i < LIGHT_SPEED; ++i) {
						if(changedLight & 1 << i) {
							for (j = 0; j < 2; ++j) {
								//send on uart 1
								uart1_puts("ls");
								uart1_putc(num2hex(1 << j));
								uart1_putc(num2hex(i));
								uart1_putc(num2hex((getLightValue(j, i) & 0xF0) >> 4));
								uart1_putc(num2hex(getLightValue(j, i) & 0x0F));
							}
						}
					}

					changedLight = 0;
				}
				break;
				case LIGHT_SET_MESSAGE_TYPE:
				setLightValue((messageNumber1[0] & 0xF0) >> 4, messageNumber1[0] & 0x0F, messageNumber1[1]);
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

		// FSM for the different key presses (short, long, etc.)
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
		if ((irmpEvent == 2 || irmpEvent == 4) && irmpNewKey != 0
				&& irmpLastKey == irmpNewKey) {
			irmpSameKeyPressCounter += irmpLastKeyPressCounter;
			irmpLastKeyPressCounter = 0;
			if (irmpEvent == 2 && irmpSameKeyPressCounter > 50) {
				irmpEvent = 4;
				irmpSameKeyPressProcessedCounter = irmpSameKeyPressCounter;
			}
		}

		// turn on the light if some key was pressed
		if (irmpEvent != 0 && irmpEvent != 2 && irmpLastKey != IR_ON_OFF
				&& lastRGBBrightness != 0) {
			deactivateUpdatePWM = 1;
			lightCommand(3, LIGHT_RGB_BRIGHTNESS, lastRGBBrightness);
			lastRGBBrightness = 0;
		}

		switch (irmpEvent) {
		// key push event
		case 1:
			switch (irmpLastKey) {
			case IR_ON_OFF:
				tmpValue = getLightValue(DEVICE_ID - 1, LIGHT_RGB_BRIGHTNESS);
				lightCommand(3, LIGHT_RGB_BRIGHTNESS, lastRGBBrightness);
				lastRGBBrightness = tmpValue;
				break;

				// color buttons
			case IR_COLOR_RED:
				rgbCommand(COLOR_RED);
				break;
			case IR_COLOR_GREEN:
				rgbCommand(COLOR_GREEN);
				break;
			case IR_COLOR_BLUE:
				rgbCommand(COLOR_BLUE);
				break;
			case IR_COLOR_WHITE:
				rgbCommand(COLOR_WHITE);
				break;
			case IR_COLOR_DARK_RED:
				rgbCommand(COLOR_DARK_RED);
				break;
			case IR_COLOR_DARK_GREEN:
				rgbCommand(COLOR_DARK_GREEN);
				break;
			case IR_COLOR_DARK_BLUE:
				rgbCommand(COLOR_DARK_BLUE);
				break;
			case IR_COLOR_LIGHT_PINK:
				rgbCommand(COLOR_LIGHT_PINK);
				break;
			case IR_COLOR_ORANGE:
				rgbCommand(COLOR_ORANGE);
				break;
			case IR_COLOR_CYAN:
				rgbCommand(COLOR_CYAN);
				break;
			case IR_COLOR_MIDNIGHT_BLUE:
				rgbCommand(COLOR_MIDNIGHT_BLUE);
				break;
			case IR_COLOR_PINK:
				rgbCommand(COLOR_PINK);
				break;
			case IR_COLOR_LIGHT_YELLOW:
				rgbCommand(COLOR_LIGHT_YELLOW);
				break;
			case IR_COLOR_JUNGLE_GREEN:
				rgbCommand(COLOR_JUNGLE_GREEN);
				break;
			case IR_COLOR_VIOLET:
				rgbCommand(COLOR_VIOLET);
				break;
			case IR_COLOR_SKY_BLUE:
				rgbCommand(COLOR_SKY_BLUE);
				break;
			case IR_COLOR_YELLOW:
				rgbCommand(COLOR_YELLOW);
				break;
			case IR_COLOR_COBALT:
				rgbCommand(COLOR_COBALT);
				break;
			case IR_COLOR_MAGENTA:
				rgbCommand(COLOR_MAGENTA);
				break;
			case IR_COLOR_LIGHT_BLUE:
				rgbCommand(COLOR_LIGHT_BLUE);
				break;

			case IR_QUICKER:
				lightCommand(3, LIGHT_SPEED,
						min(SPEED_STEPS - 1,
								getLightValue(DEVICE_ID - 1, LIGHT_SPEED)) + 1);
				break;
			case IR_SLOWER:
				lightCommand(3, LIGHT_SPEED,
						max(1, getLightValue(DEVICE_ID - 1, LIGHT_SPEED)) - 1);
				break;

			case IR_DIY3:
				break;
			case IR_DIY4:
				break;
			case IR_DIY5:
				break;
			case IR_DIY6:
				break;

//			case IR_AUTO:
//				lightCommand(3, LIGHT_PROGRAM, 2);
//				break;
//			case IR_FLASH:
//				lightCommand(3, LIGHT_PROGRAM, 3);
//				break;
//			case IR_JUMP3:
//				lightCommand(3, LIGHT_PROGRAM, 4);
//				break;
//			case IR_JUMP7:
//				lightCommand(3, LIGHT_PROGRAM, 5);
//				break;
//			case IR_FADE3:
//				lightCommand(3, LIGHT_PROGRAM, 6);
//				break;
//			case IR_FADE7:
//				lightCommand(3, LIGHT_PROGRAM, 7);
//				break;

			}
			break;

			// key up event (fired 200 ms after keypress)
		case 3:
			switch (irmpLastKey) {
			case IR_RGB_BRIGHTER:
				lightCommand(3, LIGHT_RGB_BRIGHTNESS,
						min(255 - 16, getLightValue(DEVICE_ID - 1,
						LIGHT_RGB_BRIGHTNESS)) + 16);
				break;
			case IR_RGB_DARKER:
				lightCommand(3, LIGHT_RGB_BRIGHTNESS,
						max(16, getLightValue(DEVICE_ID - 1,
						LIGHT_RGB_BRIGHTNESS)) - 16);
				break;

			case IR_RED_BRIGHTER:
				lightCommand(3, LIGHT_RED,
						min(255 - 16, getLightValue(DEVICE_ID - 1, LIGHT_RED))
								+ 16);
				break;
			case IR_RED_DARKER:
				lightCommand(3, LIGHT_RED,
						max(16, getLightValue(DEVICE_ID - 1, LIGHT_RED)) - 16);
				break;
			case IR_GREEN_BRIGHTER:
				lightCommand(3, LIGHT_GREEN,
						min(255 - 16, getLightValue(DEVICE_ID - 1, LIGHT_GREEN))
								+ 16);
				break;
			case IR_GREEN_DARKER:
				lightCommand(3, LIGHT_GREEN,
						max(16, getLightValue(DEVICE_ID - 1, LIGHT_GREEN))
								- 16);
				break;
			case IR_BLUE_BRIGHTER:
				lightCommand(3, LIGHT_BLUE,
						min(255 - 16, getLightValue(DEVICE_ID - 1, LIGHT_BLUE))
								+ 16);
				break;
			case IR_BLUE_DARKER:
				lightCommand(3, LIGHT_BLUE,
						max(16, getLightValue(DEVICE_ID - 1, LIGHT_BLUE)) - 16);
				break;

			case IR_DIY1:
				if (getLightValue(LED1_ADRESS, LIGHT_LED_BRIGHTNESS) > 127) {
					lightCommand(1 << LED1_ADRESS, LIGHT_LED_BRIGHTNESS, 0);
				} else {
					lightCommand(1 << LED1_ADRESS, LIGHT_LED_BRIGHTNESS, 255);
				}
				break;
			case IR_DIY2:
				if (getLightValue(LED2_ADRESS, LIGHT_LED_BRIGHTNESS) > 127) {
					lightCommand(1 << LED2_ADRESS, LIGHT_LED_BRIGHTNESS, 0);
				} else {
					lightCommand(1 << LED2_ADRESS, LIGHT_LED_BRIGHTNESS, 255);
				}
				break;
			}
			break;

			// key hold event
		case 4:
			tmpValue = irmpSameKeyPressCounter
					- irmpSameKeyPressProcessedCounter;
			if (tmpValue >= 2) {
				tmpValue /= 2;
				irmpSameKeyPressProcessedCounter += tmpValue * 2;

				switch (irmpLastKey) {
				case IR_RGB_BRIGHTER:
					lightCommand(3, LIGHT_RGB_BRIGHTNESS,
							min(255 - tmpValue, getLightValue(DEVICE_ID - 1,
							LIGHT_RGB_BRIGHTNESS)) + tmpValue);
					break;
				case IR_RGB_DARKER:
					lightCommand(3, LIGHT_RGB_BRIGHTNESS,
							max(tmpValue, getLightValue(DEVICE_ID - 1,
							LIGHT_RGB_BRIGHTNESS)) - tmpValue);
					break;

				case IR_RED_BRIGHTER:
					lightCommand(3, LIGHT_RED,
							min(255 - tmpValue,
									getLightValue(DEVICE_ID - 1, LIGHT_RED))
									+ tmpValue);
					break;
				case IR_RED_DARKER:
					lightCommand(3, LIGHT_RED,
							max(tmpValue,
									getLightValue(DEVICE_ID - 1, LIGHT_RED))
									- tmpValue);
					break;
				case IR_GREEN_BRIGHTER:
					lightCommand(3, LIGHT_GREEN,
							min(255 - tmpValue,
									getLightValue(DEVICE_ID - 1, LIGHT_GREEN))
									+ tmpValue);
					break;
				case IR_GREEN_DARKER:
					lightCommand(3, LIGHT_GREEN,
							max(tmpValue,
									getLightValue(DEVICE_ID - 1, LIGHT_GREEN))
									- tmpValue);
					break;
				case IR_BLUE_BRIGHTER:
					lightCommand(3, LIGHT_BLUE,
							min(255 - tmpValue,
									getLightValue(DEVICE_ID - 1, LIGHT_BLUE))
									+ tmpValue);
					break;
				case IR_BLUE_DARKER:
					lightCommand(3, LIGHT_BLUE,
							max(tmpValue,
									getLightValue(DEVICE_ID - 1, LIGHT_BLUE))
									- tmpValue);
					break;

					//TODO nicht so einfach, da der wert bei 127 hängen bleibt, Hilfe: Zustandsvariablen (hoch/runter) mit timeout
				case IR_DIY1:
//					if (getLightValue(LED1_ADRESS, LIGHT_LED_BRIGHTNESS)
//							> 127) {
//						lightCommand(1 << LED1_ADRESS, LIGHT_LED_BRIGHTNESS,
//								max(tmpValue, getLightValue(LED1_ADRESS,
//								LIGHT_LED_BRIGHTNESS)) - tmpValue);
//					} else {
//						lightCommand(1 << LED1_ADRESS, LIGHT_LED_BRIGHTNESS,
//								min(255 - tmpValue, getLightValue(LED1_ADRESS,
//								LIGHT_LED_BRIGHTNESS)) + tmpValue);
//					}
					break;
				case IR_DIY2:
//					if (getLightValue(LED2_ADRESS, LIGHT_LED_BRIGHTNESS)
//							> 127) {
//						lightCommand(1 << LED2_ADRESS, LIGHT_LED_BRIGHTNESS,
//								max(tmpValue, getLightValue(LED2_ADRESS,
//								LIGHT_LED_BRIGHTNESS)) - tmpValue);
//					} else {
//						lightCommand(1 << LED2_ADRESS, LIGHT_LED_BRIGHTNESS,
//								min(255 - tmpValue, getLightValue(LED2_ADRESS,
//								LIGHT_LED_BRIGHTNESS)) + tmpValue);
//					}
					break;
				}
			}
			break;
		}

#endif

// update Time
		if (nextCentiSecond) {
			nextCentiSecond = 0;
			centiSeconds++;
			ledStateCallback(1);
			if (programReady) {
				if (getLightValue(DEVICE_ID - 1, LIGHT_PROGRAM) == 1) {
					rgbCommand(COLOR_WHITE);
				}
				programReady = 0;
			}

#ifdef MASTER
			// poll slave
			if (!disablePollSlave) {
				uart1_puts("ia");
				rs485Timer = INTERRUPTS_COUNT / 10;
			}
#endif
		}
	}

	return 0;
}
