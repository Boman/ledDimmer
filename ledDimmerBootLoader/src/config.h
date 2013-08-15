/*
 * config.h
 *
 *  Created on: 17.12.2012
 *      Author: falko
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#ifdef MASTER
#define DEVICE_ID 1
#define IR_DEVICE
#elif defined SLAVE
#define  DEVICE_ID 2
#else
#error "choose Target MASTER or SLAVE"
#endif

#define RS485_DIRECTION_DDR         DDRD
#define RS485_DIRECTION_PORT        PORTD
#define RS485_DIRECTION_PIN         PD4

#define LEDS_PORT      		DDRA              // Port für PWM
#define LEDS_DDR        		PORTA               // Datenrichtungsregister für PWM
#define LED_GREEN			5
#define LED_RED				6
#define LED_BLUE				7
#define LEDS_INIT				LEDS_DDR |= ((1 << LED_RED) | (1 << LED_GREEN) | (1 << LED_BLUE));

static inline void LED_ON(uint8_t led) __attribute__((always_inline));
static inline void LED_ON(uint8_t led) {
	LEDS_PORT |= (1 << led);
}
;

static inline void LED_OFF(uint8_t led) __attribute__((always_inline));
static inline void LED_OFF(uint8_t led) {
	LEDS_PORT &= ~(1 << led);
}
;

#endif /* CONFIG_H_ */
