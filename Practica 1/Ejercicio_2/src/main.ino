#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// Variables globales para el control del parpadeo
volatile uint8_t parpadeo_activo = 0;
volatile uint8_t contador_tiempo = 0;

void setup(void) {
    // 1. Configurar Puertos
    DDRB |= (1 << PB0) | (1 << PB1);    // PB0 y PB1 como salidas
    PORTB |= (1 << PB0) | (1 << PB1);   // Estado inicial: Ambos LEDs encendidos

    DDRD &= ~((1 << PD2) | (1 << PD3)); // PD2 y PD3 como entradas
    PORTD |= (1 << PD2) | (1 << PD3);   // Activar resistencias pull-up internas

    // 2. Configurar TIMER0 (Modo CTC)
    // si la frecuencia de cpu es de 16 MHz:
    // F_timer = 16,000,000 / 1024 = 15625 Hz
    // Interrupción cada 10 ms -> 15625 * 0.01s = 156.25 (Usamos 155 para contar de 0 a 155)
    TCCR0A = (1 << WGM01);              // Modo CTC --> va de 0 a 0CR0A
    TCCR0B = (1 << CS02) | (1 << CS00); // Prescaler = 1024
    OCR0A = 155;                        // Valor de tope para 10 ms
    
    TIMSK0 |= (1 << OCIE0A);            // Habilitar interrupción de Compare Match A
    sei();                              // Habilitar interrupciones globales
}

int main(void) {
    setup();

    while (1) {
        // Leer registro PIND directo. Al usar pull-ups, el pin lee 0 si está presionado.
        if (!(PIND & (1 << PIND2)) || !(PIND & (1 << PIND3))) {
            if (!parpadeo_activo) {
                parpadeo_activo = 1;
                // Configurar estado inicial del parpadeo: PB0 encendido, PB1 apagado
                PORTB = (PORTB & ~((1 << PB0) | (1 << PB1))) | (1 << PB0);
                contador_tiempo = 0;
            }
        }
    }
}

// Rutina de Servicio de Interrupción para TIMER0
ISR(TIMER0_COMPA_vect) {
    if (parpadeo_activo) {
        contador_tiempo++;
        
        // 25 interrupciones de 10 ms equivalen a 250 ms
        if (contador_tiempo >= 25) {
            contador_tiempo = 0;
            // Alternar los estados de PB0 y PB1
            PORTB ^= (1 << PB0) | (1 << PB1); 
        }
    }
}