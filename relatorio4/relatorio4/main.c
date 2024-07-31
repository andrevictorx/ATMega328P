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

void inicia_timer1(){
	//REINICIA TIMER1 SEMPRE QUE ATUALIZA DISPLAY
	TCCR1A = 0b00000010; //CTC Mode
	TCCR1B = 0b00000101; //PRESCALER = 1024
	OCR1A = 97; //97*(1us)*1024 = 99,328ms
	TIMSK1 = 0b00000010; //Habilita interrupção na comparação com OCR1A
}
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
	PORTD |= (decodificador_7seg[unidade] & 0b00000001) << PD7; // Atualiza o segmento A (PD7) com o dígito da unidade
	PORTB = (decodificador_7seg[unidade] & 0b01111110) >> 1; // Atualiza os pinos PB0-PB5 com os segmentos B-G da unidade
}
void incrementa_contador(){
	// Incrementa os valores de unidade e dezena
	unidade++; // Incrementa a unidade
	if (unidade == 10) // Se a unidade atingir 10 ou pc1 pressionado, reinicia em 0 e incrementa a dezena
	{
		unidade = 0;
		dezena++;
		if (dezena == 10) // Se a dezena atingir 10 ou pc1 pressionado, reinicia em 0
		{
			dezena = 0;
			unidade = 0;
		}
	}
}
ISR(TIMER1_COMPA_vect){
	contador_timer1++;
	if (contador_timer1>=30){
		contador_timer1=0;
	}
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
ISR(PCINT1_vect) {
	estado_passado_pc0 = (PINC & (1 << PINC0));
	pc0_pressionado = !(estado_passado_pc0);
	estado_passado_pc1 = (PINC & (1 << PINC1));
	pc1_pressionado = !(estado_passado_pc1);
	inicia_timer0();
}
void setup(){
	DDRD = 0xFF; // Configura PD0-PD6 como saídas
	DDRB = 0x3F; // Configura PB0-PB5 como saídas
	DDRC = 0x00; // Configura PC0:PC5 como entradas
	PORTC = 0x03; //Configura PC0:PC1 como pull-up
	PCICR = (1 << PCIE1); // Habilita interrupção de pin change no PORTC
	PCMSK1 = (1 << PCINT8) | (1 << PCINT9); // Habilita interrupção nos pinos PC0 e PC1 (PCINT8 e PCINT9)
	atualiza_display();
	inicia_timer1();
	sei(); //Habilita interrupções globais
}
int main(void)
{
	setup();
	while (1)
	{
		if (zerado == 1){
			unidade = 0;
			dezena = 0;
		}
		// Verifica se o contador do display não está pausado e se o contador do timer1 precisa ser reiniciado
		if ((pausado == 0) && (contador_timer1 == 0)) {
			atualiza_display();
			incrementa_contador();

		}
		
	}
	return (0);
}
