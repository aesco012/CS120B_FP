/*
 * FinalProj_ReactionGame.c
 * Author : Alex Escobar
 *This is for my CS120B final Project due on June 9, 2019
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdio.h>
#include <stdlib.h>
#define F_CPU 8000000UL
#include "nokia5110.c"


volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

//Initializes the ADC and you must have the AREF pin connected to 5V
void IntializeADC()
{
	ADMUX |=(1<<REFS0);
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
}


//This is used in the Seven Segment display
unsigned char tmpD = 0x00;

//This is used in the game State machine to display the color associated with a button
unsigned char tmpC = 0x00;

//This is the count for my FSM
unsigned char count = 0x00;

//Random State holder
unsigned char RAN_STATE = 0x00;
unsigned char i = 0x00;

//These are used to determine if a sideways flick or vertical flick were made
unsigned char FLU = 0x00;
unsigned char FLD = 0x00;

//These are used to read the ADC values in the Joy Stick code
unsigned short Xaxis;
unsigned short Yaxis;


//Reads in the input from the Joystick and determines if there was a Vertical or Horizontal Movement
//Then assigns that to a variable
/////
void InputJoyStick()
{
	unsigned short XTopBound = 580;
	unsigned short XLowBound = 440;
	unsigned short YTopBound = 560;
	unsigned short YLowBound = 420;

	ADMUX = 0x40;
	if(ADMUX == 0x40)
	{
		Yaxis = ADC;
		_delay_ms(2);
		ADMUX = 0x41;
	}
	if(ADMUX == 0x41)
	{
		Xaxis = ADC;
	}

	if(Yaxis > YTopBound || Yaxis < YLowBound)
	{
		FLU = 1;
	}
	else if(Xaxis < XTopBound || Xaxis > XLowBound)
	{
		FLD = 1;
	}
	
	
}

//Shift Register Code made from the given lab template
void ShiftData(unsigned char data)
{
	for(unsigned int iter = 0; iter < 8; iter++)
	{
		PORTD = 0x08;
		PORTD |= ((data << iter) & 0x01);
		PORTD |= 0x04;
	}
	PORTD |= 0x02;
	PORTD = 0x08;
}


enum PROJ {INITIAL, Start, YEL, BLU, RED, FlickU, FlickD, LOSE, WIN} PROJ;
enum Num_States {Init, ONE, TWO, THREE, FOUR, FIVE, NULLL, WINN} Num_State;


void FSM()
{
	//States Generated Here
	RAN_STATE = rand() % 5 + 1;
	//Buttons all used on PORT A
	unsigned char YELL_BUTTON = ~PINA & 0x10;
	unsigned char BLU_BUTTON = ~PINA & 0x20;
	unsigned char RED_BUTTON = ~PINA & 0x40;
	unsigned char START_BUTTON = ~PINA & 0x08;
	unsigned char RESTART = ~PINA & 0x04;
	
	InputJoyStick();
	switch(PROJ)
	{
		case INITIAL:
		{
			PROJ = Start;
			break;
		}
		
		case Start:
		{
			PROJ = Start;
			Num_State = NULLL;
			score = 0x00;
			count = 25;
			FLD = 0;
			FLU = 0;
			if(START_BUTTON)
			{
				
				if(RAN_STATE == 1)
				{
					PROJ = YEL;
				}
				else if (RAN_STATE == 2)
				{
					PROJ = BLU;
				}
				else if (RAN_STATE == 3)
				{
					PROJ = RED;
				}
				else if (RAN_STATE == 4)
				{
					PROJ = FlickU;
				}
				else if (RAN_STATE == 5)
				{
					PROJ = FlickD;
				}
				
				Num_State = FIVE;
				nokia_lcd_clear();
			}
			break;
		}
		
		case YEL:
		{
			PROJ = YEL;
			
			if (RESTART)
			{
				PROJ = Start;
				Num_State = NULLL;
				nokia_lcd_clear();
				break;
			}
			
			if (RED_BUTTON)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (BLU_BUTTON)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (FLU)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (FLD)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (count == 0)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if(YELL_BUTTON)
			{
				
				if(RAN_STATE == 1)
				{
					PROJ = YEL;
				}
				else if (RAN_STATE == 2)
				{
					PROJ = BLU;
				}
				else if (RAN_STATE == 3)
				{
					PROJ = RED;
				}
				else if (RAN_STATE == 4)
				{
					PROJ = FlickU;
				}
				else if (RAN_STATE == 5)
				{
					PROJ = FlickD;
				}
				
				FLD = 0;
				FLU = 0;
				i = 0;
				count = 25;
				score++;
				if(score >= 10)
				{
					PROJ = WIN;
					Num_State = NULLL;
					nokia_lcd_clear();
					break;
				}
				Num_State = FIVE;
				nokia_lcd_clear();
				break;
			}
			count--;
			break;
		}
		
		case BLU:
		{
			PROJ = BLU;
			
			if (RESTART)
			{
				PROJ = Start;
				Num_State = NULLL;
				nokia_lcd_clear();
				break;
			}
			
			if (YELL_BUTTON)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (RED_BUTTON)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (FLU)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (FLD)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (count == 0)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if(BLU_BUTTON)
			{
				
				
				if(RAN_STATE == 1)
				{
					PROJ = YEL;
				}
				else if (RAN_STATE == 2)
				{
					PROJ = BLU;
				}
				else if (RAN_STATE == 3)
				{
					PROJ = RED;
				}
				else if (RAN_STATE == 4)
				{
					PROJ = FlickU;
				}
				else if (RAN_STATE == 5)
				{
					PROJ = FlickD;
				}
				
				FLD = 0;
				FLU = 0;
				i = 0;
				count = 25;
				score++;
				if(score >= 10)
				{
					PROJ = WIN;
					Num_State = NULLL;
					nokia_lcd_clear();
					break;
				}
				Num_State = FIVE;
				nokia_lcd_clear();
				break;
			}
			count--;
			break;
		}
		
		case RED:
		{
			PROJ = RED;
			
			if (RESTART)
			{
				PROJ = Start;
				Num_State = NULLL;
				nokia_lcd_clear();
				break;
			}
			
			if (YELL_BUTTON)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (BLU_BUTTON)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (FLU)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (FLD)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (count == 0)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if(RED_BUTTON)
			{
				
				
				if(RAN_STATE == 1)
				{
					PROJ = YEL;
				}
				else if (RAN_STATE == 2)
				{
					PROJ = BLU;
				}
				else if (RAN_STATE == 3)
				{
					PROJ = RED;
				}
				else if (RAN_STATE == 4)
				{
					PROJ = FlickU;
				}
				else if (RAN_STATE == 5)
				{
					PROJ = FlickD;
				}
				
				FLD = 0;
				FLU = 0;
				i = 0;
				count = 25;
				score++;
				if(score >= 10)
				{
					PROJ = WIN;
					Num_State = NULLL;
					nokia_lcd_clear();
					break;
				}
				Num_State = FIVE;
				nokia_lcd_clear();
				break;
			}
			count--;
			break;
		}
		
		case FlickU:
		{
			PROJ = FlickU;
			
			if (RESTART)
			{
				PROJ = Start;
				Num_State = NULLL;
				nokia_lcd_clear();
				break;
			}
			
			if(YELL_BUTTON)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			if (RED_BUTTON)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (BLU_BUTTON)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (FLD)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (count == 0)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if(FLU)
			{
				if(RAN_STATE == 1)
				{
					PROJ = YEL;
				}
				else if (RAN_STATE == 2)
				{
					PROJ = BLU;
				}
				else if (RAN_STATE == 3)
				{
					PROJ = RED;
				}
				else if (RAN_STATE == 4)
				{
					PROJ = FlickU;
				}
				else if (RAN_STATE == 5)
				{
					PROJ = FlickD;
				}
				
				FLD = 0;
				FLU = 0;
				i = 0;
				count = 25;
				score++;
				if(score >= 10)
				{
					PROJ = WIN;
					Num_State = NULLL;
					nokia_lcd_clear();
					break;
				}
				Num_State = FIVE;
				nokia_lcd_clear();
				break;
			}
			
			count--;
			break;
		}
		
		case FlickD:
		{
			PROJ = FlickD;
			
			
			if (RESTART)
			{
				PROJ = Start;
				Num_State = NULLL;
				nokia_lcd_clear();
				break;
			}
			
			if (RED_BUTTON)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (YELL_BUTTON)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (BLU_BUTTON)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (count == 0)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if (FLU)
			{
				PROJ = LOSE;
				nokia_lcd_clear();
				break;
			}
			
			if(FLD)
			{
				
				if(RAN_STATE == 1)
				{
					PROJ = YEL;
				}
				else if (RAN_STATE == 2)
				{
					PROJ = BLU;
				}
				else if (RAN_STATE == 3)
				{
					PROJ = RED;
				}
				else if (RAN_STATE == 4)
				{
					PROJ = FlickU;
				}
				else if (RAN_STATE == 5)
				{
					PROJ = FlickD;
				}
				
				FLD = 0;
				FLU = 0;
				i = 0;
				count = 25;
				score++;
				if(score >= 10)
				{
					PROJ = WIN;
					Num_State = NULLL;
					nokia_lcd_clear();
					break;
				}
				Num_State = FIVE;
				nokia_lcd_clear();
				break;
			}
			count--;
			break;
		}
		
		case WIN:
		{
			PROJ = WIN;
			Num_State = NULLL;
			if (RESTART)
			{
				PROJ = Start;
				nokia_lcd_clear();
			}
			break;
		}
		
		case LOSE:
		{
			PROJ = LOSE;
			Num_State = NULLL;
			if (RESTART)
			{
				PROJ = Start;
				nokia_lcd_clear();
			}
			break;
		}
		
		default:
		{
			PROJ = Init;
			break;
		}
	}
	
	
	switch(PROJ)
	{
		case INITIAL:
		{
			break;
		}
		
		case Start:
		{
			startup();
			tmpC = 0x00;
			break;
		}
		
		case YEL:
		{
			AMAR();
			tmpC = 0x01;
			break;
		}
		
		case BLU:
		{
			AZUL();
			tmpC = 0x02;
			break;
		}
		
		case RED:
		{
			ROJO();
			tmpC = 0x04;
			break;
		}
		
		case FlickU:
		{
			FL_UP();
			tmpC = 0x00;
			break;
		}
		
		case FlickD:
		{
			FL_DN();
			tmpC = 0x00;
			break;
		}
		
		case WIN:
		{
			WINNER();
			tmpC = 0x00;
			break;
		}
		
		case LOSE:
		{
			LOSER();
			tmpC = 0x00;
			break;
		}
		
		default:
		{
			PROJ = INITIAL;
			break;
		}
	}
	PORTC = tmpC;
}


//This determines what is displayed on the seven segment display
void SECS()
{
	switch(Num_State)
	{
		case Init:
		{
			Num_State = NULLL;
			break;
		}
		
		case ONE:
		{
			if (i > 5)
			{
				Num_State = NULLL;
				i = 0;
				break;
			}
			i++;
			break;
		}
		
		case TWO:
		{
			if (i > 5)
			{
				Num_State = ONE;
				i = 0;
				break;
			}
			i++;
			break;
		}
		
		case THREE:
		{
			if (i > 5)
			{
				Num_State = TWO;
				i = 0;
				break;
			}
			i++;
			break;
		}
		
		case FOUR:
		{
			if (i > 5)
			{
				Num_State = THREE;
				i = 0;
				break;
			}
			i++;
			break;
		}
		
		case FIVE:
		{
			if (i > 6)
			{
				Num_State = FOUR;
				i = 0;
				break;
			}
			i++;
			break;
		}
		
		
		case NULLL:
		{
			
			Num_State = NULLL;
			break;
		}
		
		case WINN:
		{
			Num_State = WINN;
			break;
		}
		
		
		default:
		{
			Num_State = Init;
			break;
		}
	}
	
	//For the Seven Segment display you must assign every off value as a 1
	// And every on value as a 0
	switch(Num_State)
	{
		case Init:
		{
			break;
		}
		
		case ONE:
		{
			tmpD = 0xF9;
			ShiftData(tmpD);
			break;
		}
		
		case TWO:
		{
			tmpD = 0xA4;
			ShiftData(tmpD);
			break;
		}
		
		case THREE:
		{
			tmpD = 0xB0;
			ShiftData(tmpD);
			break;
		}
		
		case FOUR:
		{
			tmpD = 0x99;
			ShiftData(tmpD);
			break;
		}
		
		case FIVE:
		{
			tmpD = 0x92;
			ShiftData(tmpD);
			break;
		}
		
		case NULLL:
		{
			tmpD = 0xFF;
			ShiftData(tmpD);
			break;
		}
		
		case WINN:
		{
			tmpD = 0xB0;
			ShiftData(tmpD);
			break;
		}
		
		default:
		{
			break;
		}
	}
	
}






int main(void)
{
	// Port A is used for every input from the game
	DDRA = 0x00; PORTA = 0xFF;
	
	//Port B is used for the Nokia screen
	DDRB = 0xFF; PORTB = 0x00;
	
	//Port D is connected to the shift register
	//Which then connects to the 7 segment display
	DDRD = 0xFF; PORTD = 0x00;
	
	//Displays a color associated with a button for you to press
	//to continue the game
	DDRC = 0xFF; PORTC = 0x00;
	
	
	IntializeADC();
	InputJoyStick();
	nokia_lcd_init();
	nokia_lcd_clear();
	
	TimerSet(200);
	TimerOn();
	
	while (1)
	{
		FSM();
		SECS();
		while (!TimerFlag){}
		TimerFlag = 0;
		
	}
}



