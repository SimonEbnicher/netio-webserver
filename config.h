/*----------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        03.11.2007
 Description:    Webserver Config-File
 Modified:       G. Menke, 15.03.2009

 Dieses Programm ist freie Software. Sie können es unter den Bedingungen der 
 GNU General Public License, wie von der Free Software Foundation veröffentlicht, 
 weitergeben und/oder modifizieren, entweder gemäß Version 2 der Lizenz oder 
 (nach Ihrer Option) jeder späteren Version. 

 Die Veröffentlichung dieses Programms erfolgt in der Hoffnung, 
 daß es Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE, 
 sogar ohne die implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT 
 FÜR EINEN BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public License. 

 Sie sollten eine Kopie der GNU General Public License zusammen mit diesem 
 Programm erhalten haben. 
 Falls nicht, schreiben Sie an die Free Software Foundation, 
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA. 
------------------------------------------------------------------------------*/

#ifndef _CONFIG_H_
	#define _CONFIG_H_	
	
	//Konfiguration der PORTS (HEX)
	//1=OUTPUT / 0=INPUT

	// PA7=ADCIN4       IN
	// PA6=ADCIN3       IN
	// PA5=ADCIN2       IN
	// PA4=ADCIN1       IN
	// PA3=DIGOUT12     OUT
	// PA2=DIGOUT11     OUT
	// PA1=DIGOUT10     OUT
	// PA0=DIGOUT9      OUT
	#define OUTA 		0x0F

	//Achtung!!!! an PORTB ist der ENC
	//nur ändern wenn man weiß was man macht!
	// PB0=DTMF PTT     OUT
	// PB3=DTMF BEEP    OUT

	// PC7=DIGOUT8      OUT
	// PC6=DIGOUT7      OUT
	// PC5=DIGOUT6      OUT
	// PC4=DIGOUT5      OUT
	// PC3=DIGOUT4      OUT
	// PC2=DIGOUT3      OUT
	// PC1=DIGOUT2      OUT
	// PC0=DIGOUT1      OUT
	#define OUTC 		0xFF

	// Connectior "EXT."
	// PD7=DTMF_D3      IN
	// PD6=DTMF_D2      IN
	// PD5=DTMF_D1      IN
	// PD4=DTMF_D0      IN
	// PD3=DTMF_INT     IN
	// PD2=WATCHDOG_PULSE_OUT	OUT
	#define OUTD 		0x04

	//Watchdog timer for the ENC2860, resets the stack if timeout occurs
	#define WTT 1200 //Watchdog timer in timer interrupt

	//Umrechnung von IP zu unsigned long
	#define IP(a,b,c,d) ((unsigned long)(d)<<24)+((unsigned long)(c)<<16)+((unsigned long)(b)<<8)+a

	//IP des Webservers und des Routers
	#define MYIP		IP(44,134,190,23)
	#define ROUTER_IP	IP(44,134,190,17)


	//Netzwerkmaske
	#define NETMASK		IP(255,255,255,240)
	
	//MAC Adresse des Webservers	
	#define MYMAC1	0x00
	#define MYMAC2	0x22
	#define MYMAC3	0xF9
	#define MYMAC4	0x01	
	#define MYMAC5	0x87
	#define MYMAC6	0x38
	
	//Taktfrequenz
	#define F_CPU 16000000UL	
	
	//Timertakt intern oder extern
	#define EXTCLOCK 0 //0=Intern 1=Externer Uhrenquarz

	//Baudrate der seriellen Schnittstelle
	#define BAUDRATE 9600

	//Webserver mit Passwort? (0 == mit Passwort)
	#define HTTP_AUTH_DEFAULT	0
	
	//AUTH String "USERNAME:PASSWORT" max 14Zeichen 
	//für Username:Passwort
	#define HTTP_AUTH_STRING "user:passwd"

	//ADC
	#define USE_ADC		1
	#define NUM_MEAN_0	10	//Anzahl der zu mittelnden Messwerte (Max. 255) Kanal 0
	#define NUM_MEAN_1	10	//Anzahl der zu mittelnden Messwerte (Max. 255) Kanal 1
	#define NUM_MEAN_2	10	//Anzahl der zu mittelnden Messwerte (Max. 255) Kanal 2
	#define NUM_MEAN_3	10	//Anzahl der zu mittelnden Messwerte (Max. 255) Kanal 3

	// Portsteuerung über DTMF möglich
	#define USE_DTMF 1
	#define DTMF_TIMEOUT			15		//dtmf timeout in seconds for partial sequences
	#define DTMF_DEFAULTPASSWORD_1 '0'
	#define DTMF_DEFAULTPASSWORD_2 '0'
	#define DTMF_DEFAULTPASSWORD_3 '0'
	#define DTMF_DEFAULTPASSWORD_4 '0'
	
	//pulselänge in millisekunden (maximum = 32767 !!!!!!!!)
	#define PULSELENGTH				5000
	
	//EEPROM
	#define PORTA_EEPROM_STORE 		20
	#define PORTC_EEPROM_STORE 		21
	#define IP_EEPROM_STORE			30
	#define NETMASK_EEPROM_STORE	34
	#define ROUTER_IP_EEPROM_STORE	38
	#define DTMF_EEPROM_PASSWORD	50
	
	//EIN/AUSGÄNGE INVERTIEREN (normal auf "0", zum invertieren auf "1" setzen)
	#define INVERT_01	0
	#define INVERT_02	0
	#define INVERT_03	0
	#define INVERT_04	0
	#define INVERT_05	0
	#define INVERT_06	0
	#define INVERT_07	0
	#define INVERT_08	0
	#define INVERT_09	0
	#define INVERT_10	0
	#define INVERT_11	0
	#define INVERT_12	0
	
#endif //_CONFIG_H


