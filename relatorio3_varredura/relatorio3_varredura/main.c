// Andre Victor Xavier Pires e Joao Vitor de Siqueira
#include <avr/interrupt.h>
#include <avr/io.h>
int estado = 0;
int contador;
int tempoDefault = 17000;
int tempo = 17000;
int main(void)
{
  static unsigned char contador = 0;
  // PB0:PB5 como saída
  DDRB = (0x01 << PINB0) | (0x01 << PINB1) |
         (0x01 << PINB2) | (0x01 << PINB3) |
         (0x01 << PINB4) | (0x01 << PINB5);
  // PBD:PD5 como saída
  DDRD = (0x01 << PIND0) | (0x01 << PIND1) |
         (0x01 << PIND2) | (0x01 << PIND3) |
         (0x01 << PIND4) | (0x01 << PIND5);
  
  DDRC = (0x01 << PINC0)| (0x01 << PINC1);
  PORTC = (0x01 << PINC0) | (0x01 << PINC1);// PC0:PC1 com resistor de pull-up


  // Habilita as interrupções mascaráveis
  sei();

  while (1)
  {
    if (contador == 7)
    { // Preencheu os displays -> zera tudo
      PORTB = 0;
      PORTD = 0;
      contador = 0;
    }
    else
    {
      if (estado == 0)
      { // Port B horário e Port C antihorário
        PORTB = (PORTB << 1) + 1;
        PORTD = (PORTD >> 1) + 0b00100000;
      } else
      { // Port C horário e Port B antihorário
        PORTD = (PORTD << 1) + 1;
        PORTB = (PORTB >> 1) + 0b00100000;
      }
    }
    // atraso de 125ms
    //delay(100);
    for (double i = 0; i < tempo; i++) {}
    // incrementa o contador de leds acessos
    contador++;
    while (((0x01 & PINC) | (0x02 & PINC)) == 0) {}
    if ((0x01 & PINC) == 0) {
      for (double i = 0; i < 13500; i++) {}
      // Botão do PC0 pressionado?
      estado = !estado; // inverte o sentido dos displays
    }
    if ((0x02 & PINC) == 0) {
      for (double i = 0; i < 13500; i++) {}
      tempo = tempo / 2;
      if (tempo <= (tempoDefault / 16)) {
        tempo = tempoDefault;
      }
    }
  }

}