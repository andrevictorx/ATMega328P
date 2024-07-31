#include <avr/io.h>
#include <avr/interrupt.h>

// Definições de pinos
#define BUTTON_SELECT_COLOR PORTD2
#define BUTTON_INCREMENT PORTD3
#define BUTTON_DECREMENT PORTD4

#define RED_LED_PIN PORTD6
#define GREEN_LED_PIN PORTD5
#define BLUE_LED_PIN PORTD7

#define DISPLAY1_A PORTD0
#define DISPLAY1_B PORTC5
#define DISPLAY1_C PORTC4
#define DISPLAY1_D PORTC3
#define DISPLAY1_E PORTC2
#define DISPLAY1_F PORTC1
#define DISPLAY1_G PORTC0

#define DISPLAY2_A PORTD1
#define DISPLAY2_B PORTB5
#define DISPLAY2_C PORTB4
#define DISPLAY2_D PORTB3
#define DISPLAY2_E PORTB2
#define DISPLAY2_F PORTB1
#define DISPLAY2_G PORTB0

// Variáveis globais
const char *mensagem[] = {
	"Selecione a cor desejada:\n0 - Vermelho\n 1 - Verde\n2 - Azul\n3 - Branco\n4 - Rosa\n\r\0",    // Mensagem para PWM ligado
	"PWM desligado!\n\r\0", // Mensagem para PWM desligado
	"Caractere invalido!\n\r\0", // Mensagem de caractere inválido
	"Cor selecionada: X \n\r\0" //Mensagem solicitando entrada do usuário
	"PWM X = YYY\n\r\0"     // Mensagem de status do PWM
};
const char corLetra[3] = {'R', 'G', 'B'}; // Letras das cores para resposta
const char *resposta; // Ponteiro para mensagem de resposta
char frameIN[10] = "0000000000"; // Buffer para o quadro recebido
unsigned char duty[5][3] = {{255,0,0}, {0,255,0}, {0,0,255}, {255,255,255}, {255,192,203}}; // Array contendo os duty cycles para R, G, B
char *dutyPtr; // Ponteiro para a parte do duty cycle do quadro
char flagRCV = 0; // Bandeira indicando que um quadro foi recebido
unsigned char erro = 0;

unsigned char decrementa_pressionado, incrementa_pressionado,seleciona_cor_pressionado;
unsigned char flag_decrementa,flag_incrementa, flag_seleciona_cor;
unsigned char decrementa,incrementa,seleciona_cor;
unsigned char cor = 0;  // 0 = Red, 1 = Green, 2 = Blue
unsigned char duty_cycle_red = 0, duty_cycle_green = 0, duty_cycle_blue = 0;
unsigned char pwm_red = 0, pwm_green = 0, pwm_blue = 0;
float f_pwm_red = 0, f_pwm_green = 0, f_pwm_blue = 0;
unsigned char display_timer = 0;
unsigned char unidade_red = 0, unidade_green = 0, unidade_blue = 0, dezena_red = 0, dezena_green = 0, dezena_blue = 0;
const unsigned char decodificador_7seg[] = {
	//XXBCDEFG
	0b00111110, // 0
	0b00110000, // 1
	0b00101101, // 2
	0b00111001, // 3
	0b00110011, // 4
	0b00011011,// 5
	0b00011111, // 6
	0b00110000, // 7
	0b00111111, // 8
	0b00111011  // 9
};
uint16_t adc_value;
int tensao, unidade_tensao, decimal_tensao;
void setup(void) {
	// Configurar pinos como saída
	DDRB = 0x3F; // PB0 - PB5 como saída
	DDRC = 0b01111110; // PORTC0 - PC5 como saída
	DDRD = 0xE3; // PD0, PD1, PD5, PD6, PD7 como saída

	// Configurar pinos como entrada com pull-up habilitado
	DDRD &= ~((1 << BUTTON_SELECT_COLOR) | (1 << BUTTON_INCREMENT) | (1 << BUTTON_DECREMENT));
	PORTD |= (1 << BUTTON_SELECT_COLOR) | (1 << BUTTON_INCREMENT) | (1 << BUTTON_DECREMENT);

	// Configurar interrupções de mudança de pino (Pin Change Interrupt)
	PCICR |= (1 << PCIE2); // Habilitar interrupções de mudança de pino para PCINT[23:16]
	PCMSK2 |= (1 << PCINT18) | (1 << PCINT19) | (1 << PCINT20); // Habilitar interrupções para PD2, PD3, PD4
	
	UCSR0A = 0;  // Resetar o registrador de controle e status A do UART
	UCSR0B = 1 << RXCIE0 | 1 << TXCIE0 | 1 << RXEN0 | 1 << TXEN0; // Habilitar RX e TX, e suas interrupções
	UCSR0C = 1 << UCSZ01 | 1 << UCSZ00; // Configurar formato do quadro: 8 bits de dados, 1 bit de parada
	UBRR0H = 0;
	UBRR0L = 51; // Configurar taxa de transmissão para 9600 bps com clock de 16 MHz
	
	// Configuração do ADC
	ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Habilita o ADC e define o fator de divisão de clock para 128
	// Configurar Timer 0 para modo normal com prescaler de 8
	TCCR0B |= (1 << CS01); // Prescaler de 8
	TIMSK0 |= (1 << TOIE0); // Habilitar interrupção de overflow

	// Configurar OC0A (PD6) e OC0B (PD5) para PWM
	TCCR0A |= (1 << COM0A1) | (1 << COM0B1) | (1 << WGM00); // Fast PWM
	sei(); // Habilita interrupções globais
}

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
void inicia_timer1(void) {
	// Configurar Timer 1 para gerar temporização de 2s
	TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC mode
	OCR1A = 31249; //31250*(1/16Mhz)*1024=1,99s
	TIMSK1 |= (1 << OCIE1A); // Habilitar interrupção de comparação A
}

void display_color(unsigned char color) {
	switch (color) {
		case 0: // vermelho
		PORTC = 0b00000101; // r
		PORTB = 0b00001111; // E
		PORTD = (PORTD & ~(1 << DISPLAY1_A)) | (1 << DISPLAY2_A); // A1=0 A2=1
		break;
		case 1: // verde
		PORTC = 0b00011111; // G
		PORTB = 0b00000101; // r
		PORTD = (PORTD & ~(1 << DISPLAY2_A)) | (1 << DISPLAY1_A); // A1=1 A2=0
		break;
		case 2: // azul
		PORTC = 0b00011111; // b
		PORTB = 0b00001110; // L
		PORTD &= ~(1 << DISPLAY1_A | 1 << DISPLAY2_A); // A1=0 A2=0
		break;
		default:
		// Caso nenhuma cor seja definida, desativa ambos displays
		PORTD &= ~(1 << DISPLAY1_A | 1 << DISPLAY2_A); // A1=0 A2=0
		break;
	}
	inicia_timer1();
}

void set_pwm_duty_cycle() {
	OCR0A = duty[frameIN[0]][0];
	OCR0B = duty[frameIN[0]][1];
	duty_cycle_blue = duty[frameIN[0]][2];
	resposta = mensagem[3];
	UDR0 = (char)*(resposta++);
	break;
}

void atualiza_display_numero(unsigned char unidade, unsigned char dezena) {
	// Atualiza o display das dezenas
	// Limpa os bits correspondentes
	PORTD &= ~(1 << DISPLAY1_A);
	PORTC &= ~((1 << DISPLAY1_B) | (1 << DISPLAY1_C) | (1 << DISPLAY1_D) | (1 << DISPLAY1_E) | (1 << DISPLAY1_F) | (1 << DISPLAY1_G));
	PORTB = decodificador_7seg[unidade];
	PORTC = decodificador_7seg[dezena];
	if(dezena == 1 || dezena == 4){
		PORTD &= ~(1 << DISPLAY1_A);
		}else{
		PORTD |= (1 << DISPLAY1_A);
	}
	if(unidade == 1 || unidade == 4){
		PORTD &= ~(1 << DISPLAY2_A);
	}
	else{
		PORTD |= (1 << DISPLAY2_A);
	}
	
}
void le_serial(){
	if (flagRCV == 1)
	{
		erro = 0;
		flagRCV = 0;
		switch(frameIN[0]){
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			set_pwm_duty_cycle();
			default:
			resposta = mensagem[2];
			UDR0 = (char)*(resposta++);
			break;
		}
	}
	
	
	


}
int main(void) {
	// Inicializações
	setup();

	// Loop principal
	while(1) {
		le_serial();
		adc_value = read_adc(0); // Lê o valor do ADC no pino PC0
		float f_tensao = (adc_value * 5.0) / 1023.0;
		
		// Converte a tensão para um inteiro entre 0 e 5000
		tensao = (int)(f_tensao * 1000);
		
		// Calcula as unidades e dezenas
		unidade_tensao = tensao / 1000; // parte inteira
		decimal_tensao = (tensao % 1000)/100; // parte decimal
		
		// PWM por software para o LED azul
		for (unsigned char i = 0; i < 100; i++) {
			if (i < duty_cycle_blue) {
				PORTD |= (1 << BLUE_LED_PIN);
				} else {
				PORTD &= ~(1 << BLUE_LED_PIN);
			}
		}
	}
	
	return 0;
}



ISR(PCINT2_vect) {
	decrementa_pressionado = !(PIND & (1 << BUTTON_DECREMENT));
	incrementa_pressionado = !(PIND & (1 << BUTTON_INCREMENT));
	seleciona_cor_pressionado = !(PIND & (1 << BUTTON_SELECT_COLOR));
	if(seleciona_cor_pressionado ==1) {
		flag_seleciona_cor = 1;
	}
	else if ((seleciona_cor_pressionado ==0) && (flag_seleciona_cor== 1) && (seleciona_cor_pressionado == 0)){
		seleciona_cor = 1;
		flag_seleciona_cor = 0;
		}else{
		seleciona_cor = 0;
	}
	if (seleciona_cor_pressionado == 1) {
		if (cor < 2){
			cor++;
			}else{
			cor = 0;
		}
		display_color(cor);
		} else if (incrementa_pressionado) {
		switch (cor) {
			case 0:
			if (pwm_red < 99) {
				pwm_red++;
				f_pwm_red= pwm_red;
				duty_cycle_red= (f_pwm_red/100)*255;
				unidade_red = pwm_red % 10;
				dezena_red = pwm_red / 10;
			}
			break;
			case 1:
			if (pwm_green < 99) {
				pwm_green++;
				f_pwm_green = pwm_green;
				duty_cycle_green= (f_pwm_green/100)*255;
				unidade_green = pwm_green % 10;
				dezena_green = pwm_green / 10;
			}
			break;
			case 2:
			if (pwm_blue < 99) {
				pwm_blue++;
				f_pwm_blue= pwm_blue;
				duty_cycle_blue= (f_pwm_blue/100)*255;
				unidade_blue = pwm_blue % 10;
				dezena_blue = pwm_blue / 10;
			}
			break;
		}
		set_pwm_duty_cycle(duty_cycle_red, duty_cycle_green, duty_cycle_blue);
		if(display_timer == 0){
			// Restaurar o display para mostrar os duty cycles atuais
			switch (cor) {
				case 0:
				atualiza_display_numero(unidade_red, dezena_red);
				break;
				case 1:
				atualiza_display_numero(unidade_green, dezena_green);
				break;
				case 2:
				atualiza_display_numero(unidade_blue, dezena_blue);
				break;
			}
		}

		} else if (decrementa_pressionado) {
		switch (cor) {
			case 0:
			if (pwm_red > 0) {
				pwm_red--;
				f_pwm_red= pwm_red;
				duty_cycle_red= (f_pwm_red/100)*255;
				unidade_red = pwm_red % 10;
				dezena_red = pwm_red / 10;
			}
			break;
			case 1:
			if (pwm_green > 0) {
				pwm_green--;
				f_pwm_green = pwm_green;
				duty_cycle_green= (pwm_green/100)*255;
				unidade_green = pwm_green % 10;
				dezena_green = pwm_green / 10;
			}
			break;
			case 2:
			if (pwm_blue > 0) {
				pwm_blue--;
				f_pwm_blue= pwm_blue;
				duty_cycle_blue= (f_pwm_blue/100)*255;
				unidade_blue = pwm_blue % 10;
				dezena_blue = pwm_blue / 10;
			}
			break;
		}
		set_pwm_duty_cycle(duty_cycle_red, duty_cycle_green, duty_cycle_blue);
		if(display_timer == 0){
			// Restaurar o display para mostrar os duty cycles atuais
			switch (cor) {
				case 0:
				atualiza_display_numero(unidade_red, dezena_red);
				break;
				case 1:
				atualiza_display_numero(unidade_green, dezena_green);
				break;
				case 2:
				atualiza_display_numero(unidade_blue, dezena_blue);
				break;
			}
		}
	}
}


ISR(TIMER1_COMPA_vect) {
	display_timer++;
	if(display_timer>1){
		display_timer = 0;
		// Restaurar o display para mostrar os duty cycles atuais
		switch (cor) {
			case 0:
			atualiza_display_numero(unidade_red, dezena_red);
			break;
			case 1:
			atualiza_display_numero(unidade_green, dezena_green);
			break;
			case 2:
			atualiza_display_numero(unidade_blue, dezena_blue);
			break;
		}
		// Desabilitar o Timer 1 após a atualização
		TIMSK1 &= ~(1 << OCIE1A);
	}
}

ISR(TIMER0_OVF_vect) {
	// Atualizações de overflow do Timer 0, se necessário
}
ISR(USART_RX_vect)
{
	frameIN[0] = UDR0;
	flagRCV = 1;
}

ISR(USART_TX_vect)
{
	if (*resposta != '\0')
	{
		if (*resposta == 'X')
		UDR0 = (char)corLetra[cor];
		else
		{
			if (*resposta == 'Y')
			{
				if ((*dutyPtr >= '0') && (*dutyPtr <= '9'))
				UDR0 = *dutyPtr++;
				else
				{
					while (*(++resposta) != '\n');
					UDR0 = *resposta;
				}
			}
			else
			UDR0 = *resposta;
		}
		resposta++;

	}
}