/*
 * ledDimmer.h
 *
 *  Created on: 13.02.2013
 *      Author: falko
 */

#ifndef LEDDIMMER_H_
#define LEDDIMMER_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <string.h>

#define LED_PORT		PORTA
#define LED_G			5
#define LED_R				6
#define LED_B				7

// Defines an den Controller und die Anwendung anpassen
#define F_PWM         600L               // PWM-Frequenz in Hz
#define PWM_PRESCALER 1                  // Vorteiler für den Timer
#define PWM_STEPS     256                // PWM-Schritte pro Zyklus(1..256)
#define PWM_PORT      LED_PORT              // Port für PWM
#define PWM_DDR       LED_PORT               // Datenrichtungsregister für PWM
#define PWM_CHANNELS  8                  // Anzahl der PWM-Kanäle
// ab hier nichts ändern, wird alles berechnet

#define T_PWM (F_CPU/(PWM_PRESCALER*F_PWM*PWM_STEPS)) // Systemtakte pro PWM-Takt
//#define T_PWM 1   //TEST

#if ((T_PWM*PWM_PRESCALER)<(111+5))
#error T_PWM zu klein, F_CPU muss vergrössert werden oder F_PWM oder PWM_STEPS verkleinert werden
#endif

#if ((T_PWM*PWM_STEPS)>65535)
#error Periodendauer der PWM zu gross! F_PWM oder PWM_PRESCALER erhöhen.
#endif

extern uint8_t pwm_setting[];
#define setPWM(channel, value) pwm_setting[(channel)] = (value)
//void setPWM(uint8_t channel, uint8_t value);
void tausche_zeiger(void);
void pwm_update(void);
void initDimmer(void);

#endif /* LEDDIMMER_H_ */
