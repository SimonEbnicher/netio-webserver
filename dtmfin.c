/* Zusatz für Ansteuerung des PortC über DTMF
 Die Abfrage des MT8870 erfolgt über die Pin PD2 - PD6 
 
 */

#include "dtmfin.h"
#include <util/delay.h>

//PB0 = DTMF PTT
//PB3 = DTMF BEEP

unsigned char DTMFReceiveBuffer[10]; // Buffer für "*" + 4 Zeichen PWD + # + 2 Zeichen Port Name + 1 Zeichen CMD + \0
unsigned char DTMFReceiveCounter=0;  // Anzahl der Empfangenen Zeichen
unsigned char DTMFSymbol=0;
unsigned int  beep_counter = 0;
unsigned long time_first_dtmf = 0;
unsigned long time_now = 0;

uint16_t pulse_timer = 0;
unsigned char pulse_channel = 100;
unsigned char pulse_running = 0;

struct {
  unsigned char new:1;
  unsigned char first_received:1;
  unsigned char beep:1;
  unsigned char tone:1;
} dtmf;

//----------------------------------------------------------------------------
//
void dtmf_init (void) {
  // init variables
  dtmf.new = 0;
  dtmf.first_received = 0;
  dtmf.beep = 0;
  dtmf.tone = 0;
  
  //init password
  eeprom_busy_wait();
  dtmfpw.pw1 = eeprom_read_byte(DTMF_EEPROM_PASSWORD);
  eeprom_busy_wait();
  dtmfpw.pw2 = eeprom_read_byte(DTMF_EEPROM_PASSWORD+1);
  eeprom_busy_wait();
  dtmfpw.pw3 = eeprom_read_byte(DTMF_EEPROM_PASSWORD+2);
  eeprom_busy_wait();
  dtmfpw.pw4 = eeprom_read_byte(DTMF_EEPROM_PASSWORD+3);
  //consider password all zero or all 0xFF to be invalid and use default instead
  if(dtmfpw.pw1==0 && dtmfpw.pw2==0 && dtmfpw.pw3==0 && dtmfpw.pw4==0) {
	  dtmfpw.pw1 = DTMF_DEFAULTPASSWORD_1;
	  dtmfpw.pw2 = DTMF_DEFAULTPASSWORD_2;
	  dtmfpw.pw3 = DTMF_DEFAULTPASSWORD_3;
	  dtmfpw.pw4 = DTMF_DEFAULTPASSWORD_4;
  }
  if(dtmfpw.pw1==255 && dtmfpw.pw2==255 && dtmfpw.pw3==255 && dtmfpw.pw4==255) {
	  dtmfpw.pw1 = DTMF_DEFAULTPASSWORD_1;
	  dtmfpw.pw2 = DTMF_DEFAULTPASSWORD_2;
	  dtmfpw.pw3 = DTMF_DEFAULTPASSWORD_3;
	  dtmfpw.pw4 = DTMF_DEFAULTPASSWORD_4;
  }
  
  for(;DTMFReceiveCounter<10;DTMFReceiveCounter++) { DTMFReceiveBuffer[DTMFReceiveCounter]=0; }
  DTMFReceiveCounter = 0;
  
  // init outputs
  DDRB |= (1<<PB0)|(1<<PB3);
  PORTB &= ~(1<<PB0); // clear PTT out
  PORTB &= ~(1<<PB3); // clear Beep out
  
  // init interrupts
  // INT1
  MCUCR |= (1<<ISC11) | (1<<ISC10); //set INT1 to interrupt on rising edge
  GICR |= (1<<INT1); // enable INT1 interrupt
  // TMR2
  TCNT2 = 0;
  OCR2 = 125;   // 16M/2k(freq)=8k --- 8k/64(prescaler)=125(OCR count)
  TCCR2 |= (1<<WGM21); // set clear timer on compare match mode
  TIMSK |= (1<<OCIE2); // enable output compare match interrupt
  TCCR2 |= (1<<CS22); // start timer (set prescaler from "no clock" to CLKio/64)
  //TCCR2 &= ~(1<<CS22); // stop timer by setting prescaler to "no clock"
}


//----------------------------------------------------------------------------
// Process logic (call once every program cycle)
void dtmf_do (void) {
  unsigned char err = 0;
  unsigned char channel_num = 0;

  // new symbol arrived ?
  if(dtmf.new) {
    //Reset counter, first symbol arrived
    if(DTMFSymbol == '*') {
      DTMFReceiveCounter = 0;
      dtmf.first_received = 1;
	  time_first_dtmf = timer_get();
      DTMFReceiveBuffer[DTMFReceiveCounter++] = DTMFSymbol;
    }
    //Otherwise fill buffer
    else {
      DTMFReceiveBuffer[DTMFReceiveCounter++] = DTMFSymbol;
    }
    dtmf.new = 0;
  }
  
  //timeout DTMF_TIMEOUT seconds
  time_now = timer_get();
  if( time_now >= (time_first_dtmf + DTMF_TIMEOUT) ) {
	 DTMFReceiveCounter = 0;
	 dtmf.first_received = 0; 
  }
    
  
  // full sequence received
  if(DTMFReceiveCounter == 9) {
    
    // parse the sequence & check passwd
    if(DTMFReceiveBuffer[0] != '*') { err = 1; }
    if(DTMFReceiveBuffer[1] != dtmfpw.pw1) { err = 1; }
    if(DTMFReceiveBuffer[2] != dtmfpw.pw2) { err = 1; }
    if(DTMFReceiveBuffer[3] != dtmfpw.pw3) { err = 1; }
    if(DTMFReceiveBuffer[4] != dtmfpw.pw4) { err = 1; }
    if(DTMFReceiveBuffer[5] != '#') { err = 1; }
    switch(DTMFReceiveBuffer[6]) {
      case '0':
        //channel_num += 0;
        break;
      case '1':
        channel_num += 10;
        break;
      case '2':
		channel_num += 20;
		break;
      case '3':
		channel_num += 30;
		break;
      case '4':
		channel_num += 40;
		break;
      case '5':
		channel_num += 50;
		break;
	  case '6':
		channel_num += 60;
		break;
      case '7':
		channel_num += 70;
		break;
      case '8':
		channel_num += 80;
		break;
      case '9':
		channel_num += 90;
		break;
      default:
        err = 1;
        break;
    }
    switch(DTMFReceiveBuffer[7]) {
      case '0':
        //channel_num += 0;
        break;
      case '1':
        channel_num += 1;
        break;
      case '2':
        channel_num += 2;
        break;
      case '3':
        channel_num += 3;
        break;
      case '4':
        channel_num += 4;
        break;
      case '5':
        channel_num += 5;
        break;
      case '6':
        channel_num += 6;
        break;
      case '7':
        channel_num += 7;
        break;
      case '8':
        channel_num += 8;
        break;
      case '9':
        channel_num += 9;
        break;
      default:
        err = 1;
        break;
    }
    if(DTMFReceiveBuffer[8] != '0' && DTMFReceiveBuffer[8] != '1' && DTMFReceiveBuffer[8] != 'A' && DTMFReceiveBuffer[8] != 'D' && DTMFReceiveBuffer[8] != '6') { err = 1; }
    if(err) {
      DTMFReceiveCounter = 0;
      dtmf.first_received = 0;
      return;
    }   
    
	/////////////////////////////////////////////////////////////////////////////////////
	//execute function
	/*disable*/
	if(DTMFReceiveBuffer[8] == '0') {
		output_set(channel_num, 0);
		dtmf_beep(0);
	}
	/*enable*/
	if(DTMFReceiveBuffer[8] == '1') {
		output_set(channel_num, 1);
		dtmf_beep(1);
	}
	/*pulse*/
	if(DTMFReceiveBuffer[8] == 'D') {
		start_pulse(channel_num, 1);
		/*the pulse is stopped automatically with the timer interrupt*/
	}
	/*ask*/
	if(DTMFReceiveBuffer[8] == 'A') {
		if(output_get(channel_num)) { dtmf_beep(1); }
		else { dtmf_beep(0); }
	}
	/////////////////////////////////////////////////////////////////////////////////////
    
    DTMFReceiveCounter = 0;
    dtmf.first_received = 0;
	eeprom_write_byte((uint8_t *)PORTA_EEPROM_STORE,PINA);
	eeprom_write_byte((uint8_t *)PORTC_EEPROM_STORE,PINC);
  }
}



//****************************************************************************
//PRIVATE FUNCTIONS

//----------------------------------------------------------------------------
// Activate the beep
//parameter tone: 0 = tone high->low / non 0 = tone low->high
void dtmf_beep (unsigned char tone) {
  if(tone) { dtmf.tone = 1; }
  else     { dtmf.tone = 0; }
  dtmf.beep = 1;
  beep_counter = 0;
}

//----------------------------------------------------------------------------
// Set output helper
void output_set(unsigned char output, unsigned char enable) {
		switch(output) {
			case  1:
				if(enable ^ INVERT_01)
					PORTC |= (1<<PC0);
				else
					PORTC &= ~(1<<PC0);
				break;
			case  2:
				if(enable ^ INVERT_02)
					PORTC |= (1<<PC1);
				else
					PORTC &= ~(1<<PC1);
				break;
			case  3:
				if(enable ^ INVERT_03)
					PORTC |= (1<<PC2);
				else
					PORTC &= ~(1<<PC2);
				break;
			case  4:
				if(enable ^ INVERT_04)
					PORTC |= (1<<PC3);
				else
					PORTC &= ~(1<<PC3);
				break;
			case  5:
				if(enable ^ INVERT_05)
					PORTC |= (1<<PC4);
				else
					PORTC &= ~(1<<PC4);
				break;
			case  6:
				if(enable ^ INVERT_06)
					PORTC |= (1<<PC5);
				else
					PORTC &= ~(1<<PC5);
				break;
			case  7:
				if(enable ^ INVERT_07)
					PORTC |= (1<<PC6);
				else
					PORTC &= ~(1<<PC6);
				break;
			case  8:
				if(enable ^ INVERT_08)
					PORTC |= (1<<PC7);
				else
					PORTC &= ~(1<<PC7);
				break;
			case  9:
				if(enable ^ INVERT_09)
					PORTA |= (1<<PA0);
				else
					PORTA &= ~(1<<PA0);
				break;
			case 10:
				if(enable ^ INVERT_10)
					PORTA |= (1<<PA1);
				else
					PORTA &= ~(1<<PA1);
				break;
			case 11:
				if(enable ^ INVERT_11)
					PORTA |= (1<<PA2);
				else
					PORTA &= ~(1<<PA2);
				break;
			case 12:
				if(enable ^ INVERT_12)
					PORTA |= (1<<PA3);
				else
					PORTA &= ~(1<<PA3);
				break;
		}
}

//----------------------------------------------------------------------------
// Get output helper
unsigned char output_get(unsigned char output) {
	switch(output) {
		case  1:
			if(!INVERT_01)
				return (PINC & 0x01);
			else
				return !(PINC & 0x01);
		case  2:
			if(!INVERT_02)
				return (PINC & 0x02);
			else
				return !(PINC & 0x02);
		case  3:
			if(!INVERT_03)
				return (PINC & 0x04);
			else
				return !(PINC & 0x04);
		case  4:
			if(!INVERT_04)
				return (PINC & 0x08);
			else
				return !(PINC & 0x08);
		case  5:
			if(!INVERT_05)
				return (PINC & 0x10);
			else
				return !(PINC & 0x10);
		case  6:
			if(!INVERT_06)
				return (PINC & 0x20);
			else
				return !(PINC & 0x20);
		case  7:
			if(!INVERT_07)
				return (PINC & 0x40);
			else
				return !(PINC & 0x40);
		case  8:
			if(!INVERT_08)
				return (PINC & 0x80);
			else
				return !(PINC & 0x80);
		case  9:
			if(!INVERT_09)
				return (PINA & 0x01);
			else
				return !(PINA & 0x01);
		case 10:
			if(!INVERT_10)
				return (PINA & 0x02);
			else
				return !(PINA & 0x02);
		case 11:
			if(!INVERT_11)
				return (PINA & 0x04);
			else
				return !(PINA & 0x04);
		case 12:
			if(!INVERT_12)
				return (PINA & 0x08);
			else
				return !(PINA & 0x08);
	}
}

//----------------------------------------------------------------------------
// start the pulse
void start_pulse(unsigned char output, unsigned char do_beep) {
	/*if pulse already running: abort*/
	if(pulse_running)
		return;
	/*start the pulse*/
	pulse_channel = output;
	pulse_timer = 2 * PULSELENGTH;
	pulse_running = 1;
	/*toggle the output and beep*/
	if(output_get(output)) {
		output_set(output, 0);
		if(do_beep) { dtmf_beep(0); }
	}
	else {
		output_set(output, 1);
		if(do_beep) { dtmf_beep(1); }
	}
}

//----------------------------------------------------------------------------
// stop the pulse
void stop_pulse(void) {
	/*stop the pulse*/
	pulse_running = 0;
	pulse_timer = 0;
	/*toggle the output and beep*/
	if(output_get(pulse_channel)) {
		output_set(pulse_channel, 0);
		dtmf_beep(0);
	}
	else {
		output_set(pulse_channel, 1);
		dtmf_beep(1);
	}	
}

//----------------------------------------------------------------------------
// When INT1 is triggered a new DTMF symbol is ready
ISR (INT1_vect) {
  // read symbol
  DTMFSymbol = DTMF2ASCII(DTMFPORTIN);
  dtmf.new = 1;
}

//----------------------------------------------------------------------------
// TMR2 generates a 2000 Hz tick for the beep generation and pulse generation
ISR (TIMER2_COMP_vect) {
  //TCNT2 = 0; //happens automatically within the CTC2 mode
	
  //check for pulse and decrement
  if(pulse_running) {
    
	pulse_timer--;
    
	if(pulse_timer == 0)
      stop_pulse();
  }
	
  //check for beep and do beep logic
  if(dtmf.beep) {
    
	beep_counter++;
    
    //prepause 3 sec
    if(beep_counter >= 0 && beep_counter < 6000) {
      PORTB &= ~(1<<PB0); // clear PTT
      PORTB &= ~(1<<PB3); // clear BEEP (it's a pause after all)
    }
    
    //start & pause
    if(beep_counter >= 6000 && beep_counter < 10000) {
      PORTB |=  (1<<PB0); // set PTT
      PORTB &= ~(1<<PB3); // clear BEEP (it's a pause after all)
    }
    
    //tone1
    if(beep_counter >= 10000 && beep_counter < 11000) {
      //tone high: first tone low
      if(dtmf.tone) {
        if((beep_counter>>1) & 0x0001) { PORTB |= (1<<PB3); }
        else { PORTB &= ~(1<<PB3); }
      }
      //tone low: first tone high
      else {
        if(beep_counter & 0x0001) { PORTB |= (1<<PB3); }
        else { PORTB &= ~(1<<PB3); }
      }
    }
    
    //tone2
    if(beep_counter >= 11000 && beep_counter < 12000) {
      //tone high: second tone high
      if(dtmf.tone) {
        if(beep_counter & 0x0001) { PORTB |= (1<<PB3); }
        else { PORTB &= ~(1<<PB3); }
      }
      //tone low: second tone low
      else {
        if((beep_counter>>1) & 0x0001) { PORTB |= (1<<PB3); }
        else { PORTB &= ~(1<<PB3); }
      }
    }
    
    //end
    if(beep_counter >= 12000) {
      PORTB &= ~(1<<PB0); // clear PTT
      PORTB &= ~(1<<PB3); // clear BEEP
      beep_counter = 0;
      dtmf.beep = 0;
    }
  }
}

//----------------------------------------------------------------------------
unsigned char DTMF2ASCII(unsigned char DTMFCode) {
    /*
    FLOW 	FHIGH 	KEY     TOW Q4 Q3 Q2 Q1
    697 	1209 	1 	     H   0  0  0  1
    697 	1336 	2 	     H   0  0  1  0
    697 	1477 	3 	     H   0  0  1  1
    770 	1209 	4      	 H   0  1  0  0
    770 	1336 	5 	     H   0  1  0  1
    770 	1477 	6 	     H   0  1  1  0
    852 	1209 	7        H   0  1  1  1
    852 	1336 	8 	     H   1  0  0  0
    852 	1477 	9 	     H   1  0  0  1
    941 	1336 	0 	     H   1  0  1  0
    941 	1209 	* 	     H   1  0  1  1
    941 	1477 	# 	     H   1  1  0  0
    697 	1633 	A 	     H   1  1  0  1
    770 	1633 	B 	     H   1  1  1  0
    852 	1633 	C 	     H   1  1  1  1
    941 	1633 	D 	     H   0  0  0  0
    */
    switch (DTMFCode) {	//DTMF-Zeichen werden in ASCII umgewandelt
        case 0b1010:
            return '0';
        case 0b1011:
            return '*';
        case 0b1100:
            return '#';
        case 0b1101:
            return 'A';
        case 0b1110:
            return 'B';
        case 0b1111:
            return 'C';
        case 0b0000:
            return 'D';
        default:
            return DTMFCode+'0';
    }
}
