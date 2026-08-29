#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    // PD2 y PD3 como entradas
    DDRD &= ~((1 << PD2) | (1 << PD3));

    // Activar pull-up internas
    PORTD |= (1 << PD2) | (1 << PD3);

    // PB0 y PB1 como salidas
    DDRB |= (1 << PB0) | (1 << PB1);

    // Ambos LEDs encendidos al iniciar
    PORTB |= (1 << PB0) | (1 << PB1);

    while (1)
    {
        // Verificar si alguno de los pulsadores está presionado
        if (!(PIND & (1 << PD2)) || !(PIND & (1 << PD3)))
        {
            while(1){
                // PB0 encendido, PB1 apagado
                PORTB |= (1 << PB0);
                PORTB &= ~(1 << PB1);

                _delay_ms(250);

                // PB0 apagado, PB1 encendido
                PORTB &= ~(1 << PB0);
                PORTB |= (1 << PB1);

                _delay_ms(250);

            }
            
        }
    }

    return 0;
}