#include <Arduino.h>
#include <avr/io.h>
#include <Arduino_FreeRTOS.h>
#include <task.h>

/* 1. Definir F_CPU explícitamente */
#define F_CPU 16000000UL
#define BAUD 9600
#define BRC ((F_CPU/16/BAUD) - 1)

void uart_init(void) {
    /* 2. Forzar UCSR0A a 0 para anular la doble velocidad (U2X0) de Arduino */
    UCSR0A = 0; 
    UBRR0H = (BRC >> 8);
    UBRR0L = BRC;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_puts(const char* str) {
    while(*str) {
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 = *str++;
    }
}

void vTaskImprimirMensaje(void *pvParameters) {
    char *nombreTarea = (char *)pvParameters;
    
    for(;;) {
        uart_puts("Ejecutando: ");
        uart_puts(nombreTarea);
        uart_puts("\r\n"); 
        
        
        vTaskDelay(pdMS_TO_TICKS(500)); //escenario 1: se ejecuta un round robin. Las tareas se turnan
                                        //el TaskDelay pone a la tarea de estado running a blocked por un tiempo dteerminado, lo que hace que otra tarea pueda ejecutarse. 
                                        //Si no se pone el TaskDelay, la tarea que se ejecuta primero nunca cede el control y las otras tareas no se ejecutan.          

                                        //se puede probar este escenario cambiando las prioridades de las tareas tambien.


        //volatile uint32_t ul;         //escenario 2: una tarea se ejecuta y toma el control de la CPU, las otras tareas no se ejecutan.
        //for(ul = 0; ul < 30000; ul++) {}
      }
}

extern "C" void setup() {
    uart_init();

    xTaskCreate(vTaskImprimirMensaje, "T1", 100, (void *)"Tarea 1", 1, NULL); //el quinto parametro define prioridad
    xTaskCreate(vTaskImprimirMensaje, "T2", 100, (void *)"Tarea 2", 1, NULL);
    xTaskCreate(vTaskImprimirMensaje, "T3", 100, (void *)"Tarea 3", 1, NULL);
}

extern "C" void loop() {
    /* 3. CRÍTICO: Debe estar completamente vacío. Borrar el for(;;); */
}