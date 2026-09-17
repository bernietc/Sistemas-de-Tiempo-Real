#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

/* Arreglo centralizado que define el orden exacto de ejecución */
const uint8_t secuencia[] = {
    1, 3, 2,          // Secuencia A (índices 0, 1, 2)
    2, 2, 3, 1,       // Secuencia B (índices 3, 4, 5, 6)
    3, 3, 3, 1, 2     // Secuencia C (índices 7, 8, 9, 10, 11)
};
const uint8_t TOTAL_PASOS = 12;

/* Variable global compartida para saber en qué paso vamos */
volatile uint8_t paso_actual = 0;

/* Arreglo de semáforos binarios: un semáforo de bloqueo por cada tarea */
SemaphoreHandle_t semTareas[3];

void vTaskGenerica(void *pvParameters) {
    int mi_id = (int)pvParameters; // Recibe 1, 2 o 3 al crearse
    
    for(;;) {
        /* 1. Bloqueo indefinido: la tarea duerme hasta que otra le haga "Give" a su semáforo */
        xSemaphoreTake(semTareas[mi_id - 1], portMAX_DELAY);
        
        /* 2. Formateo de inicio de secuencia */
        if (paso_actual == 0) Serial.print("A. ");
        else if (paso_actual == 3) Serial.print("\r\nB. ");
        else if (paso_actual == 7) Serial.print("\r\nC. ");
        
        /* 3. Impresión del nombre */
        Serial.print("Tarea ");
        Serial.print(mi_id);
        
        /* 4. Formateo del guion separador o fin de línea */
        if (paso_actual != 2 && paso_actual != 6 && paso_actual != 11) {
            Serial.print("- ");
        } else if (paso_actual == 11) {
            Serial.println(); // Salto de línea extra al finalizar el bloque C
        }
        
        /* Retardo voluntario de 400ms para poder leer la terminal en Proteus */
        vTaskDelay(pdMS_TO_TICKS(400));
        
        /* 5. Lógica de transición de estados */
        paso_actual++;
        if (paso_actual >= TOTAL_PASOS) {
            paso_actual = 0; // Reinicia el ciclo completo
        }
        
        /* 6. Despierta a la Tarea que corresponde al siguiente paso de la secuencia */
        uint8_t id_siguiente = secuencia[paso_actual];
        xSemaphoreGive(semTareas[id_siguiente - 1]);
    }
}

void setup() {
    Serial.begin(9600);
    
    /* Crear semáforos binarios para la sincronización */
    semTareas[0] = xSemaphoreCreateBinary();
    semTareas[1] = xSemaphoreCreateBinary();
    semTareas[2] = xSemaphoreCreateBinary();
    
    /* Crear las tres tareas con la MISMA prioridad (1) */
    xTaskCreate(vTaskGenerica, "T1", 120, (void *)1, 1, NULL);
    xTaskCreate(vTaskGenerica, "T2", 120, (void *)2, 1, NULL);
    xTaskCreate(vTaskGenerica, "T3", 120, (void *)3, 1, NULL);
    
    /* Disparador de inicio: Entregamos el primer token a la Tarea 1 para romper la inercia */
    xSemaphoreGive(semTareas[0]);
}

void loop() {
    // Vacío
}