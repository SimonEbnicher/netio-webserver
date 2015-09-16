/*----------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        24.10.2007
 Description:    Webserver uvm.
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
----------------------------------------------------------------------------*/

#include <avr/io.h>
#include <avr/wdt.h>
#include "config.h"
#include "usart.h"
#include "analog.h"
#include "enc28j60.h"
#include "stack.h"
#include "timer.h"
#include "httpd.h"
#include "cmd.h"
#include "base64.h"
#include "dtmfin.h"

//----------------------------------------------------------------------------
//Hier startet das Hauptprogramm
int main(void)
{  
	//Konfiguration der Ausgänge bzw. Eingänge
	//definition erfolgt in der config.h
	DDRA = OUTA;
	PORTA &= 0x0F;
	DDRC = OUTC;
	DDRD = OUTD;
	
	unsigned long a;
	
	usart_init(BAUDRATE); // setup the UART
	
	#if USE_ADC
		ADC_Init();
	#endif
	
	usart_write("\n\rSystem Ready\n\r");
    
	for(a=0;a<1000000;a++){asm("nop");};

	//Applikationen starten
	stack_init();
	httpd_init();
    dtmf_init();
	
	//Ethernetcard Interrupt enable
	ETH_INT_ENABLE;
	
	//Globale Interrupts einschalten
	sei(); 
	
	usart_write("\r\nIP   %1i.%1i.%1i.%1i\r\n", myip[0]     , myip[1]     , myip[2]     , myip[3]);
	usart_write("MASK %1i.%1i.%1i.%1i\r\n", netmask[0]  , netmask[1]  , netmask[2]  , netmask[3]);
	usart_write("GW   %1i.%1i.%1i.%1i\r\n", router_ip[0], router_ip[1], router_ip[2], router_ip[3]);
	usart_write("DTMF Password: %c %c %c %c\r\n", dtmfpw.pw1, dtmfpw.pw2, dtmfpw.pw3, dtmfpw.pw4);
	
	eeprom_busy_wait();
	PORTA = eeprom_read_byte((uint8_t *)PORTA_EEPROM_STORE) & 0x0F;
	eeprom_busy_wait();
	PORTC = eeprom_read_byte((uint8_t *)PORTC_EEPROM_STORE);
		
    wdt_enable(WDTO_2S);
	
	//beep after init
	dtmf_beep(1);
        
	while(1)
	{
		ANALOG_ON;
		
		eth_get_data();
		
		//Terminalcommandos auswerten
		if (usart_status.usart_ready){
			usart_write("\r\n");
			if(extract_cmd(&usart_rx_buffer[0]))
			{
				usart_write("Ready\r\n\r\n");
			}
			else
			{
				usart_write("ERROR\r\n\r\n");
			}
			usart_status.usart_ready =0;
		}
	
        if(ping.result) {
            usart_write("Get PONG: %i.%i.%i.%i\r\n",ping.ip1[0],ping.ip1[1],ping.ip1[2],ping.ip1[3]); 
            ping.result = 0;
        }
        
        // DTMF
		dtmf_do();
		
		//Toggle Watchdog Output
		PORTD ^= 1 << PD2;
        
        //Reset Watch Dog Timer
        wdt_reset();
	
    }//while (1)
		
return(0);
}

