/*
 * main.c
 *
 * Created: 7/5/2024 11:24:30 AM
 *  Author: João Vitor
 */ 

#include <xc.h>
#include <avr/interrupt.h>
#include <string.h>

uint16_t adc_value;
int tensao, tensao_rosa;

const char *mensagem[] = {"Cor selecionada: Vermelho\n\r\0",/*0*/
	"Cor selecionada: Verde\n\r\0",/*1*/
	"Cor selecionada: Azul\n\r\0",/*2*/
	"Cor selecionada: Branco\n\r\0",/*3*/
	"Cor selecionada: Rosa\n\r\0",/*4*/
	"PWM ligado!\n\r0 - Vermelho\n\r1 - Verde\n\r2 - Azul\n\r3 - Branco\n\r4 - Rosa\n\rSelecione uma cor: \n\r\0",/*5*/
	"PWM desligado!\n\r\0",/*6*/
	"Caracter invalido!\n\r\0",/*7*/
	"PWM R=XXX PWM G=YYY  PWM B=ZZZ \n\r\0"/*8*/
};
const char *resposta;
char frameIN[12]={"0000000000"};
char flagRCV = 0;
unsigned char duty[5][3] = {{255,000,000}, {000,255,000}, {000,000,255}, {255,255,255}, {000,000,000}}; // Array contendo os duty cycles para R, G, B
int cor = 0, frameINPtr = 0, dutyPtr = 0;
unsigned char duty_cycle_blue = 0;
unsigned char ligado = 0, liga_desliga_pressionado, flag_liga_desliga;
static char buffer[4]; // Buffer para converter os valores de duty cycle para string
static const char *resposta_original = 0;
static char mensagem_final[50]; // Buffer para a mensagem final

uint16_t adc_value;
int tensao, tensao_rosa;

uint16_t read_adc(uint8_t adc_channel) {
	// Seleciona o canal ADC
	ADMUX = (1 << REFS0) | adc_channel;
	// Inicia a conversão
	ADCSRA |= (1 << ADSC);
	// Aguarda a conversão ser concluída
	while (ADCSRA & (1 << ADSC));
	// Retorna o valor do ADC
	return ADC;
}

void inicia_ios(void) {
	// Configurar pinos como sa?da
	DDRB = 0x3F; // PB0 - PB5 como sa?da
	DDRD = 0xE3; // PD0, PD1, PD5, PD6, PD7 como sa?da

	// Configurar pinos como entrada com pull-up habilitado
	DDRD &= ~(1 << PORTD2);
	PORTD |= (1 <<PORTD2);

	// Configurar interrup??es de mudan?a de pino (Pin Change Interrupt)
	PCICR |= (1 << PCIE2); // Habilitar interrup??es de mudan?a de pino para PCINT[23:16]
	PCMSK2 |= (1 << PCINT18) | (1 << PCINT19) | (1 << PCINT20); // Habilitar interrup??es para PD2, PD3, PD4
	
	ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); 
	
	UCSR0A = 0;
	UCSR0B = 1 << RXCIE0 | 1 << TXCIE0 | 1 << RXEN0 | 1 << TXEN0;
	UCSR0C = 1 << UCSZ01 | 1 << UCSZ00;
	UBRR0H = 0;
	UBRR0L = 51;
	
	OCR0B = 0;
	OCR0A = 0;
	duty_cycle_blue =0;
	sei();
}
void inicia_timer0(void) {
	// Configurar Timer 0 para modo normal com prescaler de 8
	TCCR0B |= (1 << CS01); // Prescaler de 8
	TIMSK0 |= (1 << TOIE0); // Habilitar interrup??o de overflow

	// Configurar OC0A (PD6) e OC0B (PD5) para PWM
	TCCR0A |= (1 << COM0A1) | (1 << COM0B1) | (1 << WGM00); // Fast PWM
}

void AtualizaDuty(void) {
	OCR0B = duty[cor][0]; // Vermelho
	OCR0A = duty[cor][1]; // Verde
	duty_cycle_blue = duty[cor][2];
}

void converte_duty_para_string(unsigned char duty, char* buffer) {
	buffer[0] = (duty / 100) + '0';          // Centenas
	buffer[1] = ((duty % 100) / 10) + '0';   // Dezenas
	buffer[2] = (duty % 10) + '0';           // Unidades
	buffer[3] = '\0';                        // Terminador nulo
}



void le_serial() {
	if (flagRCV == 1 && ligado == 1) {
		unsigned char erro = 0;
		flagRCV = 0;
		switch (frameINPtr) {
			case 0:
			switch (frameIN[0]) {
				case '0':
					resposta = mensagem[0];
					resposta_original = resposta; // Inicializa resposta_original
					UDR0 = (char) *(resposta++);
					frameINPtr = 1;
					cor = 0;
					break;
				case '1':
					resposta = mensagem[1];
					resposta_original = resposta; // Inicializa resposta_original
					UDR0 = (char) *(resposta++);
					frameINPtr = 1;
					cor = 1;
					break;
				case '2':
					resposta = mensagem[2];
					resposta_original = resposta; // Inicializa resposta_original
					UDR0 = (char) *(resposta++);
					frameINPtr = 1;
					cor = 2;
					break;
				case '3':
					resposta = mensagem[3];
					resposta_original = resposta; // Inicializa resposta_original
					UDR0 = (char) *(resposta++);
					frameINPtr = 1;
					cor = 3;
					break;
				case '4':
					resposta = mensagem[4];
					resposta_original = resposta; // Inicializa resposta_original
					UDR0 = (char) *(resposta++);
					frameINPtr = 1;
					cor = 4;
					break;
				default:
				erro = 1;
					frameINPtr = 1;
					break;
			}
			break;
			case 1:
				if ((frameIN[frameINPtr] == '\r') || (frameIN[frameINPtr] == '\n')) {
					AtualizaDuty();
					converte_duty_para_string(duty[cor][0], buffer);
					strcpy(mensagem_final, "PWM R=");
					strcat(mensagem_final, buffer);
					converte_duty_para_string(duty[cor][1], buffer);
					strcat(mensagem_final, " PWM G=");
					strcat(mensagem_final, buffer);
					converte_duty_para_string(duty[cor][2], buffer);
					strcat(mensagem_final, " PWM B=");
					strcat(mensagem_final, buffer);
					strcat(mensagem_final, "\n\r");

					resposta = mensagem_final;
					UDR0 = (char) *(resposta++);
					frameINPtr = 0;
					} else {
					frameINPtr = 0;
				}
				break;
			default:
				frameINPtr = 0;
				break;
		}
		if (erro == 1) {
			frameINPtr = 0;
			resposta = mensagem[7];
			resposta_original = resposta; // Inicializa resposta_original
			UDR0 = (char) *(resposta++);
		}
	}
}

int main(void) {

	inicia_ios();
	inicia_timer0();
	while(1) {
		// Lê a tensão do pino PC0
		adc_value = read_adc(0); // Lê o valor do ADC no pino PC0
		float voltage = (adc_value * 5.0) / 1023.0;
			
		// Converte a tensão para um inteiro entre 0 e 255
		tensao = (int)(voltage * 255);
		tensao_rosa = (int)(voltage * 100);
		
		duty[0][0] = 255 - tensao;
		duty[0][1] = 0;
		duty[0][2] = 0;

		duty[1][0] = 0;
		duty[1][1] = 255 - tensao;
		duty[1][2] = 0;

		duty[2][0] = 0;
		duty[2][1] = 0;
		duty[2][2] = 255 - tensao;

		duty[3][0] = 255 - tensao;
		duty[3][1] = 255 - tensao;
		duty[3][2] = 255 - tensao;

		duty[4][0] = 255 - tensao;
		duty[4][1] = 0;
		duty[4][2] = 255 - tensao;
		if (ligado == 1){
			AtualizaDuty();	
		}
		
		le_serial();
		// PWM por software para o LED azul
		for (unsigned char i = 0; i < 100; i++) {
			if (i < duty_cycle_blue) {
				PORTD |= (1 << PORTD7);
				} else {
				PORTD &= ~(1 << PORTD7);
			}
		}
	}
}

ISR(USART_RX_vect) {
	frameIN[frameINPtr] = UDR0;
	flagRCV = 1;
}

ISR(USART_TX_vect) {
	if (*resposta != '\0') {
		UDR0 = *resposta;
		resposta++;
	}
}

ISR(PCINT2_vect) {
	liga_desliga_pressionado = !(PIND & (1 << PIND2));
	if (liga_desliga_pressionado == 1) {
		flag_liga_desliga = 1;
	}
	if ((liga_desliga_pressionado == 0) && (flag_liga_desliga == 1) && (ligado == 1)) {
		ligado = 0;
		flag_liga_desliga = 0;
		resposta = mensagem[6];
		UDR0 = (char) *(resposta++);
		frameINPtr = 0;
		OCR0B = 0;
		OCR0A = 0;
		duty_cycle_blue =0;
		} else if ((liga_desliga_pressionado == 0) && (flag_liga_desliga == 1) && (ligado == 0)) {
		ligado = 1;
		flag_liga_desliga = 0;
		resposta = mensagem[5];
		UDR0 = (char) *(resposta++);
		frameINPtr = 0;
	}
}
ISR(TIMER0_OVF_vect){
	
}