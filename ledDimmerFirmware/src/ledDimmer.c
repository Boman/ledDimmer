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
uint8_t pwm_setting_tmp[PWM_CHANNELS + 1]; // Einstellungen der PWM Werte, sortiert

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

// PWM Update, berechnet aus den PWM Einstellungen
// die neuen Werte für die Interruptroutine

void pwm_update(void) {

	uint8_t i, j, k;
	uint8_t m1, m2, tmp_mask; // ändern uint16_t oder uint32_t für mehr Kanäle
	uint8_t min, tmp_set; // ändern auf uint16_t für mehr als 8 Bit Auflösung

	// PWM Maske für Start berechnen
	// gleichzeitig die Bitmasken generieren und PWM Werte kopieren

	m1 = 1;
	m2 = 0;
	for (i = 1; i <= (PWM_CHANNELS); i++) {
		main_ptr_mask[i] = ~m1; // Maske zum Löschen der PWM Ausgänge
		pwm_setting_tmp[i] = pwm_setting[i - 1];
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
ISR( TIMER3_COMPA_vect) {
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
}

void initDimmer(void) {
	// PWM Port einstellen
	PWM_DDR = 0xFF; // Port als Ausgang

	// Timer 3 OCRA3, als variablen Timer nutzen
	TCCR3B = (1 << CS30); // Timer läuft mit Prescaler 1
	TIMSK3 |= (1 << OCIE3A); // Interrupt freischalten
}
