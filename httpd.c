/*----------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        24.10.2007
 Description:    Webserver Applikation
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
#include "httpd.h"
#include "webpage.h"
#include "dtmfin.h"
#include <util/delay.h>

struct http_table http_entry[MAX_TCP_ENTRY];

//Hier wird das codierte Passwort aus config.h gespeichert.
unsigned char http_auth_passwort[30];

unsigned char post_in[5] = {'O','U','T','='};
unsigned char post_ready[5] = {'S','U','B','='};
unsigned char PORT_tmp = 0;
char dstr[24]={"No Time...             "};

//----------------------------------------------------------------------------
//Variablenarry zum einfügen in Webseite %VA@00 bis %VA@09
unsigned int var_array[MAX_VAR_ARRAY] = {10,50,30,2,0,0,0,0,0,0};
//----------------------------------------------------------------------------

PROGMEM const char http_header1[]={	"HTTP/1.0 200 Document follows\r\n"
					"Server: AVR_Small_Webserver\r\n"
					"Content-Type: text/html\r\n\r\n"};

PROGMEM const char http_header2[]={	"HTTP/1.0 200 Document follows\r\n"
					"Server: AVR_Small_Webserver\r\n"
					"Content-Type: image/jpg\r\n\r\n"};

PROGMEM const char http_header3[]={	"HTTP/1.0 401 Unauthorized\r\n"
					"Server: AVR_Small_Webserver\r\n"
					"WWW-Authenticate: Basic realm=\"NeedPassword\""
					"\r\nContent-Type: text/html\r\n\r\n"};

//----------------------------------------------------------------------------
//Kein Zugriff Seite bei keinem Passwort
PROGMEM const char Page0[] = {"401 Unauthorized%END"};

unsigned char rx_header_end[5] = {"\r\n\r\n\0"};

//----------------------------------------------------------------------------
//Initialisierung des Httpd Testservers
void httpd_init (void)
{
	//HTTP_AUTH_STRING 
	decode_base64((unsigned char*)HTTP_AUTH_STRING,http_auth_passwort);

	//Serverport und Anwendung eintragen
	add_tcp_app (HTTPD_PORT, (void(*)(unsigned char))httpd);
}
   
//----------------------------------------------------------------------------
//http Testserver
void httpd (unsigned char index)
{
    //Verbindung wurde abgebaut!
    if (tcp_entry[index].status & FIN_FLAG)
    {
        return;
    }

	//Allererste Aufruf des Ports für diese Anwendung
	//HTTPD_Anwendungsstack löschen
	if(tcp_entry[index].app_status==1)
	{
		httpd_stack_clear(index);
	}
	
	//HTTP wurde bei dieser Verbindung zum ersten mal aufgerufen oder
	//HTML Header Retransmission!
	if (tcp_entry[index].app_status <= 2)
	{	
		httpd_header_check (index);
		return;
	}
	
	//Der Header wurde gesendet und mit ACK bestätigt (tcp_entry[index].app_status+1)
	//war das HTML Packet fertig, oder müssen weitere Daten gesendet werden, oder Retransmission?
	if (tcp_entry[index].app_status > 2 && tcp_entry[index].app_status < 0xFFFE)
	{
		httpd_data_send (index);
		return;
	}
	
	//Verbindung kann geschlossen werden! Alle HTML Daten wurden gesendet TCP Port kann
	//geschlossen werden (tcp_entry[index].app_status >= 0xFFFE)!!
	if (tcp_entry[index].app_status >= 0xFFFE)
	{
		tcp_entry[index].app_status = 0xFFFE;
		tcp_Port_close(index);
		return;
	}
	return;
}

//----------------------------------------------------------------------------
//HTTPD_STACK löschen
void httpd_stack_clear (unsigned char index)
{
	http_entry[index].http_header_type =0;
	http_entry[index].first_switch = 0;
	http_entry[index].http_auth = HTTP_AUTH_DEFAULT;
	http_entry[index].new_page_pointer = 0;
	http_entry[index].old_page_pointer = 0;
	http_entry[index].post = 0;
	http_entry[index].auth_ptr = http_auth_passwort;
	http_entry[index].post_ptr = post_in;
	http_entry[index].post_ready_ptr = post_ready;
	http_entry[index].hdr_end_pointer = rx_header_end;
	HTTP_DEBUG("\r\n**** NEUE HTTP ANFORDERUNG ****\r\n\r\n");	
	return;
}

//----------------------------------------------------------------------------
//Eintreffenden Header vom Client checken
void httpd_header_check (unsigned char index)
{
	unsigned int a = 0;
	
	if(strcasestr_P((char*)&eth_buffer[TCP_DATA_START_VAR],PSTR("POST"))!=0)
		{
		http_entry[index].post = 1;
		}
	
	//finden der Authorization und das Ende im Header auch über mehrere Packete hinweg!!
	if(*http_entry[index].hdr_end_pointer != 0)
	{		
		for(a=TCP_DATA_START_VAR;a<(TCP_DATA_END_VAR);a++)
		{	
			HTTP_DEBUG("%c",eth_buffer[a]);
			
			if(!http_entry[index].http_auth) 
			{
				if (eth_buffer[a] != *http_entry[index].auth_ptr++)
				{
					http_entry[index].auth_ptr = http_auth_passwort;
				}
				if(*http_entry[index].auth_ptr == 0) 
				{
					http_entry[index].http_auth = 1;
					HTTP_DEBUG("  <---LOGIN OK!--->\r\n");
				}
			}
			
			if (eth_buffer[a] != *http_entry[index].hdr_end_pointer++)
			{
				http_entry[index].hdr_end_pointer = rx_header_end;
			}
			
			//Das Headerende wird mit (CR+LF+CR+LF) angezeigt!
			if(*http_entry[index].hdr_end_pointer == 0) 
			{
				HTTP_DEBUG("<---HEADER ENDE ERREICHT!--->\r\n");
				break;
			}
		}
	}
	
	//Einzelne Postpacket (z.B. bei firefox)
	if(http_entry[index].http_auth && http_entry[index].post == 1)
	{
		for(a = TCP_DATA_START_VAR;a<(TCP_DATA_END_VAR);a++)
		{	
			//Schaltanweisung finden!
			if (eth_buffer[a] != *http_entry[index].post_ptr++)
			{
				http_entry[index].post_ptr = post_in;
			}
			if(*http_entry[index].post_ptr == 0) 
			{
				switch (eth_buffer[a+1])
				  {
				    case ('A'):
					  if(output_get(1)) { output_set(1, 0); }
                      else { output_set(1, 1); }
				      break;
				    case ('B'):
					  if(output_get(2)) { output_set(2, 0); }
					  else { output_set(2, 1); }
				      break;
				    case ('C'):
					  if(output_get(3)) { output_set(3, 0); }
					  else { output_set(3, 1); }
				      break;
				    case ('D'):
					  if(output_get(4)) { output_set(4, 0); }
					  else { output_set(4, 1); }
				      break;
				    case ('E'):
					  if(output_get(5)) { output_set(5, 0); }
					  else { output_set(5, 1); }
				      break;
				    case ('F'):
					  if(output_get(6)) { output_set(6, 0); }
					  else { output_set(6, 1); }
				      break;
				    case ('G'):
					  if(output_get(7)) { output_set(7, 0); }
					  else { output_set(7, 1); }
				      break;
				    case ('H'):
					  if(output_get(8)) { output_set(8, 0); }
					  else { output_set(8, 1); }
				      break;
					case ('I'):
					  if(output_get(9)) { output_set(9, 0); }
					  else { output_set(9, 1); }
					  break;
					case ('J'):
					  if(output_get(10)) { output_set(10, 0); }
					  else { output_set(10, 1); }
					  break;
					case ('K'):
					  if(output_get(11)) { output_set(11, 0); }
					  else { output_set(11, 1); }
					  break;
					case ('L'):
					  if(output_get(12)) { output_set(12, 0); }
					  else { output_set(12, 1); }
					  break;
					case ('M'):
					  start_pulse(1, 0);
					  break;
					case ('N'):
					  start_pulse(2, 0);
					  break;
					case ('O'):
					  start_pulse(3, 0);
					  break;
					case ('P'):
					  start_pulse(4, 0);
					  break;
					case ('Q'):
					  start_pulse(5, 0);
					  break;
					case ('R'):
					  start_pulse(6, 0);
					  break;
					case ('S'):
					  start_pulse(7, 0);
					  break;
					case ('T'):
					  start_pulse(8, 0);
					  break;
					case ('U'):
					  start_pulse(9, 0);
					  break;
					case ('V'):
					  start_pulse(10, 0);
					  break;
					case ('W'):
					  start_pulse(11, 0);
					  break;
					case ('X'):
					  start_pulse(12, 0);
					  break;
				  }
				http_entry[index].post_ptr = post_in;
				//Schaltanweisung wurde gefunden
			}
		
			//Submit schließt die suche ab!
			if (eth_buffer[a] != *http_entry[index].post_ready_ptr++)
			{
				http_entry[index].post_ready_ptr = post_ready;
			}
			if(*http_entry[index].post_ready_ptr == 0) 
			{
				http_entry[index].post = 0;

				/*if((PORT_tmp & 0x02)||(PORT_tmp & 0x20)||(PORT_tmp & 0x40)){
				if (PORT_tmp & 0x02){	// Auf PC1 wird nur ein Impuls ausgegeben
					PORTC |= (1<<PC1);
					_delay_ms(100);
					PORTC &= ~(1<<PC1);
				}

				if(PORT_tmp & 0x20){	// Auf PC6 wird nur ein Impuls ausgegeben
					PORTC |= (1<<PC5);
					_delay_ms(100);
					PORTC &= ~(1<<PC5);
				}

				if(PORT_tmp & 0x40){	// Auf PC6 wird nur ein Impuls ausgegeben
					PORTC |= (1<<PC6);
					_delay_ms(100);
					PORTC &= ~(1<<PC6);
				}
					PORT_tmp = 0;
				}
				else
				{
					PORTC = PORT_tmp;
                	PORT_tmp = 0;
				}*/
				eeprom_busy_wait();
				eeprom_write_byte((uint8_t *)PORTA_EEPROM_STORE,PINA);
				eeprom_busy_wait();
				eeprom_write_byte((uint8_t *)PORTC_EEPROM_STORE,PINC);
				break;
				//Submit gefunden
			}
		}
	}	
	
	//Welche datei wird angefordert? Wird diese in der Flashspeichertabelle gefunden?
	unsigned char page_index = 0;
	
	if (!http_entry[index].new_page_pointer)
	{
		for(a = TCP_DATA_START_VAR+5;a<(TCP_DATA_END_VAR);a++)
		{
			if (eth_buffer[a] == '\r')
			{
				eth_buffer[a] = '\0';
				break;
			}
		}
	
		while(WEBPAGE_TABLE[page_index].filename)
		{
			if (strcasestr((char*)&eth_buffer[TCP_DATA_START_VAR],WEBPAGE_TABLE[page_index].filename)!=0) 
				{
					http_entry[index].http_header_type = 1;
					HTTP_DEBUG("\r\n\r\nDatei gefunden: ");
					HTTP_DEBUG("%s",(char*)WEBPAGE_TABLE[page_index].filename);
					HTTP_DEBUG("<----------------\r\n\r\n");	
					if (strcasestr(WEBPAGE_TABLE[page_index].filename,".jpg")!=0)
					{
						http_entry[index].http_header_type = 1;
					}
					if (strcasestr(WEBPAGE_TABLE[page_index].filename,".gif")!=0)
					{
						http_entry[index].http_header_type = 1;
					}	
					if (strcasestr(WEBPAGE_TABLE[page_index].filename,".htm")!=0)
					{
						http_entry[index].http_header_type = 0;	
					}	
					http_entry[index].new_page_pointer = WEBPAGE_TABLE[page_index].page_pointer;
					break;
				}
			page_index++;
		}
	}

	//Wurde das Ende vom Header nicht erreicht
	//kommen noch weitere Stücke vom Header!
	if ((*http_entry[index].hdr_end_pointer != 0) || (http_entry[index].post == 1))
	{
		//Der Empfang wird Quitiert und es wird auf weiteres Headerstück gewartet
		tcp_entry[index].status =  ACK_FLAG;
		create_new_tcp_packet(0,index);
		//Warten auf weitere Headerpackete
		tcp_entry[index].app_status = 1;
		return;
	}	
	
	//Wurde das Passwort in den ganzen Headerpacketen gefunden?
	//Wenn nicht dann ausführen und Passwort anfordern!
	if((!http_entry[index].http_auth) && tcp_entry[index].status&PSH_FLAG)
	{	
		//HTTP_AUTH_Header senden!
		http_entry[index].new_page_pointer = Page0;
		memcpy_P((char*)&eth_buffer[TCP_DATA_START_VAR],http_header3,(sizeof(http_header3)-1));
		tcp_entry[index].status =  ACK_FLAG | PSH_FLAG;
		create_new_tcp_packet((sizeof(http_header3)-1),index);
		tcp_entry[index].app_status = 2;
		return;
	}
	
	//Standart INDEX.HTM Seite wenn keine andere gefunden wurde
	if (!http_entry[index].new_page_pointer)
	{
		//Besucher Counter
		var_array[MAX_VAR_ARRAY-1]++;
		
		http_entry[index].new_page_pointer = Page1;
		http_entry[index].http_header_type = 0;
	}	
	
	tcp_entry[index].app_status = 2;
	//Seiten Header wird gesendet
	if(http_entry[index].http_header_type == 1)
	{
		memcpy_P((char*)&eth_buffer[TCP_DATA_START_VAR],http_header2,(sizeof(http_header2)-1));
        tcp_entry[index].status =  ACK_FLAG | PSH_FLAG;
        create_new_tcp_packet((sizeof(http_header2)-1),index);
        return;
	}
     
	if(http_entry[index].http_header_type == 0)
	{
		memcpy_P((char*)&eth_buffer[TCP_DATA_START_VAR],http_header1,(sizeof(http_header1)-1));
        tcp_entry[index].status =  ACK_FLAG | PSH_FLAG;
        create_new_tcp_packet((sizeof(http_header1)-1),index);
        return;
	}
    return;
}

//----------------------------------------------------------------------------
//Daten Packete an Client schicken
void httpd_data_send (unsigned char index)
{	
	unsigned int a;
	unsigned char str_len;
	
	char var_conversion_buffer[CONVERSION_BUFFER_LEN];
	
	//Passwort wurde im Header nicht gefunden
	if(!http_entry[index].http_auth)
	{
		http_entry[index].new_page_pointer = Page0;
	}
	//kein Packet empfangen Retransmission des alten Packetes
	if (tcp_entry[index].status == 0) 
	{
		http_entry[index].new_page_pointer = http_entry[index].old_page_pointer;
	}
	http_entry[index].old_page_pointer = http_entry[index].new_page_pointer;

	for (a = 0;a<(MTU_SIZE-(TCP_DATA_START)-150);a++)
	{
		unsigned char b;
		b = pgm_read_byte(http_entry[index].new_page_pointer++);
		eth_buffer[TCP_DATA_START + a] = b;
		
		//Müssen Variablen ins Packet eingesetzt werden? ===> %VA@00 bis %VA@09
		if (b == '%')
		{
			
                        if (strncasecmp_P("VA@",http_entry[index].new_page_pointer,3)==0)
			{	
				b = (pgm_read_byte(http_entry[index].new_page_pointer+3)-48)*10;
				b +=(pgm_read_byte(http_entry[index].new_page_pointer+4)-48);
				
				if ((b==4)||(b==5)) {			//	Umrechnung für Temperatur
					int wert;
					wert=(long)var_array[b];
					wert/=2;
 					wert-=50;
					itoa(wert,&var_conversion_buffer[0],10);
				}
				else if ((b==6)||(b==7)) {			//	Umrechnung für Spannung 0...1000 Schritte = 0...50V / Schrittweite 50mV
					uint16_t wert;
					wert=var_array[b]*50; //spannung in mV
					utoa(wert,&var_conversion_buffer[0],10);
				}
				str_len = strnlen(var_conversion_buffer,CONVERSION_BUFFER_LEN);
				memmove(&eth_buffer[TCP_DATA_START+a],var_conversion_buffer,str_len);
				a = a + (str_len-1);
				http_entry[index].new_page_pointer=http_entry[index].new_page_pointer+5;
			}
			
			
            
			//Einsetzen des Port Status %PORTxy durch "checked" wenn Portx.Piny = 1
			//x: A..G  y: 0..7 
			if (strncasecmp_P("PORT",http_entry[index].new_page_pointer,4)==0)
			{
				unsigned char pin  = (pgm_read_byte(http_entry[index].new_page_pointer+5)-48);	
				b = 0;
				switch(pgm_read_byte(http_entry[index].new_page_pointer+4))
				{
					case 'A':
						b = (PORTA & (1<<pin));
						break;
					case 'B':
						b = (PORTB & (1<<pin));
						break;
					case 'C':
						b = (PORTC & (1<<pin));
						break;
					case 'D':
						b = (PORTD & (1<<pin));
						break; 
				}
				
				if(b)
				{
					strcpy_P(var_conversion_buffer, PSTR("checked"));
				}
				else
				{
					strcpy_P(var_conversion_buffer, PSTR("\0"));
				}
				str_len = strnlen(var_conversion_buffer,CONVERSION_BUFFER_LEN);
				memmove(&eth_buffer[TCP_DATA_START+a],var_conversion_buffer,str_len);
				a += str_len-1;
				http_entry[index].new_page_pointer = http_entry[index].new_page_pointer+6;
			}
			
			//Einsetzen des Pin Status %PI@xy bis %PI@xy durch "ledon" oder "ledoff"
			//x = 0 : PINA / x = 1 : PINB / x = 2 : PINC / x = 3 : PIND
			if (strncasecmp_P("PIN",http_entry[index].new_page_pointer,3)==0)
			{
				unsigned char pin  = (pgm_read_byte(http_entry[index].new_page_pointer+4)-48);	
				b = 0;
				switch(pgm_read_byte(http_entry[index].new_page_pointer+3))
				{
					case 'A':
						//b = (PINA & (1<<pin));
						switch(pin) {
							case 0:
								b = output_get(9);
								break;
							case 1:
								b = output_get(10);
								break;
							case 2:
								b = output_get(11);
								break;
							case 3:
								b = output_get(12);
								break;
							default:
								break;
						}
						break;
					case 'B':
						//b = (PINB & (1<<pin));
						break;
					case 'C':
						switch(pin) {
							case 0:
								b = output_get(1);
								break;
							case 1:
								b = output_get(2);
								break;
							case 2:
								b = output_get(3);
								break;
							case 3:
								b = output_get(4);
								break;
							case 4:
								b = output_get(5);
								break;
							case 5:
								b = output_get(6);
								break;
							case 6:
								b = output_get(7);
								break;
							case 7:
								b = output_get(8);
								break;
							default:
								break;
						}
						//b = (PINC & (1<<pin));
						//if(pin>=4) { b = !b; } //this line inverts the display behaviour if PC4...7
						break;
					case 'D':
						//b = (PIND & (1<<pin));
						break;    
				}
				
				if(b)
				{
                                        strcpy_P(var_conversion_buffer, PSTR("ledon.gif"));
					//strcpy_P(var_conversion_buffer, PSTR("ledoff.gif"));
				}
				else
				{
                                        strcpy_P(var_conversion_buffer, PSTR("ledoff.gif"));
                                        //strcpy_P(var_conversion_buffer, PSTR("ledon.gif"));
				}
				str_len = strnlen(var_conversion_buffer,CONVERSION_BUFFER_LEN);
				memmove(&eth_buffer[TCP_DATA_START+a],var_conversion_buffer,str_len);
				a += str_len-1;
				http_entry[index].new_page_pointer = http_entry[index].new_page_pointer+5;
			}
			//wurde das Ende des Packetes erreicht?
			//Verbindung TCP Port kann beim nächsten ACK geschlossen werden
			//Schleife wird abgebrochen keine Daten mehr!!
			if (strncasecmp_P("END",http_entry[index].new_page_pointer,3)==0)
			{	
				tcp_entry[index].app_status = 0xFFFD;
				break;
			}
		}
	}
	//Erzeugte Packet kann nun gesendet werden!
	tcp_entry[index].status =  ACK_FLAG | PSH_FLAG;
	create_new_tcp_packet(a,index);
	return;
}


