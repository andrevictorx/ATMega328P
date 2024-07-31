/*
 * main.c
 *
 * Created: 5/3/2024 10:34:10 AM
 *  Author: User
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000UL

unsigned char estado_atual_pc0, estado_atual_pc1, estado_passado_pc0, estado_passado_pc1;
unsigned char pc0_pressionado, pc1_pressionado;
unsigned char pausado = 1, zerado = 1, flag_pc0 = 0, flag_pc1 = 0;
unsigned long tempo_debounce = 1000, timer_counter = 0;

void inicia_timer0(){
	// TIMER0
	TCCR0A = 0b00000010; // CTC Mode
	TCCR0B = 0b00000010; // PRESCALER = 8
	OCR0A = 124; // 125*(1us)*8 = 1ms
	TIMSK0 = 0b00000010; // Habilita interrupção na comparação com OCR0A
}
void debouncing_pc0(){
	estado_atual_pc0 = (PINC & (1 << PINC0));
	if((estado_atual_pc0 == estado_passado_pc0) && (pc0_pressionado ==1)){
		flag_pc0 = 1;
	}
	if((pc0_pressionado ==0) && (flag_pc0 == 1) && (pausado == 1)){
		pausado = 0;
		flag_pc0 = 0;
	}
	else if ((pc0_pressionado ==0) && (flag_pc0 == 1) && (pausado == 0)){
		pausado = 1;
		flag_pc0 = 0;
	}
}
void debouncing_pc1(){
	estado_atual_pc1 = (PINC & (1 << PINC1));
	if((estado_atual_pc1 == estado_passado_pc1) && (pc1_pressionado ==1)){
		flag_pc1 = 1;
	}
	if((pc1_pressionado ==0) && (flag_pc1 == 1) && (zerado == 1)){
		zerado = 0;
		flag_pc1 = 0;
	}
	else if ((pc1_pressionado ==0) && (flag_pc1 == 1) && (zerado == 0)){
		zerado = 1;
		flag_pc1 = 0;
	}
}

int main(void) {
	DDRD = 0xFF; // Configura PORTD como saída
	PORTD = 0x00; // Inicia PORTD com valor baixo
	DDRC = 0x00; // Configura PORTC como entrada
	PORTC = 0x03; // Habilita resistores de pull-up nos pinos PC0 e PC1
	DDRB = 0xFF;
	PCICR = (1 << PCIE1); // Habilita interrupção de pin change no PORTC
	PCMSK1 = (1 << PCINT8) | (1 << PCINT9); // Habilita interrupção nos pinos PC0 e PC1 (PCINT8 e PCINT9)
	sei(); // Habilita interrupções globais
	
	while(1) {
		if (zerado == 0 || pausado == 0) {
			PORTD = 0xFF; // Liga LED
		}
		else if (zerado == 1 || pausado == 1){
			PORTD = 0x00;
		}
		
	}
}

ISR(TIMER0_COMPA_vect) {
	if (timer_counter < tempo_debounce) {
		timer_counter++;
	}
	else{
		timer_counter = 0;
		debouncing_pc0();
		debouncing_pc1();
	}
}

ISR(PCINT1_vect) {
	estado_passado_pc0 = (PINC & (1 << PINC0));
	pc0_pressionado = !(estado_passado_pc0);
	estado_passado_pc1 = (PINC & (1 << PINC1));
	pc1_pressionado = !(estado_passado_pc1);
	inicia_timer0();
}
