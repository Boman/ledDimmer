/*
 Eine 8-kanalige PWM mit intelligentem Lösungsansatz
 */

// includes
#include "ledDimmer.h"

// globale Variablen

uint16_t pwm_timing[PWM_CHANNELS + 1]; // Zeitdifferenzen der PWM Werte
uint16_t pwm_timing_tmp[PWM_CHANNELS + 1];

uint8_t pwm_mask[PWM_CHANNELS + 1]; // Bitmaske für PWM Bits, welche gelöscht werden sollen
uint8_t pwm_mask_tmp[PWM_CHANNELS + 1]; // ändern uint16_t oder uint32_t für mehr Kanäle

uint8_t pwm_setting[PWM_CHANNELS]; // Einstellungen für die einzelnen PWM-Kanäle
uint16_t pwm_setting_tmp[PWM_CHANNELS + 1]; // Einstellungen der PWM Werte, sortiert

volatile uint8_t pwm_cnt_max = 1; // Zählergrenze, Initialisierung mit 1 ist wichtig!
volatile uint8_t pwm_sync; // Update jetzt möglich

// Pointer für wechselseitigen Datenzugriff

uint16_t *isr_ptr_time = pwm_timing;
uint16_t *main_ptr_time = pwm_timing_tmp;

uint8_t *isr_ptr_mask = pwm_mask; // Bitmasken fuer PWM-Kanäle
uint8_t *main_ptr_mask = pwm_mask_tmp; // ändern uint16_t oder uint32_t für mehr Kanäle

//void setPWM(uint8_t channel, uint8_t value) {
//	pwm_setting[channel] = value;
//}

// Zeiger austauschen
// das muss in einem Unterprogramm erfolgen,
// um eine Zwischenspeicherung durch den Compiler zu verhindern

void tausche_zeiger(void) {
	uint16_t *tmp_ptr16;
	uint8_t *tmp_ptr8; // ändern uint16_t oder uint32_t für mehr Kanäle

	tmp_ptr16 = isr_ptr_time;
	isr_ptr_time = main_ptr_time;
	main_ptr_time = tmp_ptr16;
	tmp_ptr8 = isr_ptr_mask;
	isr_ptr_mask = main_ptr_mask;
	main_ptr_mask = tmp_ptr8;
}

const uint16_t pwmtable[256] PROGMEM = { 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6,
		6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 10, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16,
		17, 17, 18, 18, 19, 19, 20, 20, 21, 22, 22, 23, 23, 24, 25, 25, 26, 27, 28, 28, 29, 30, 30,
		31, 32, 33, 34, 34, 35, 36, 37, 38, 39, 40, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 52,
		53, 54, 55, 56, 57, 59, 60, 61, 62, 64, 65, 66, 68, 69, 70, 72, 73, 75, 76, 78, 79, 81, 83,
		84, 86, 88, 89, 91, 93, 95, 96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 117, 119, 121,
		123, 126, 128, 130, 133, 135, 138, 141, 143, 146, 148, 151, 154, 157, 160, 163, 166, 169,
		172, 175, 178, 181, 185, 188, 191, 195, 198, 202, 206, 209, 213, 217, 221, 225, 229, 233,
		237, 241, 246, 250, 254, 259, 263, 268, 273, 278, 283, 288, 293, 298, 303, 308, 314, 319,
		325, 331, 337, 342, 348, 355, 361, 367, 374, 380, 387, 393, 400, 407, 414, 422, 429, 436,
		444, 452, 459, 467, 476, 484, 492, 501, 509, 518, 527, 536, 545, 555, 564, 574, 584, 594,
		604, 615, 625, 636, 647, 658, 669, 681, 693, 705, 717, 729, 741, 754, 767, 780, 794, 807,
		821, 835, 849, 864, 878, 893, 909, 924, 940, 956, 972, 989, 1006, 1023 };

// PWM Update, berechnet aus den PWM Einstellungen
// die neuen Werte für die Interruptroutine

void pwm_update(void) {

	uint8_t i, j, k;
	uint8_t m1, m2, tmp_mask; // ändern uint16_t oder uint32_t für mehr Kanäle
	uint16_t min, tmp_set; // ändern auf uint16_t für mehr als 8 Bit Auflösung

	// PWM Maske für Start berechnen
	// gleichzeitig die Bitmasken generieren und PWM Werte kopieren

	m1 = 1 << PWM_CHANNEL_OFFSET;
	m2 = 0;
	for (i = 1; i <= (PWM_CHANNELS); i++) {
		main_ptr_mask[i] = ~m1; // Maske zum Löschen der PWM Ausgänge
		pwm_setting_tmp[i] = pgm_read_word (& pwmtable[pwm_setting[i - 1]]);
		if (pwm_setting_tmp[i] != 0)
			m2 |= m1; // Maske zum setzen der IOs am PWM Start
		m1 <<= 1;
	}
	main_ptr_mask[0] = m2; // PWM Start Daten

	// PWM settings sortieren; Einfügesortieren

	for (i = 1; i <= PWM_CHANNELS; i++) {
		min = PWM_STEPS - 1;
		k = i;
		for (j = i; j <= PWM_CHANNELS; j++) {
			if (pwm_setting_tmp[j] < min) {
				k = j; // Index und PWM-setting merken
				min = pwm_setting_tmp[j];
			}
		}
		if (k != i) {
			// ermitteltes Minimum mit aktueller Sortiertstelle tauschen
			tmp_set = pwm_setting_tmp[k];
			pwm_setting_tmp[k] = pwm_setting_tmp[i];
			pwm_setting_tmp[i] = tmp_set;
			tmp_mask = main_ptr_mask[k];
			main_ptr_mask[k] = main_ptr_mask[i];
			main_ptr_mask[i] = tmp_mask;
		}
	}

	// Gleiche PWM-Werte vereinigen, ebenso den PWM-Wert 0 löschen falls vorhanden

	k = PWM_CHANNELS; // PWM_CHANNELS Datensätze
	i = 1; // Startindex

	while (k > i) {
		while (((pwm_setting_tmp[i] == pwm_setting_tmp[i + 1]) || (pwm_setting_tmp[i] == 0))
				&& (k > i)) {

			// aufeinanderfolgende Werte sind gleich und können vereinigt werden
			// oder PWM Wert ist Null
			if (pwm_setting_tmp[i] != 0)
				main_ptr_mask[i + 1] &= main_ptr_mask[i]; // Masken vereinigen

			// Datensatz entfernen,
			// Nachfolger alle eine Stufe hochschieben
			for (j = i; j < k; j++) {
				pwm_setting_tmp[j] = pwm_setting_tmp[j + 1];
				main_ptr_mask[j] = main_ptr_mask[j + 1];
			}
			k--;
		}
		i++;
	}

	// letzten Datensatz extra behandeln
	// Vergleich mit dem Nachfolger nicht möglich, nur löschen
	// gilt nur im Sonderfall, wenn alle Kanäle 0 sind
	if (pwm_setting_tmp[i] == 0)
		k--;

	// Zeitdifferenzen berechnen

	if (k == 0) { // Sonderfall, wenn alle Kanäle 0 sind
		main_ptr_time[0] = (uint16_t) T_PWM * PWM_STEPS / 2;
		main_ptr_time[1] = (uint16_t) T_PWM * PWM_STEPS / 2;
		k = 1;
	} else {
		i = k;
		main_ptr_time[i] = (uint16_t) T_PWM * (PWM_STEPS - pwm_setting_tmp[i]);
		tmp_set = pwm_setting_tmp[i];
		i--;
		for (; i > 0; i--) {
			main_ptr_time[i] = (uint16_t) T_PWM * (tmp_set - pwm_setting_tmp[i]);
			tmp_set = pwm_setting_tmp[i];
		}
		main_ptr_time[0] = (uint16_t) T_PWM * tmp_set;
	}

	// auf Sync warten

	pwm_sync = 0; // Sync wird im Interrupt gesetzt
	while (pwm_sync == 0)
		;

	// Zeiger tauschen
	cli();
	tausche_zeiger();
	pwm_cnt_max = k;
	sei();
}

// Timer 3 Output COMPARE A Interrupt
ISR(TIMER3_COMPA_vect) {
	static uint8_t pwm_cnt; // ändern auf uint16_t für mehr als 8 Bit Auflösung
	uint8_t tmp; // ändern uint16_t oder uint32_t für mehr Kanäle

	OCR3A += isr_ptr_time[pwm_cnt];
	tmp = isr_ptr_mask[pwm_cnt];

	if (pwm_cnt == 0) {
		PWM_PORT = tmp; // Ports setzen zu Begin der PWM
						// zusätzliche PWM-Ports hier setzen
		pwm_cnt++;
	} else {
		PWM_PORT &= tmp; // Ports löschen
						 // zusätzliche PWM-Ports hier setzen
		if (pwm_cnt == pwm_cnt_max) {
			pwm_sync = 1; // Update jetzt möglich
			pwm_cnt = 0;
		} else
			pwm_cnt++;
	}

	//TODO
	PORTB &= ~0x80;
	PORTB |= PWM_PORT & 0x80;
}

void initDimmer(void) {
	// PWM Port einstellen
	PWM_DDR = 0xFF; // Port als Ausgang
	DDRB = 0xFF; // Port als Ausgang

	// Timer 3 OCRA3, als variablen Timer nutzen
	//TCCR3B = (1 << CS30); // Timer läuft mit Prescaler 1
	TCCR3B = (1 << CS31); // Timer läuft mit Prescaler 8
	TIMSK3 |= (1 << OCIE3A); // Interrupt freischalten
}
