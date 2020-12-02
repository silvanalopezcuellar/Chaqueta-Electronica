/*
 * Chaqueta Electronica.c
 * Proyecto 2
 * Author : Silvana López Cuellar. Maria Paola Fonseca Páez
 */ 

///////////Inclusion de Headers ////////////
#include <avr/io.h>
#include "adc_driver.h"
#include "adc_driver.c"

////////////Declaration of Data///////////
#define ADC PORTB					//PORTB as ADC port
#define PWM PORTE					//PORTE as PWM port
typedef enum STATE state_m;			
enum STATE { temp, light, fit};		//states definition

uint8_t temp_table[256] = {125,125,125,125,125,124,123,122,121,120,119,118,117,116,115,114,	//temperature table
	113,112,111,110,110,109,108,108,107,106,106,105,104,104,103,102,
	102,101,100,100,99,98,98,97,96,96,95,94,94,93,92,92,
	91,90,90,89,89,88,88,87,87,86,86,85,85,84,83,83,
	82,81,80,80,79,79,78,78,77,77,76,76,75,75,74,74,
	73,73,72,72,71,71,70,70,69,69,68,68,68,67,67,66,
	66,65,65,65,64,64,63,63,63,62,62,61,61,61,60,60,
	59,59,59,58,58,57,57,57,56,56,56,55,55,54,54,54,
	53,53,53,52,52,51,51,51,50,50,50,49,49,49,48,48,
	48,47,47,47,46,46,46,45,45,44,44,43,43,43,42,42,
	42,41,41,41,41,40,40,39,39,38,38,38,38,37,37,36,
	36,36,35,35,35,34,34,34,33,33,32,32,31,31,31,30,
	30,29,29,29,28,28,27,27,26,26,25,25,24,24,24,23,
	23,23,22,22,21,21,21,20,20,19,19,18,18,17,17,16,
	16,15,14,14,13,13,12,11,11,10,10,9,8,8,7,6,
	6,5,4,3,3,2,1,0,0,0,0,0,0,0,0,0};

//////////// Prototipo de funciones////////////

void Dynamic_display(int Display_1, int Display_2, int temperature_f);	//runs the dynamic display function
void Seven_segments(int num);											//Shows the number in a display
void Light_function(uint8_t *res);										//Converts the ADC result to light intensity
void ADC_toggle(state_m state_f);										//Toggles the input to the ADC and starts a new conversion
uint8_t temp_value();

///////////   MAIN   /////////////

int main(void)
{
	state_m state = temp;					//State variable begins at temp state
	uint8_t light_result =0;				//Variable to save the light sensor result
	
	int Seven_segments_1=1, Seven_segments_2=0, temperature=0, boolean_1=0, boolean_2=0;
											//Display and push buttons flags (one line up)
	PORTA.DIRSET= 0xFE;						//7 Segments (A,B,C,D,E,F) as outputs
	PORTC.DIRCLR= 0x0c;						//Common anodes and the two push buttons as inputs
	PORTC.DIRSET= 0x03;						//The common anodes in low
	PORTC.OUTCLR= 0x01;						//At the beginning The first display is on
	PORTC.OUTSET= 0x02;						//At the beginning The second display is off
	PORTA.OUTCLR= 0x7E;						//Number 0 initially at the 7 segments
	PORTB.DIRSET= 0xf0;						//The leds of the card as outputs (witnesses)
	PORTB.OUTSET= 0xf0;						//The leds of the card off
	PORTE.DIRSET= 0x0C;						//Pin4 to pin 5 of PORTE as outputs
	TCC0.PER= 2500;						    //Per for 10 milliseconds for the dynamic display timer
	TCC0.CTRLA=0x04;						//Preescal of 8
	ADC.DIRCLR = 0x0F;	     				//Pin0 to pin4 of PORTB as inputs
	PWM.DIRSET = 0x0F;						//Pin0 to pin 4 of PORTE as outputs
	PWM.OUTSET = 0x0F;						//Pin0 to pin 4 to zero

	//ADC
	ADC_CalibrationValues_Load(&ADCB);		//Moves calibration values from memory to CALL/CALH. It's not necessary to understand it
	ADCB.CTRLB = 0x04;						//no limit current, unsigned, no free run, 8 bit resolution
	ADCB.REFCTRL = 0x00;					//internal reference voltage of 1 volt
	ADCB.SAMPCTRL = 0x3F;					//Configurates the maximum sample time
	ADCB.CH0.CTRL=0x01 ;					//single ended, gain factor x1
	ADCB.CH0.MUXCTRL= 0x00;					//Selects pin 1 of PORTB as the ADC input (temperature sensor)
	ADCB.CTRLA |=0x01;						//Enables ADC
	
	//PWM
	TCE0.CTRLB |= 0x03;						//Selects SINGLESLOPE for signal generation
	TCE0.CTRLB |= 0xf0;						//Enables channels A, B, C and D by writing 1 to the CCxEN bit.
	TCE0.CTRLA |= 0x01;						//Sets the prescaler to 1.
	TCE0.PER = 4000;						//Period = CPUfrequency ÷ prescaler ÷ PWMfrecuency; PWMfrecuency=500Hz
	
	
	TCE0.CCABUF = 4000;						//Sets the Duty cycle for channel A to 0%. CCABUF=period-(period*duty_cycle÷100);
	TCE0.CCBBUF = 4000;						//Sets the duty cycle for channel B to 0%.
	TCE0.CCCBUF = 4000;						//Sets the duty cycle for channel C to 0%.
	TCE0.CCDBUF = 4000;						//Sets the duty cycle for channel D to 0%.
	
	ADCB.CH0.CTRL|=0x80;					//Starts an ADC conversión

	
///////////  Finite States Machine  ///////////
	while (1) 
	{
		switch(state)
		{
	
						////////// Temp state /////////
		case temp:
								
			if((ADCB.CH0.INTFLAGS & 0x01) !=0)				//Asks if the ADC conversion is complete
			{
				ADCB.CH0.INTFLAGS |= 0x01;					//Clears the ADC flag
				temperature = temp_table[ADCB.CH0.RESL];	//Save the temperature value in the 'temperature' variable
			
			}
			
			if(temperature<=20)								//Asks if the temp value is under the 'confort' temp value (<20)
			{PORTE.OUTSET =0x02;							//Turns on the heat resistance by switching the rele on
			PORTB.OUTSET =0x10;}							//witness light turns on 
			else if (temperature>=22)						//Asks if the temp value is above the 'confort' temp value	(>22)
			{PORTE.OUTCLR =0x02;							//Turns off the heat resistance by switching the rele off
			PORTB.OUTCLR =0x10;}							//witness light tuns off
								
			if((TCC0.INTFLAGS & 0x01)!=0)					//Each time the time has passed, changes the display that is on
			{
				TCC0.INTFLAGS|=0x01;						//Clear timer
				if(Seven_segments_1==1 && Seven_segments_2==0) 
				{	Seven_segments_1=0;
					Seven_segments_2=1;}
				else if(Seven_segments_2==1 && Seven_segments_1==0)
			   {	Seven_segments_2=0;
					Seven_segments_1=1;}
				Dynamic_display(Seven_segments_1,Seven_segments_2,temperature);	//Calls the Dynamic display function after the display switch
				ADC_toggle (state);							//Toggles the channel input of the ADC to the light sensor and starts a new conversion
				state=light;								//changes the state to light
			}		
		break;
						
						////////// Light state ////////////
						
		case light:
		
			if((ADCB.CH0.INTFLAGS & 0x01) !=0)			//Asks if the ADC conversion is complete
			{
				ADCB.CH0.INTFLAGS |= 0x01;				//Clears the ADC flag
				Light_function(&light_result);			//gets the ADC conversion result and turns it into a value of light intensity
				ADC_toggle (state);						//Toggles the channel input of the ADC to the temperature sensor and starts a new conversion
				state=fit;								//Changes the state to fit
			}					
		break;
						
						///////////  fit state  ////////////
						
		case fit:
			
			if((PORTC.IN & 0x08)==0)						//Asks if the Push button #1 is pressed
			{boolean_1=1;
			PORTB.OUTCLR=0x40;								//The witness #1 turns on
			TCE0.CCCBUF = 2000;								//The motor runs in a direction 1
			TCE0.CCDBUF = 4000;}							
			else if((PORTC.IN & 0x08)!=0 && (boolean_1==1)) //Asks if the Push button #1 is no longer being pressed
			{PORTB.OUTSET=0xF0;								//The witness #1 turns off
			TCE0.CCCBUF = 4000;								//The motor stops
			TCE0.CCDBUF = 4000;
			boolean_1=0;}
			if((PORTC.IN & 0x04)==0)						//Asks if the Push button #2 is pressed
			{boolean_2=1;
			PORTB.OUTCLR=0x80;								//The witness #2 turns on
			TCE0.CCCBUF = 4000;								//The motor runs in a direction 2 (the opposite direction)
			TCE0.CCDBUF = 2000;}
			else if((PORTC.IN & 0x04)!=0 && (boolean_2==1))	//Asks if the Push button #1 is no longer being pressed
			{PORTB.OUTSET=0xF0;								//The witness #2 turns off
			TCE0.CCCBUF = 4000;								//The motor stops
			TCE0.CCDBUF = 4000;
			boolean_2=0;}
				
			state=temp;										//changes the state to temp
		break;
		}
	}
}

/////////// FUNCTIONS ///////////////    Functions of ADC y PWM 


void Light_function(uint8_t *res)
{
	if ((ADCB.CH0.RESL>=0) && (ADCB.CH0.RESL<25))
	{TCE0.CCABUF = 4000;}					//Sets the Duty cycle for channel A to 0%. CCABUF=period-(period*duty_cycle÷100);
	
	if ((ADCB.CH0.RESL>=50) && (ADCB.CH0.RESL<75))
	{TCE0.CCABUF = 3500;}					//Sets the Duty cycle for channel A to 25%
	
	if ((ADCB.CH0.RESL>=75) && (ADCB.CH0.RESL<100))
	{TCE0.CCABUF = 3000;}					//Sets the Duty cycle for channel A to 50%
	
	if ((ADCB.CH0.RESL>=100) && (ADCB.CH0.RESL<=125))
	{TCE0.CCABUF = 2500;}					//Sets the Duty cycle for channel A to 75%
	
	if ((ADCB.CH0.RESL>=125) && (ADCB.CH0.RESL<=150))
	{TCE0.CCABUF = 2000;}					//Sets the Duty cycle for channel A to 100%
		
	if ((ADCB.CH0.RESL>=150) && (ADCB.CH0.RESL<=175))
	{TCE0.CCABUF = 1500;}					//Sets the Duty cycle for channel A to 100%
		
	if ((ADCB.CH0.RESL>=200) && (ADCB.CH0.RESL<=225))
	{TCE0.CCABUF = 1000;}					//Sets the Duty cycle for channel A to 100%
		
	if ((ADCB.CH0.RESL>=225) && (ADCB.CH0.RESL<=255))
	{TCE0.CCABUF = 0;}						//Sets the Duty cycle for channel A to 100%
		
}

void ADC_toggle(state_m state_f)
{
	if(state_f==light)
	{
		ADCB.CH0.MUXCTRL= 0x00;		//Selects pin 0 of PORTB as the ADC input (temperature sensor)
	}
	else
	{
		ADCB.CH0.MUXCTRL= 0x10;		//Selects pin 2 of PORTB as the ADC input (light sensor)
	}
	ADCB.CH0.CTRL|=0x80;			//Starts an ADC conversion
}


////////Functions 7 segments////////


void Dynamic_display(int Display_1, int Display_2, int temperature)
{
	int one=0, two=0;
	one=(temperature/10);						//Takes the temperature value and splits the digits
	two=(temperature%10);
	PORTC.OUTCLR=0x03;							//both digits off
	if(Display_1==1 && Display_2==0)			//Asks if the digit one is the one on 
	{
		PORTC.OUTSET= 0x01;						//First digit on
		Seven_segments(one);					//Calls the seven_segments function with the first digit
	}
	if(Display_1==0 && Display_2==1)			//Asks if the digit one is the one on
	{
		PORTC.OUTSET= 0x02;						//Second digit on 
		Seven_segments(two);					//Calls the seven_segments function with the second digit 
	}
}
void Seven_segments(int num)
{
	PORTA.OUTSET= 0xFE;							//All segments off 
	if(num==0)
	{PORTA.OUTCLR= 0x7E;}				
	if(num==1 || (num>9 && num<20))
	{PORTA.OUTCLR= 0x0C;}
	if(num==2 || (num>19 && num<30))
	{PORTA.OUTCLR= 0xB6;}
	if(num==3 || (num>29 && num<40))
	{PORTA.OUTCLR= 0x9E;}
	if(num==4 || (num>39 && num<50))
	{PORTA.OUTCLR= 0xCC;}
	if(num==5 || (num>49 && num<60))
	{PORTA.OUTCLR= 0xDA;}
	if(num==6 || (num>59 && num<70))
	{PORTA.OUTCLR= 0xFA;}
	if(num==7 || (num>69 && num<80))
	{PORTA.OUTCLR= 0x0E;}
	if(num==8 || (num>79 && num<90))
	{PORTA.OUTCLR= 0xFE;}
	if(num==9 || (num>89 && num<100))
	{PORTA.OUTCLR= 0xDE;}
}


