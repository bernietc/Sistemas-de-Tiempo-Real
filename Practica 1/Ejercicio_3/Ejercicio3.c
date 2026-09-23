#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#define LE_LATCH_1 PC1
#define LE_LATCH_2 PC2

int main(void)
{
    DDRB = 0xFF;

    DDRC |= (1 << DDC1) | (1 << DDC2);
    DDRC &= ~(1 << DDC0);

    PORTC &= ~((1 << LE_LATCH_1) | (1 << LE_LATCH_2));
    PORTC &= ~(1 << PC0);

    ADMUX = (1 << REFS0);

    ADCSRA = (1 << ADEN) |
             (1 << ADPS2) |
             (1 << ADPS1) |
             (1 << ADPS0);

    while (1)
    {
        uint16_t valor_adc;
        uint8_t byte_bajo;
        uint8_t byte_alto;

        ADCSRA |= (1 << ADSC);

        while ((ADCSRA & (1 << ADIF)) == 0)
        {
        }

        valor_adc = ADC;

        ADCSRA |= (1 << ADIF);

        byte_bajo = (uint8_t)(valor_adc & 0x00FF);
        byte_alto = (uint8_t)(valor_adc >> 8);

        PORTB = byte_bajo;

        PORTC |= (1 << LE_LATCH_1);
        _delay_us(1);
        PORTC &= ~(1 << LE_LATCH_1);

        PORTB = byte_alto;

        PORTC |= (1 << LE_LATCH_2);
        _delay_us(1);
        PORTC &= ~(1 << LE_LATCH_2);
    }
}