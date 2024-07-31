/*
 * main.c
 *
 * Created: 5/24/2024 7:14:08 PM
 *  Author: André Victor Xavier Pires e João Vitor de Siqueira
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000UL

// Tabela de decodificação de 0 a 9 em display de 7 segmentos
const unsigned char decodificador_7seg[] = {
	0b00111111, // 0
	0b00000110, // 1
	0b01011011, // 2
	0b01001111, // 3
	0b01100110, // 4
	0b01101101, // 5
	0b01111101, // 6
	0b00000111, // 7
	0b01111111, // 8
	0b01101111  // 9
};
unsigned char estado_atual_pc0, estado_atual_pc1, estado_passado_pc0, estado_passado_pc1;
unsigned char pc0_pressionado, pc1_pressionado;
unsigned char pausado = 1, zerado = 0, flag_pc0 = 0, flag_pc1 = 0;
unsigned char tempo_debounce = 100, contador_timer0 = 0,contador_timer1 = 0;
unsigned char dezena = 0;
unsigned char unidade = 0;

void reinicia_timer1(){
	//REINICIA TIMER1 SEMPRE QUE ATUALIZA DISPLAY
	TCCR1A = 0b00000010; //CTC Mode
	TCCR1B = 0b00000101; //PRESCALER = 1024
	OCR1A = 0xFF; //97*(1us)*1024 = 99,328ms
	TIMSK1 = 0b00000010; //Habilita interrupção na comparação com OCR1A
}
void reinicia_timer0(){
	//TIMER0
	TCCR0A = 0b00000010; //CTC Mode
	TCCR0B = 0b00000001; //PRESCALER = 8
	OCR0A = 249; //250*(1us)*8 = 2ms
	TIMSK0 = 0b00000010; //Habilita interrupção na comparação com OCR0A
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
		zerado = 1;
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

void atualiza_display(){
	// Atualiza o display das dezenas
	PORTD = decodificador_7seg[dezena]; // Atualiza os pinos PD0-PD6 com o dígito da dezena
	// Atualiza o display das unidades
	PORTD |= (decodificador_7seg[unidade % 10] & 0b00000001) << PD7; // Atualiza o segmento A (PD7) com o dígito da unidade
	PORTB = (decodificador_7seg[unidade % 10] & 0b01111110) >> 1; // Atualiza os pinos PB0-PB5 com os segmentos B-G da unidade
	
	// Incrementa os valores de unidade e dezena
	if (pausado == 0) {
		unidade++; // Incrementa a unidade
		if (unidade == 10) // Se a unidade atingir 10 ou pc1 pressionado, reinicia em 0 e incrementa a dezena
		{
			unidade = 0;
			dezena++;
			if (dezena == 10) // Se a dezena atingir 10 ou pc1 pressionado, reinicia em 0
			{
				dezena = 0;
			}
		}
	}
		
}
ISR(TIMER1_COMPA_vect){
	contador_timer1 = contador_timer1 + 1;
	if (contador_timer1>10){
		contador_timer1=0;
		atualiza_display();
	}
}
ISR(PCINT1_vect) {
	estado_passado_pc0 = (PINC & (1 << PINC0));
	pc0_pressionado = !(estado_passado_pc0);
	estado_passado_pc1 = (PINC & (1 << PINC1));
	pc1_pressionado = !(estado_passado_pc1);
	reinicia_timer0();
}

ISR(TIMER0_COMPA_vect) {
	if (contador_timer0 < tempo_debounce) {
		contador_timer0++;
	}
	else{
		contador_timer0 = 0;
		debouncing_pc0();
		debouncing_pc1();
	}
}



int main(void)
{
	DDRD = 0xFF; // Configura PD0-PD7 como saídas
	DDRB = 0x3F; // Configura PB0-PB5 como saídas
	DDRC = 0x00; // Configura PC0:PC5 como entradas
	PORTC = 0x03; //Configura PC0:PC1 como pull-up
	PCICR = 0b00000010; // Habilita interrupção de pin change no PORTC
	PCMSK1 = 0b00000011;//Habilita interrupção nos pinos PC0 e PC1
	sei(); //Habilita interrupções globais
	
	while (1)
	{
		// Verifica se o contador do display não está pausado e se o contador do timer1 precisa ser reiniciado
		if ((pausado == 0) && (contador_timer1 == 0)) {
			reinicia_timer1();
		}
		
		if (zerado == 1){
			unidade = 0;
			dezena = 0;
			zerado = 0;
		}
	}
	return (0);
}
