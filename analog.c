/*------------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        31.12.2007
 Description:    Analogeingänge Abfragen
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
#include "analog.h"

#if USE_ADC
volatile unsigned char channel = 0;
uint32_t akkumulator0 = 0;
uint32_t akkumulator1 = 0;
uint32_t akkumulator2 = 0;
uint32_t akkumulator3 = 0;
uint16_t value0 = 0;
uint16_t value1 = 0;
uint16_t value2 = 0;
uint16_t value3 = 0;
uint8_t counter0 = 0;
uint8_t counter1 = 0;
uint8_t counter2 = 0;
uint8_t counter3 = 0;

//------------------------------------------------------------------------------
//
void ADC_Init(void)
{ 
	ADMUX = (1<<REFS0);		// AVCC ist Referenz
	//Free Running Mode, Division Factor 128, Interrupt on
	ADCSRA=(1<<ADEN)|(1<<ADSC)|(1<<ADATE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADIE);
}

//------------------------------------------------------------------------------
//
ISR (ADC_vect)
{
    ANALOG_OFF; //ADC OFF

	//var_array[channel] = ADC;

	if(channel==0)	{
		akkumulator0 += (uint32_t)ADC;
		counter0++;
		if (counter0 >= NUM_MEAN_0) {
			counter0 = 0;
			var_array[4] = akkumulator0/(uint32_t)NUM_MEAN_0;
			akkumulator0 = 0;
		}
	}
	if(channel==1)	{
		akkumulator1 += (uint32_t)ADC;
		counter1++;
		if (counter1 >= NUM_MEAN_1) {
			counter1 = 0;
			var_array[5] = akkumulator1/(uint32_t)NUM_MEAN_1;
			akkumulator1 = 0;
		}
	}
	if(channel==2)	{
		akkumulator2 += (uint32_t)ADC;
		counter2++;
		if (counter2 >= NUM_MEAN_2) {
			counter2 = 0;
			var_array[6] = akkumulator2/(uint32_t)NUM_MEAN_2;
			akkumulator2 = 0;
		}
	}
	if(channel==3)	{
		akkumulator3 += (uint32_t)ADC;
		counter3++;
		if (counter3 >= NUM_MEAN_3) {
			counter3 = 0;
			var_array[7] = akkumulator3/(uint32_t)NUM_MEAN_3;
			akkumulator3 = 0;
		}
	}
	
	channel++;
	if (channel > 3) channel = 0;
    ADMUX =(1<<REFS0) + (channel+4);
    ANALOG_ON; //ADC ON
}

#endif //USE_ADC


