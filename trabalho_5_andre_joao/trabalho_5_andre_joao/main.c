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
const unsigned char primeira_letra[]  = {
   // -GFEDCBA
	0b01010000,//r
	0b01111101,//G
	0b01111100,//b
};
const unsigned char segunda_letra[]  = {
	//-GFEDCBA
	0b00111001, //E
	0b00101000, //r
	0b00011100, //L
};

unsigned char estado_passado_pd2, estado_passado_pd3, estado_passado_pd4;
unsigned char pd2_pressionado, pd3_pressionado, pd4_pressionado;
unsigned char flag_pd2 = 0, flag_pd3 = 0, flag_pd4 = 0;
int unidade = 0, dezena = 0, cor = -1;
unsigned char duty_r=0, duty_g = 0, duty_b = 0,pwm_r = 0,pwm_g = 0,pwm_b = 0;
void reinicia_timer1();
void debouncing_pd2();
void debouncing_pd3();
void debouncing_pd4();
void atualiza_display_numero();
void atualiza_display_letra();
void configura_PWM();

void reinicia_timer1(){
	// REINICIA TIMER1 SEMPRE QUE ATUALIZA DISPLAY
	TCCR1A = 0b00000010; // CTC Mode
	TCCR1B = 0b00000101; // PRESCALER = 1024
	OCR1AH = 0x07;
	OCR1AL = 0xA0; // 1953*(1us)*1024 = 1,999872s
	TIMSK1 = 0b00000010; // Habilita interrupção na comparação com OCR1A
}

void debouncing_pd2(){
	if(pd2_pressionado == 1){
		flag_pd2 = 1;
	}
	if((pd2_pressionado == 0) && (flag_pd2 == 1)){
		flag_pd2 = 0;
		cor++;
		if(cor > 2){
			cor = 0;
		}
		atualiza_display_letra();
	}
}

void debouncing_pd3(){
	if(pd3_pressionado == 1){
		flag_pd3 = 1;
	}
	if((pd3_pressionado == 0) && (flag_pd3 == 1)){
		flag_pd3 = 0;
		switch (cor){
			case 0:
				if(duty_r<=0){
					duty_r = 0;
				}
				else{
					duty_r--;
				}
				pwm_r = (duty_r/100)*255;
				OCR0A = pwm_r;
				break;
			case 1:
				if(duty_g<=0){
					duty_g = 0;
				}
				else{
					duty_g--;
				}
				pwm_g = (duty_g/100)*255;
				OCR0B = pwm_g;
				break;
			case 2:
				if(duty_b<=0){
					duty_b = 0;
				}
				else{
					duty_b--;
				}
				break;
		}
		if((dezena == 0) && (unidade == 0)){
			dezena = 0;
			unidade = 0;
		}else if((unidade == 0) && (dezena > 0)){
			dezena--;
			unidade = 9;
		}else{
			unidade--;
		}
	}
	atualiza_display_numero();
	
}

void debouncing_pd4(){
	if(pd4_pressionado == 1){
		flag_pd4 = 1;
	}
	if((pd4_pressionado == 0) && (flag_pd4 == 1)){
		flag_pd4 = 0;
		switch (cor){
			case 0:
				if(duty_r>=100){
					duty_r = 99;
				}
				else{
					duty_r++;
				}
				pwm_r = (duty_r/100)*255;
				OCR0A = pwm_r;
				break;
			case 1:
				if(duty_g>=100){
					duty_g = 99;
				}
				else{
					duty_g++;
				}
				pwm_g = (duty_g/100)*255;
				OCR0B = pwm_g;
				break;
			case 2:
				if(duty_b>=100){
					duty_b = 99;
				}
				else{
					duty_b++;
				}
				pwm_b = (duty_b/100)*255;
				break;
		}
		
		if(unidade == 9 && dezena < 10){
			dezena++;
			unidade = 0;
		}else{
			unidade++;
		}	
		
		if(dezena>=10){
			dezena = 9;
			unidade = 9;
		}
	}
	atualiza_display_numero();
}

void atualiza_display_numero(){
	// Atualiza o display das dezenas
	PORTD |= (decodificador_7seg[dezena] & 0b00000001) << PORTD0;
	PORTC = (decodificador_7seg[dezena] & 0b01111110) >> 1; ; // Atualiza os pinos PD0-PD6 com o dígito da dezena
	// Atualiza o display das unidades
	PORTD |= (decodificador_7seg[unidade % 10] & 0b00000001) << PORTD1; // Atualiza o segmento A (PD7) com o dígito da unidade
	PORTB = (decodificador_7seg[unidade % 10] & 0b01111110) >> 1; // Atualiza os pinos PB0-PB5 com os segmentos B-G da unidade
}
void atualiza_display_letra(){
	if(cor==1){
		PORTD |= (1 << PORTD0);
	}else{
		PORTD &= ~(1 << PORTD0);
	}
	PORTC = primeira_letra[cor];
	PORTB = segunda_letra[cor];
	reinicia_timer1();
}

void configura_PWM(){
	// Configuração do Timer2 para PWM nos pinos PC3, PC4 e PC5
	TCCR0A |= (1 << WGM00) | (1 << WGM01); // Modo de PWM rápido de 8 bits
	TCCR0A |= (1 << COM0A1) | (1 << COM0A0) | (1 << COM0B0) | (1 << COM0B1); // Clear OC0A/OC0B no comparativo, set OC0A/OC0B no fundo
	TCCR0B |= (1 << CS01); // Prescaler = 8
	OCR0A = pwm_r; // Duty cycle para o pino OC0A (PC3)
	OCR0B = pwm_g; // Duty cycle para o pino OC0B (PC4)
}
int main(void){
	DDRC = 0x3F; // Configura PC0-PC5 como saídas
	DDRB = 0x3F; // Configura PB0-PB5 como saídas
	DDRD = 0b11100011;
	PORTB = 0x00;
	PORTC = 0x00;
	PORTD =  0b00011100; // Configura PD2-PC4 com pull-up
	PCICR = (1 << PCIE2); // Habilita interrupção de pin change no PORTD
	PCMSK1 |= (1 << PCINT18) | (1 << PCINT19) | (1 << PCINT20); // Habilita interrupção nos pinos PD2-PC4
	configura_PWM();
	sei(); // Habilita interrupções globais
		
	while(1){
		if(TCNT0>=pwm_b){
			PORTC |= (1 << PORTD7);
		}
		else{
			PORTC &= ~(1 << PORTD7);
		}
	}
	return 0;
}
ISR(TIMER1_COMPA_vect){
	atualiza_display_numero();
}

ISR(PCINT1_vect){
	pd2_pressionado = !(PIND & (1 << PIND2));
	pd3_pressionado = !(PIND & (1 << PIND3));
	pd4_pressionado = !(PIND & (1 << PIND4));
	debouncing_pd2();	
	debouncing_pd3();
	debouncing_pd4();
}
ISR(TIMER0_COMPA_vect){
	PORTD ^= (1 << PORTD5);
}
ISR(TIMER0_COMPB_vect){
	PORTC ^= (1 << PORTC6);
}