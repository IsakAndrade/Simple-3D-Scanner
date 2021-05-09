/* 
 * The main goal of this project is to create the basis for a simple 3D scanner using the Sharp Infrared Proximity Sensor. The code consist of 3
 * main parts:
 *		- Menu where the user can select what to do, using an I2C screen.
 *		- Commands controlling two DC-motors connected to an H-bridge.
  */

#define F_CPU 16000000L
#define TOTAL_ROWS 1
#define TOTAL_COLUMNS 2

// include libraries
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "lib/usart.h"
#include "lib/ssd1306.h"




#define MOTOR_1_PIN_1 PINB0
#define MOTOR_1_PIN_2 PINB1
#define MOTOR_1_REG DDRB
#define MOTOR_1_PORT PORTB

#define MOTOR_2_PIN_1 PINB0
#define MOTOR_2_PIN_2 PINB1
#define MOTOR_2_REG DDRB
#define MOTOR_2_PORT PORTB


#define RESOLUTION 99

void initMotors(void){
	MOTOR_1_REG |= (1<<MOTOR_1_PIN_1)|(1<<MOTOR_1_PIN_2);
	MOTOR_2_REG |= (1<<MOTOR_2_PIN_1)|(1<<MOTOR_2_PIN_2);
}


void controlMotor1(int select){
	switch (select)
	{
		case 0:
			// Foreward movement
			MOTOR_1_PORT &=~ (1<<MOTOR_1_PIN_1);
			MOTOR_1_PORT |= (1<<MOTOR_1_PIN_2);
			break;
		case 1:
			// Reverse movement
			MOTOR_1_PORT &=~ (1<<MOTOR_1_PIN_2);
			MOTOR_1_PORT |= (1<<MOTOR_1_PIN_1);
			break;
		default:
			// Stop motor
			MOTOR_1_PORT &=~ (1<<MOTOR_1_PIN_1);
			MOTOR_1_PORT &=~ (1<<MOTOR_1_PIN_2);
			break;
	}
}


void controlMotor2(int select){
	switch (select)
	{
		case 0:
			// Foreward movement
			MOTOR_2_PORT &=~ (1<<MOTOR_2_PIN_1);
			MOTOR_2_PORT |= (1<<MOTOR_2_PIN_2);
			break;
		case 1:
			// Reverse movement
			MOTOR_2_PORT &=~ (1<<MOTOR_2_PIN_2);
			MOTOR_2_PORT |= (1<<MOTOR_2_PIN_1);
			break;
		default:
			// Stop motor
			MOTOR_2_PORT &=~ (1<<MOTOR_2_PIN_1);
			MOTOR_2_PORT &=~ (1<<MOTOR_2_PIN_2);
			break;
	}	
}


// Function created to initiate desired timer values.
static inline void initTimer(void){
	TCCR1A |= (0<<COM1A1)|(0<<COM1A0); // Selecting "Normal" operation.
	TCCR1A |= (0<<WGM13)|(1<<WGM12)|(0<<WGM11)|(0<<WGM10); // CTC mode.
	TCCR1B |= (1<<CS12)|(0<<CS11)|(0<<CS10); // Prescaler of 256.
	TIMSK1 |=(1<<OCIE1A); // Enabling interrupts.
	
}

// Found on p.116 in datasheet :)
void TIM16_WriteTCNT1( unsigned int i ){
	unsigned char sreg;
	/* Save global interrupt flag */
	sreg = SREG;
	/* Disable interrupts */
	cli();
	/* Set TCNT1 to i */
	TCNT1 = i;
	/* Restore global interrupt flag */
	SREG = sreg;
	sei();
}





// ------ FUNCTIONS --------

// Initiates the Analog to Digital Converter on ADC0. NOTE THAT THIS INCLUDES INTERRUPTS!
static inline void initADC0(void) {
	ADMUX |= (0 << REFS0)|(0 << REFS1);	// Reference voltage at AREF
	ADCSRA |= (1<<ADEN);	// Enabling the ADC.
	ADCSRA |= (1<<ADIE);	// Enabling Interrupts for ADC-conversions.
	ADCSRA |=(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);	// Scaling the CPU clock appropriately for the ADC.
	OCR1A = (10000*100)/RESOLUTION; // Setting wait time for adc 
}

// A function that reads and returns 10 bit number from the ADC.
int read_10b_adc(void){
	uint16_t adcl = ADCL;
	uint16_t adch = (ADCH<<8);
	uint16_t adc_result = adch + adcl;
	return adc_result;
}

void initInterrupt0(void){
	EIMSK |= (1<<INT0);	// Enable INT0
	EICRA |= (1<<ISC01)|(1<<ISC00);	// Rising edge on INT0 generates interrupt.
}

// Setting our global values to volatile so that they don't get optimzed to oblivion.
volatile uint16_t adc_result;

ISR(ADC_vect){
	// Printing the resulting ADC.
	// This starts a conversion should be used in the main code as well.
	adc_result = read_10b_adc();
	
}

volatile uint8_t timer_event;
// Using the interrupt for "Compare Match" events.
ISR(TIMER1_COMPA_vect){
	timer_event = 1;
}


// The start process value is used to control the different functions, and cancel them if needed
volatile uint8_t start_process;
uint16_t row;
uint16_t columns;
ISR(INT0_vect){
	start_process = !start_process;
	
}





int main(void)
{
	// init ssd1306
	initUSART();
	initADC0();
	initTimer();
	initInterrupt0();
	sei();
	
	SSD1306_Init();

	initMotors();
	
	// Initiating some important variables
	adc_result = 0;
	uint8_t meas_count = 0;
	uint16_t measurements[9];
	
	// These two values are used to determine 
	row = 0;
	columns = 0;
	
	printString("We are on line!");
	
	//-----INITIATING SCREEN---
	// Notice that this should be its own function.
	// Clear screen
	SSD1306_ClearScreen();
	// Draw line
	SSD1306_DrawLineHorizontal(4, 4, 115);
	// set position x, y
	SSD1306_SetPosition(5, 1);
	// Draw string
	SSD1306_DrawString("Main Menu");
	// Draw line
	SSD1306_DrawLineHorizontal(4, 18, 115);
	
	SSD1306_SetPosition(5, 3);
	SSD1306_DrawString("> Run 3D Scan");
	
	SSD1306_SetPosition(5, 4);
	SSD1306_DrawString("  Resolution");

	SSD1306_SetPosition(5, 5);
	SSD1306_DrawString("  Sleep");
	
	SSD1306_UpdateScreen();
	
	do 
	{
	  
	  switch(adc_result/341) {
		  case 0:
		  // Begin 3D-scan.
		  SSD1306_SetPosition(5, 3);
		  SSD1306_DrawString("> Run 3D Scan");
		  
		  SSD1306_SetPosition(5, 4);
		  SSD1306_DrawString("  Resolution");

		  SSD1306_SetPosition(5, 5);
		  SSD1306_DrawString("  Sleep");
		  
		  SSD1306_UpdateScreen();
		  
		  if(start_process){
			  //Reset
			  while((row > 0)&&start_process){
				  
				  controlMotor1(1);
				  
				  TIM16_WriteTCNT1(0);
				  while(!timer_event);
				  timer_event = 0;
				  
				  controlMotor1(2);
				  
				  TIM16_WriteTCNT1(0);
				  while(!timer_event);
				  timer_event = 0;
				  
				  printString("R");
				  printWord(row);
				  row--;
			  }
			  while((columns > 0)&&start_process){
				  
				  controlMotor2(1);
				  
				  TIM16_WriteTCNT1(0);
				  while(!timer_event);
				  timer_event = 0;
				  
				  controlMotor2(2);
				  
				  TIM16_WriteTCNT1(0);
				  while(!timer_event);
				  timer_event = 0;
				  
				  printString("C");
				  printWord(columns);
				  columns--;
			  }
			  
			  // Scanning
			  while(start_process&&(row<TOTAL_ROWS)) 
			  {

				  
				  
				  while(start_process&&(columns<((TOTAL_COLUMNS*RESOLUTION)/100))) 
				  {
					// Takes 10 measurements.
					for (uint8_t i = 0; i < 9; ++i)
					{
					  while(ADCSRA&(1<<ADSC)){}
					  measurements[i] = adc_result;
					  ADCSRA |= (1<<ADSC);
					}
					
					// Send the row of measurements.
					printString("R");
					printWord(row);
				  
				  
					// Send all data for that row.
					for (uint8_t i = 0; i < 9; ++i)
					{
						printString("C");
						printWord(measurements[i]);
					}
				  
					// Moving motor 1 step
					
					controlMotor1(0);
					
					TIM16_WriteTCNT1(0);
					while(!timer_event);
					timer_event = 0;
					
					controlMotor1(2);
					TIM16_WriteTCNT1(0);
					while(!timer_event);
					timer_event = 0;
					columns++;
				  
				  }
				  
				  
				  
				  controlMotor2(0);
				  TIM16_WriteTCNT1(0);
				  while(!timer_event);
				  timer_event = 0;
				  
				  controlMotor2(2);
				  TIM16_WriteTCNT1(0);
				  while(!timer_event);
				  timer_event = 0;
				  
				  
				  row++;
				  				  
			  }
			  
			  start_process = 0;
		  }
		  
		  ADCSRA |= (1<<ADSC);
		  break;
		  
		  case 1:
		  // Enter sleep mode.
		  // > 2. Adjust resolution
		  SSD1306_SetPosition(5, 3);
		  SSD1306_DrawString("  Run 3D Scan");
		  
		  SSD1306_SetPosition(5, 4);
		  SSD1306_DrawString("> Resolution");

		  SSD1306_SetPosition(5, 5);
		  SSD1306_DrawString("  Sleep");
		  
		  SSD1306_UpdateScreen();
		  
		  // Adjusting resolution TOTAL_COLUMNS*ADC_RESULT/1024
		  // Adding time to each section, so for loops needs to be added on waiting
		  while(start_process){
			SSD1306_SetPosition(5, 3);
			SSD1306_DrawString("  The current resolution is:");
			
			
			// Printing resolution!
			char str[] = {'0' + (RESOLUTION / 100), '0' + ((RESOLUTION / 10) % 10), '0' + (RESOLUTION % 10), '%'};			
			
			SSD1306_SetPosition(5, 4);
			SSD1306_DrawString(str);

			SSD1306_SetPosition(5, 5);
			SSD1306_DrawString("                          ");
			
			SSD1306_UpdateScreen();  
		  }
		  
		  ADCSRA |= (1<<ADSC);
		  break;
		  
		  
		  default:
		  // > 3. Go To Sleep
		  SSD1306_SetPosition(5, 3);
		  SSD1306_DrawString("  Run 3D Scan");
		  
		  SSD1306_SetPosition(5, 4);
		  SSD1306_DrawString("  Resolution");

		  SSD1306_SetPosition(5, 5);
		  SSD1306_DrawString("> Sleep");
		  
		  SSD1306_UpdateScreen();
		  
		  if(start_process){
		  
		  SSD1306_ClearScreen();
		  SSD1306_SetPosition(5, 4);
		  SSD1306_DrawString("Seep zzz");
		  
		  while(start_process){
			  ADCSRA |= (1<<ADEN);
		  }
		  
		  
		  }
		  
		  
		  ADCSRA |= (1<<ADSC);
		  
		  
	  }
	  
	  
  } while (1);
  return 0;
}