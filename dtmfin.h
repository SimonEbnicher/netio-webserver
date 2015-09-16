/* Zusatz für Ansteuerung des PortC über DTMF
 Die Abfrage des MT8870 erfolgt über die Pin PD2 - PD6 
 
 */
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include "usart.h"
#include "timer.h"
#include "config.h" 

#if USE_DTMF
#ifndef _DTMFIN_H_
	#define _DTMFIN_H_

	#define DTMFPORTIN      ((PIND&0xf0)>>4)
	#define RESET() {asm("ldi r30,0"); asm("ldi r31,0"); asm("ijmp");}
	
	struct {
		unsigned char pw1;
		unsigned char pw2;
		unsigned char pw3;
		unsigned char pw4;
	} dtmfpw;
	
	void dtmf_init (void);
	void dtmf_do (void);
    void dtmf_beep (unsigned char tone);
	unsigned char DTMF2ASCII(unsigned char DTMFCode);
	void output_set(unsigned char output, unsigned char enable);
	unsigned char output_get(unsigned char output);
	void start_pulse(unsigned char output, unsigned char do_beep);
	void stop_pulse(void);
	
#endif
#endif
