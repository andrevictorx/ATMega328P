//Alunos: Andre Victor Xavier Pires e João Vitor de Siqueira
#include <xc.h>
#include <avr/interrupt.h>
#include <string.h>
const char *mensagem[] = {"L","I","G","A","D","E","S","L","I","G","A","\rPWM ligado!\n\r\0",
	"\rPWM desligado!\n\r\0",
	"\rCaracter invalido!\n\r\0",
	"PWM X = YYY\n\r\0",
	"R","G","B","%"
};
const char corLetra[3] = {'R', 'G', 'B'};
const char *resposta;
char frameIN[10]={"0000000000"};
int frameINPtr = 0;
char flagRCV = 0;
unsigned char duty;
int cor = 0;
unsigned char duty_cycle_blue,dezena,unidade;
static char buffer[4]; // Buffer para converter os valores de duty cycle para string
static char mensagem_final[50]; // Buffer para a mensagem final
unsigned char contador_timer1 = 0, j=0;
unsigned char dezena_r,dezena_g,dezena_b;
unsigned char unidade_r,unidade_g,unidade_b;
const unsigned char decodificador_7seg[] = {
	//XXFEDCBA 
	0b00111111, // 0
	0b00000110, // 1
	0b00011011, // 2
	0b00001111, // 3
	0b00100110, // 4
	0b00101101, // 5
	0b00111101, // 6
	0b00000111, // 7
	0b00111111, // 8
	0b00101111  // 9
};
const unsigned char primeira_letra[]  = {
	//xxFEDCBA
	0b00010000,//r
	0b00111101,//G
	0b00111100,//b
};
const unsigned char segunda_letra[]  = {
	//--FEDCBA
	0b00111001, //E
	0b00010000, //r
	0b00111000, //L
};

void atualiza_display_numero(){
	switch (j){
		case 0:
			PORTC = decodificador_7seg[dezena_r]; // Atualiza os pinos PC0-PC5 com o dígito da dezena
			PORTB = decodificador_7seg[unidade_r]; // Atualiza os pinos PB0-PB5 com os segmentos A-F da unidade
			if(unidade_r == 0 || unidade_r == 1 || unidade_r == 7){
				PORTD &= ~(1 << PORTD3);
			}else{
				PORTD |= (1 << PORTD3);
			}
			if(dezena_r == 0 || dezena_r == 1 || dezena_r == 7){
				PORTD &= ~(1 << PORTD2);
				}else{
				PORTD |= (1<<PORTD2);
			}
			break;
		case 1:
			PORTC = decodificador_7seg[dezena_g]; // Atualiza os pinos PC0-PC5 com o dígito da dezena
			PORTB = decodificador_7seg[unidade_g]; // Atualiza os pinos PB0-PB5 com os segmentos A-F da unidade
			if(unidade_g == 0 || unidade_g == 1 || unidade_g == 7){
				PORTD &= ~(1 << PORTD3);
				}else{
				PORTD |= (1 << PORTD3);
			}
			if(dezena_g == 0 || dezena_g == 1 || dezena_g == 7){
				PORTD &= ~(1 << PORTD2);
				}else{
				PORTD |= (1<<PORTD2);
			}
			break;
		case 2:
			PORTC = decodificador_7seg[dezena_b]; // Atualiza os pinos PC0-PC5 com o dígito da dezena
			PORTB = decodificador_7seg[unidade_b]; // Atualiza os pinos PB0-PB5 com os segmentos A-F da unidade
			if(unidade_b == 0 || unidade_b == 1 || unidade_b == 7){
				PORTD &= ~(1 << PORTD3);
			}else{
				PORTD |= (1 << PORTD3);
			}
			if(dezena_b == 0 || dezena_b == 1 || dezena_b == 7){
				PORTD &= ~(1 << PORTD2);
			}else{
				PORTD |= (1<<PORTD2);
			}
			break;
	}

}

void atualiza_display_letra(){
	if(j==2){
		PORTD &= ~(1 << PORTD3);
	}else{
		PORTD |= (1 << PORTD3);
	}
	PORTC = primeira_letra[j];
	PORTB = segunda_letra[j];	
	PORTD |= (1 << PORTD2);
}
void inicia_ios(void) {
	// Configurar pinos como saída
	DDRB = 0xFF; // PB0 - PB5 como sa?da
	DDRD = 0xFF;
	DDRC = 0xFF;
	UCSR0A = 0;
	UCSR0B = 1 << RXCIE0 | 1 << TXCIE0 | 1 << RXEN0 | 1 << TXEN0;
	UCSR0C = 1 << UCSZ01 | 1 << UCSZ00;
	UBRR0H = 0;
	UBRR0L = 8;

	OCR0B = 0;
	OCR0A = 0;
	duty_cycle_blue =0;
	sei();
}
void inicia_timer0(void) {
	// Configurar Timer0 para Fast PWM, com comparação de saída nos pinos OC0A e OC0B
	TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM00) | (1 << WGM01);
	TCCR0B = (1 << CS02) | (1 << CS00); // Prescaler = 1024
}
void inicia_timer1(){
	// REINICIA TIMER1 SEMPRE QUE ATUALIZA DISPLAY
	TCCR1A |= (1 << WGM12); // CTC Mode
	TCCR1B |= (1 << CS11)  ; // PRESCALER = 8
	OCR1AH = 0x61;
	OCR1AL = 0xA8; // 25000*(1us)*8 = 0.2s
	TIMSK1 = 0b00000010; // Habilita interrupção na comparação com OCR1A
}
void converte_duty_para_string(unsigned char duty, char* buffer) {
	buffer[0] = (duty / 100) + '0';          // Centenas
	buffer[1] = ((duty % 100) / 10) + '0';   // Dezenas
	buffer[2] = (duty % 10) + '0';           // Unidades
	buffer[3] = '\0';                        // Terminador nulo
}
void AtualizaDuty(void)
{
	float porcentagem;
	switch(cor){
		case 0:
			porcentagem = ((dezena_r*10) + (unidade_r))/100.0;
			duty = porcentagem*255;
			OCR0B = duty; // Vermelho
			converte_duty_para_string(duty, buffer);
			strcpy(mensagem_final, "\rPWM R=");
			strcat(mensagem_final, buffer);
			strcat(mensagem_final, "\r");
			break;
		case 1:
			porcentagem = ((dezena_g*10) + (unidade_g))/100.0;
			duty = porcentagem*255;
			OCR0A = duty; // Verde
			converte_duty_para_string(duty, buffer);
			strcpy(mensagem_final, "\rPWM G=");
			strcat(mensagem_final, buffer);
			strcat(mensagem_final, "\r");
			break;
		case 2:
			porcentagem = ((dezena_b*10) + (unidade_b))/100.0;
			duty = porcentagem*255;
			duty_cycle_blue = duty;
			converte_duty_para_string(duty, buffer);
			strcpy(mensagem_final, "\rPWM B=");
			strcat(mensagem_final, buffer);
			strcat(mensagem_final, "\r");
			break;
		default:
			OCR0B = 0; // Vermelho
			OCR0A = 0; // Verde
			duty_cycle_blue = 0;
			break;
	}
}
int main(void)
{
	unsigned char erro;
	inicia_ios();
	inicia_timer0();
	inicia_timer1();
	while(1)
	{
		if(flagRCV == 1)
		{
			erro = 0;
			flagRCV = 0;
			switch(frameINPtr)
			{
				case 0:
					if((frameIN[frameINPtr]=='d')||(frameIN[frameINPtr]=='D')){
						resposta = mensagem[4];
						UDR0 = (char) *(resposta++);
						frameINPtr++;
						
					}else if((frameIN[frameINPtr]=='l')||(frameIN[frameINPtr]=='L')){
						resposta = mensagem[0];
						UDR0 = (char) *(resposta++);
						frameINPtr++;
					}else if(((frameIN[0]=='R')||(frameIN[0]=='r'))|| ((frameIN[0]=='G')||(frameIN[0]=='g'))||((frameIN[0]=='b')||(frameIN[0]=='B'))){
						if((frameIN[0] == 'r') || (frameIN[0] == 'R')){
							cor = 0;
							resposta = mensagem[15];
							UDR0 = (char) *(resposta++);
						}
						if((frameIN[0] == 'g') || (frameIN[0] == 'G')){
							cor = 1;
							resposta = mensagem[16];
							UDR0 = (char) *(resposta++);
						}
						if((frameIN[0] == 'b') || (frameIN[0] == 'B')){
							cor = 2;
							resposta = mensagem[17];
							UDR0 = (char) *(resposta++);
						}
						frameINPtr++;
					}else{
						erro = 1;
					}
					break;
				case 1:
					if(frameIN[frameINPtr] == '='){
						UDR0 = (char) (frameIN[frameINPtr]);
						frameINPtr++;
					}else if((frameIN[frameINPtr]=='e')||(frameIN[frameINPtr]=='E')){
						resposta = mensagem[5];
						UDR0 = (char) *(resposta++);
						frameINPtr++;
					
					}else if((frameIN[frameINPtr]=='i')||(frameIN[frameINPtr]=='I')){
						resposta = mensagem[1];
						UDR0 = (char) *(resposta++);
						frameINPtr++;
					}
					else{
						erro = 1;
					}
					
					break;
				case 2:
					if((frameIN[frameINPtr] >= '0') && (frameIN[frameINPtr] <= '9')){
						UDR0 = (char) (frameIN[frameINPtr]);
						if((frameIN[0] == 'r') || (frameIN[0] == 'R')) dezena_r = frameIN[frameINPtr]-48;
						if((frameIN[0] == 'g') || (frameIN[0] == 'G')) dezena_g = frameIN[frameINPtr]-48;
						if((frameIN[0] == 'b') || (frameIN[0] == 'B')) dezena_b = frameIN[frameINPtr]-48;
						frameINPtr++;
					}else if((frameIN[frameINPtr]=='S')||(frameIN[frameINPtr]=='s')){
						resposta = mensagem[6];
						UDR0 = (char) *(resposta++);
						frameINPtr++;
						
					}else if((frameIN[frameINPtr]=='G')||(frameIN[frameINPtr]=='g')){
						resposta = mensagem[2];
						UDR0 = (char) *(resposta++);
						frameINPtr++;
					}
					else{
						erro = 1;
					}
					break;
				case 3:
					if((frameIN[frameINPtr] >= '0') && (frameIN[frameINPtr] <= '9')){
						UDR0 = (char) (frameIN[frameINPtr]);
						if((frameIN[0] == 'r') || (frameIN[0] == 'R')) unidade_r = frameIN[frameINPtr]-48;
						if((frameIN[0] == 'g') || (frameIN[0] == 'G')) unidade_g = frameIN[frameINPtr]-48;
						if((frameIN[0] == 'b') || (frameIN[0] == 'B')) unidade_b = frameIN[frameINPtr]-48;
						frameINPtr++;
					}else if((frameIN[frameINPtr]=='L')||(frameIN[frameINPtr]=='l')){
						resposta = mensagem[7];
						UDR0 = (char) *(resposta++);
						frameINPtr++;
						
					}else if((frameIN[frameINPtr]=='A')||(frameIN[frameINPtr]=='a')){
						resposta = mensagem[3];
						UDR0 = (char) *(resposta++);
						frameINPtr++;
					}
					else{
						erro = 1;
					}
					break;
				case 4:
					if(frameIN[frameINPtr] == '%'){
						resposta = mensagem[18];
						UDR0 = (char) *(resposta++);
						frameINPtr++;
					}else if((frameIN[frameINPtr]=='I')||(frameIN[frameINPtr]=='i')){
						resposta = mensagem[8];
						UDR0 = (char) *(resposta++);
						frameINPtr++;
						
					}else if ((frameIN[frameINPtr] == '\r')||(frameIN[frameINPtr] == '\n')){
						if((frameIN[0]=='L')||(frameIN[0]=='l')){
							resposta = mensagem[11];
							UDR0 = (char) *(resposta++);
							frameINPtr=0;
							OCR0A = 5;
							OCR0B = 5;
							duty_cycle_blue = 5;
						}else{
							erro = 1;
						}
					}else{
						erro=1;
					}
					break;
				case 5:
					if((frameIN[frameINPtr] >= '0') && (frameIN[frameINPtr] <= '9')){
						frameINPtr++;
					}else if((frameIN[frameINPtr]=='G')||(frameIN[frameINPtr]=='g')){
						resposta = mensagem[9];
						UDR0 = (char) *(resposta++);
						frameINPtr++;
					}else if((frameIN[frameINPtr] >= '0') && (frameIN[frameINPtr] <= '9')){
						frameINPtr++;
					}else if ((frameIN[frameINPtr] == '\r')||(frameIN[frameINPtr] == '\n')){
						AtualizaDuty();
						resposta = mensagem_final;
						UDR0 = (char) *(resposta++);
						frameINPtr = 0;
					}else{
						erro = 1;
					}
					break;
				case 6:
					if((frameIN[frameINPtr]=='A')||(frameIN[frameINPtr]=='a')){
						resposta = mensagem[10];
						UDR0 = (char) *(resposta++);
						frameINPtr++;
					}else{
						erro = 1;
					}
					break;
				case 7:
					if ((frameIN[frameINPtr] == '\r')||(frameIN[frameINPtr] == '\n')){
						if((frameIN[0]=='D')||(frameIN[0]=='d')){
							resposta = mensagem[12];
							UDR0 = (char) *(resposta++);
							frameINPtr = 0;
							OCR0A = 0;
							OCR0B = 0;
							duty_cycle_blue =0;
						}else{
							erro = 1;
						}
					}
					break;
				default:
					frameINPtr = 0;
					break;
			}
			if(erro == 1)
			{
				frameINPtr = 0;
				resposta = mensagem[13];
				UDR0 = (char) *(resposta++);
			}
		}
		// PWM por software para o LED azul
		for (unsigned char i = 0; i < 255; i++) {
			if (i < duty_cycle_blue) {
				PORTD |= (1 << PORTD7);
			} else {
				PORTD &= ~(1 << PORTD7);
			}
		}
	}
}

ISR(USART_RX_vect)
{
	frameIN[frameINPtr] = UDR0;
	flagRCV = 1;
}

ISR(USART_TX_vect)
{
	if (*resposta != '\0') {
		UDR0 = *resposta;
		resposta++;
	}
}
ISR(TIMER1_COMPA_vect){
	if(contador_timer1 == 1){
		atualiza_display_numero();
		contador_timer1 = 0;
	}else if(contador_timer1 == 0){
		if (j<2){
			j++;
		}else{
			j=0;
		}
		atualiza_display_letra();
		contador_timer1 = 1;
	}
}