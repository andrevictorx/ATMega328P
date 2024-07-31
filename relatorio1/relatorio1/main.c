#include <avr/io.h>
#include <util/delay.h>
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

int main(void)
{
	int unidade = 0, dezena = 0;
	DDRD = 0xFF; // Configura PD0-PD6 como saídas
	DDRB = 0x3F; // Configura PB0-PB5 como saídas
	while (1)
	{
		// Atualiza o display das dezenas
		PORTD = decodificador_7seg[dezena]; // Atualiza os pinos PD0-PD6 com o dígito da dezena
		// Atualiza o display das unidades
		PORTD |= (decodificador_7seg[unidade] & 0b00000001) << PD7; // Atualiza o segmento A (PD7) com o dígito da unidade
		PORTB = (decodificador_7seg[unidade] & 0b01111110) >> 1; // Atualiza os pinos PB0-PB5 com os segmentos B-G da unidade
		// Incrementa os valores de unidade e dezena
		unidade++; // Incrementa a unidade
		if (unidade == 10) // Se a unidade atingir 10, reinicia em 0 e incrementa a dezena
		{
			unidade = 0;
			dezena++;
			if (dezena == 10) // Se a dezena atingir 10, reinicia em 0
			{
				dezena = 0;
			}
		}
		_delay_ms(10000); 
	}
	return (0);
}
