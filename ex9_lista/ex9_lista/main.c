/*
 * main.c
 *
 * Created: 4/23/2024 7:04:59 PM
 *  Author: User
 */ 

#include <xc.h>
#include <avr/interrupt.h>

void setup(){
	
}

int main(void)
{
	
    while(1)
    {
        //TODO:: Please write your application code 
		//Periodo_interrupção = 100ms
		//Periodo_clock = 1us
		//OCR0A = (Periodo_interrupção/Periodo_clock)*(1/prescaler) - 1
		//OCR0A <= (2^8)-1 = 255
		//OCR0A = (10^6/PRESCALER) - 1= 100ms
		OCR0A = 97;
		//PRESCALER = 1024
		TCCR0B = (1 << TCCR0B);
		//TCCR0B = 0b00000101; // --> PRESCALER = 1024
		//TCCR0A = 0b00000010; (CTC MODE) (pagina 86);
		TCCR0A = (1 << WGM01);
		//TIMSK0 = 0b00000010 // --> Output Compare Match A Interrupt Enable
		TIMSK0 = (1 << OCIE0A);
		//SEI() --> habilita rotina de interrupção
		//Status Register: Global Interrupt Enable = 1
		DDRD = 0xFF;
		for (int i=0; i<100;i++){
			PORTD = (1 << PORTD);
		}
		sei();
		}
}
ISR(TIMER0_COMPA_vect){
	asm("nop");
}