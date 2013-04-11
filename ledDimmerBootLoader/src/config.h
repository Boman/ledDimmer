/*
 * config.h
 *
 *  Created on: 17.12.2012
 *      Author: falko
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#ifdef MASTER
#define Device_ID 1
#elif defined SLAVE
#define  Device_ID 2
#else
#error "choose Target MASTER or SLAVE"
#endif

#define RS485_DIRECTION_DDR		DDRB
#define RS485_DIRECTION_PORT		PORTB
#define RS485_DIRECTION_PIN		PB4

#endif /* CONFIG_H_ */
