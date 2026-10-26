/*
 * Banco de pruebas de planificación - Sistemas de Tiempo Real
 * ATmega328P + FreeRTOS (feilipu/FreeRTOS) - Framework Arduino / PlatformIO
 * ---------------------------------------------------------------------------
 * Cada tarea periódica:
 *   1. espera su activación (xTaskDelayUntil: activaciones exactas cada T),
 *   2. pone su pin del Puerto B en alto,
 *   3. consume C unidades de CPU con un lazo de ESPERA ACTIVA,
 *   4. pone el pin en bajo y registra sus estadísticas.
 *
 * El lazo de espera activa cuenta TRABAJO HECHO (bloques de iteraciones), no
 * tiempo de reloj: si la tarea es expropiada, al reanudarse sigue desde el
 * bloque en que quedó y no pierde el cómputo ya consumido.
 *
 * Una tarea de menor prioridad (reporte) imprime por TXD, para cada tarea:
 * activaciones, plazos perdidos, tiempo de respuesta máximo y jitter máximo.
 *
 * Toda la configuración del experimento está en include/config_tareas.h
 */

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <task.h>
#include "config_tareas.h"

/* ------------------------------------------------------------------------- */
/* Definiciones                                                              */
/* ------------------------------------------------------------------------- */
#define N_TAREAS            4
#define PRIO_MONITOR        1       /* la menor, después de idle (0) */
#define PILA_TAREA          128     /* bytes */
#define PILA_MONITOR        288

/* Un "bloque" es una cantidad fija de iteraciones vacías. */
#define ITER_POR_BLOQUE     50
/* Durante la calibración se consulta el reloj cada este número de bloques. */
#define BLOQUES_POR_CONSULTA 256

typedef struct {
    uint16_t activaciones;      /* trabajos completados                        */
    uint16_t perdidos;          /* trabajos que terminaron después del plazo   */
    uint32_t r_max_us;          /* tiempo de respuesta máximo [us]             */
    uint32_t jitter_max_us;     /* máx. (inicio real - activación teórica) [us]*/
} Estadisticas;

static const DefTarea tareas[N_TAREAS] = TABLA_TAREAS;
static Estadisticas   est[N_TAREAS];

static uint32_t   bloques_calibracion;  /* bloques que entran en TICKS_CALIBRACION ticks */
static TickType_t t_inicio;             /* instante 0 común a todas las tareas           */

/* ------------------------------------------------------------------------- */
/* Medición de tiempos con resolución de 4 us                                */
/* ------------------------------------------------------------------------- */
/* El tick del sistema lo genera el Timer1 en modo CTC (ver FreeRTOSVariant.h):
 * cuenta a 250 kHz (4 us por cuenta) y vuelve a 0 en cada tick. Entonces el
 * instante actual es: número de tick + cuentas del Timer1 dentro del tick.
 * Las activaciones de las tareas coinciden con ticks, así que el tiempo desde
 * una activación se obtiene exacto, sin relojes adicionales. */
#define US_POR_TICK     ((uint32_t)portTICK_PERIOD_MS * 1000UL)
#define US_POR_CUENTA   (64UL * 1000000UL / F_CPU)          /* prescaler 64 */

/* Lee el tick actual y las cuentas del Timer1 dentro de ese tick. */
static void leer_tiempo(TickType_t *tick, uint16_t *cuentas)
{
    taskENTER_CRITICAL();
    TickType_t t = xTaskGetTickCount();
    uint16_t   c = TCNT1;
    /* Si ya hubo comparación pero la interrupción del tick todavía no se
     * atendió (estamos en sección crítica), el contador ya volvió a 0:
     * corresponde al tick siguiente. */
    if ((TIFR1 & _BV(OCF1A)) && c < OCR1A / 2) {
        t++;
    }
    taskEXIT_CRITICAL();
    *tick = t;
    *cuentas = c;
}

/* Tiempo transcurrido [us] desde el comienzo del tick 'desde' hasta ahora. */
static uint32_t us_desde_tick(TickType_t desde)
{
    TickType_t t;
    uint16_t   c;
    leer_tiempo(&t, &c);
    return (uint32_t)(TickType_t)(t - desde) * US_POR_TICK + (uint32_t)c * US_POR_CUENTA;
}

/* Espera el comienzo del próximo tick y devuelve su número. */
static TickType_t esperar_flanco_tick(void)
{
    TickType_t t = xTaskGetTickCount();
    TickType_t nuevo;
    while ((nuevo = xTaskGetTickCount()) == t) { }
    return nuevo;
}

/* ------------------------------------------------------------------------- */
/* Consumo de CPU por espera activa                                          */
/* ------------------------------------------------------------------------- */
/* Ejecuta n bloques de trabajo. Es la única primitiva de consumo de CPU y la
 * usan tanto la calibración como las tareas, así el costo por bloque es
 * idéntico en los dos casos (noinline evita que el compilador la optimice
 * de forma distinta en cada lugar). */
static void __attribute__((noinline)) consumir_bloques(uint32_t n)
{
    while (n--) {
        for (volatile uint8_t i = 0; i < ITER_POR_BLOQUE; i++) {
            /* vacío: solo ocupa CPU */
        }
    }
}

/* Consume 'ticks' ticks de CPU. Las variables viven en la pila de la tarea,
 * así que una expropiación no hace perder el avance. */
static void consumir_cpu(uint16_t ticks)
{
    consumir_bloques(((uint32_t)ticks * bloques_calibracion) / TICKS_CALIBRACION);
}

/* ------------------------------------------------------------------------- */
/* Manejo de pines                                                           */
/* ------------------------------------------------------------------------- */
/* El pin de una tarea está en alto solo mientras esa tarea ocupa la CPU
 * durante su ráfaga:
 *   - al empezar la ráfaga, la tarea marca su bit en 'en_rafaga' y sube el pin;
 *   - si el planificador la saca de la CPU (expropiación o fin del cuanto en
 *     Round Robin), vSTRTareaSale() baja todos los pines;
 *   - cuando vuelve a la CPU, vSTRTareaEntra() sube su pin si sigue en ráfaga;
 *   - al terminar la ráfaga, borra su bit y baja el pin.
 * Así el analizador lógico muestra exactamente cuándo se ejecuta cada tarea. */
#define MASCARA_PINES   0x0F                /* PB0..PB3 */

static volatile uint8_t en_rafaga;          /* bits de las tareas con ráfaga en curso */

extern "C" void vSTRTareaSale(void)
{
    PORTB &= (uint8_t)~MASCARA_PINES;
}

extern "C" void vSTRTareaEntra(unsigned char bit_pin)
{
    PORTB = (uint8_t)((PORTB & ~MASCARA_PINES) | (bit_pin & en_rafaga));
}

static inline void inicio_rafaga(uint8_t bit)
{
    taskENTER_CRITICAL();
    en_rafaga |= (uint8_t)(1 << bit);
    PORTB     |= (uint8_t)(1 << bit);
    taskEXIT_CRITICAL();
}

static inline void fin_rafaga(uint8_t bit)
{
    taskENTER_CRITICAL();
    en_rafaga &= (uint8_t)~(1 << bit);
    PORTB     &= (uint8_t)~(1 << bit);
    taskEXIT_CRITICAL();
}

/* ------------------------------------------------------------------------- */
/* Tarea periódica genérica                                                  */
/* ------------------------------------------------------------------------- */
static void tarea_periodica(void *param)
{
    const uint8_t    idx = (uint8_t)(uintptr_t)param;
    const DefTarea  *t   = &tareas[idx];
    const TickType_t C   = (TickType_t)t->C * TICKS_POR_UNIDAD;
    const TickType_t T   = (TickType_t)t->T * TICKS_POR_UNIDAD;
    const uint32_t   D_us = (uint32_t)T * US_POR_TICK;  /* plazo = período */

    /* El tag de la tarea es el bit de su pin (lo usan los ganchos de arriba). */
    vTaskSetApplicationTaskTag(NULL, (TaskHookFunction_t)(uintptr_t)(1 << t->bit));

    /* Así la primera activación cae exactamente en t_inicio (desfase S = 0). */
    TickType_t activacion = t_inicio - T;

    for (;;) {
        xTaskDelayUntil(&activacion, T);    /* activacion = instante teórico */

        uint32_t retardo_us = us_desde_tick(activacion);
        inicio_rafaga(t->bit);
        consumir_cpu(C);
        fin_rafaga(t->bit);
        uint32_t respuesta_us = us_desde_tick(activacion);

        taskENTER_CRITICAL();
        est[idx].activaciones++;
        if (respuesta_us > D_us)                  est[idx].perdidos++;
        if (respuesta_us > est[idx].r_max_us)     est[idx].r_max_us = respuesta_us;
        if (retardo_us   > est[idx].jitter_max_us) est[idx].jitter_max_us = retardo_us;
        taskEXIT_CRITICAL();
    }
}

/* ------------------------------------------------------------------------- */
/* Calibración del lazo de espera activa                                     */
/* ------------------------------------------------------------------------- */
static void calibrar(void)
{
    /* Sincronizar con el comienzo de un tick. */
    TickType_t t0 = esperar_flanco_tick();

    /* Ejecutar bloques de trabajo durante al menos TICKS_CALIBRACION ticks,
     * consultando el reloj solo cada BLOQUES_POR_CONSULTA bloques para que
     * la consulta casi no pese en la medición. */
    uint32_t n = 0;
    while ((TickType_t)(xTaskGetTickCount() - t0) < TICKS_CALIBRACION) {
        consumir_bloques(BLOQUES_POR_CONSULTA);
        n += BLOQUES_POR_CONSULTA;
    }
    uint32_t us_total = us_desde_tick(t0);

#if BLOQUES_POR_TICK_FIJO > 0
    (void)n; (void)us_total;
    bloques_calibracion = (uint32_t)BLOQUES_POR_TICK_FIJO * TICKS_CALIBRACION;
#else
    /* Bloques que entran en TICKS_CALIBRACION ticks, a partir del tiempo
     * realmente transcurrido (el último grupo pudo pasarse del tick final). */
    bloques_calibracion = (uint32_t)((float)n * TICKS_CALIBRACION * US_POR_TICK
                                     / us_total + 0.5f);
#endif

    Serial.print(F("Tick: "));
    Serial.print(US_POR_TICK);
    Serial.println(F(" us (Timer1)"));
    Serial.print(F("Calibracion: "));
    Serial.print(bloques_calibracion / TICKS_CALIBRACION);
    Serial.println(F(" bloques por tick"));
}

/* ------------------------------------------------------------------------- */
/* Tarea monitor (la de menor prioridad)                                     */
/*   1. calibra la espera activa (todavía no existe ninguna otra tarea),     */
/*   2. crea las tareas periódicas,                                          */
/*   3. imprime el reporte una vez por hiperperíodo.                         */
/* ------------------------------------------------------------------------- */
static void imprimir_col(uint32_t valor, uint8_t ancho)
{
    uint8_t digitos = 1;
    for (uint32_t v = valor; v >= 10; v /= 10) digitos++;
    while (digitos++ < ancho) Serial.print(' ');
    Serial.print(valor);
}

/* Imprime microsegundos como milisegundos con un decimal, alineado. */
static void imprimir_ms(uint32_t us, uint8_t ancho)
{
    uint32_t decimas = (us + 50) / 100;
    imprimir_col(decimas / 10, ancho - 2);
    Serial.print('.');
    Serial.print((uint8_t)(decimas % 10));
}

static void tarea_monitor(void *param)
{
    (void)param;
    const TickType_t periodo = (TickType_t)PERIODO_REPORTE_UNIDADES * TICKS_POR_UNIDAD;

    calibrar();

    Serial.print(F("Creando tareas:"));
    for (uint8_t i = 0; i < N_TAREAS; i++) {
        if (TAREAS_ACTIVAS & (1 << i)) {
            Serial.print(' ');
            Serial.print(tareas[i].nombre);
        }
    }
    Serial.println();
    Serial.println(F("Primer reporte en un hiperperiodo."));

    /* Esperar a que el puerto serie termine de transmitir: si no, sus
     * interrupciones le robarían tiempo a las tareas al arrancar. */
    while (Serial.availableForWrite() < SERIAL_TX_BUFFER_SIZE - 1) {
        vTaskDelay(1);
    }
    vTaskDelay(1);      /* el último carácter todavía sale por el registro */

    /* Las tareas se crean con el planificador suspendido: así ninguna empieza
     * a ejecutarse antes de que existan todas (si no, las primeras creadas
     * podrían ocupar la CPU y demorar la creación de las demás). */
    uint8_t errores = 0;
    vTaskSuspendAll();
    t_inicio = xTaskGetTickCount() + 2;     /* instante 0 común: todas se activan juntas */
    for (uint8_t i = 0; i < N_TAREAS; i++) {
        if (!(TAREAS_ACTIVAS & (1 << i))) continue;
        char nombre[2] = { tareas[i].nombre, '\0' };
        uint8_t prio = (PRIORIDAD_UNICA > 0) ? PRIORIDAD_UNICA : tareas[i].prioridad;
        if (xTaskCreate(tarea_periodica, nombre, PILA_TAREA,
                        (void *)(uintptr_t)i, prio, NULL) != pdPASS) {
            errores++;
        }
    }
    xTaskResumeAll();
    if (errores) {
        Serial.println(F("ERROR: sin memoria para crear todas las tareas"));
    }

    /* Alinear los reportes con el instante 0. */
    TickType_t ultimo = t_inicio - periodo;
    xTaskDelayUntil(&ultimo, periodo);          /* ultimo = t_inicio */

    for (;;) {
        xTaskDelayUntil(&ultimo, periodo);

        /* Como es la tarea de menor prioridad, puede ejecutarse bastante
         * después de despertarse: se informa el instante real de la copia. */
        Estadisticas copia[N_TAREAS];
        taskENTER_CRITICAL();
        memcpy(copia, est, sizeof(est));
        TickType_t transcurrido = xTaskGetTickCount() - t_inicio;
        taskEXIT_CRITICAL();

        Serial.print(F("\r\n--- Reporte en t = "));
        Serial.print(transcurrido / TICKS_POR_UNIDAD);
        Serial.print(F(" u ("));
        Serial.print(transcurrido);
        Serial.println(F(" ticks) ---"));
        Serial.println(F("Tarea Activ Perd  Rmax[ms] Jitter[ms]"));

        for (uint8_t i = 0; i < N_TAREAS; i++) {
            if (!(TAREAS_ACTIVAS & (1 << i))) continue;
            Serial.print(F("  "));
            Serial.print(tareas[i].nombre);
            Serial.print(F("  "));
            imprimir_col(copia[i].activaciones, 5);
            imprimir_col(copia[i].perdidos, 5);
            imprimir_ms(copia[i].r_max_us, 10);
            imprimir_ms(copia[i].jitter_max_us, 11);
            Serial.println();
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Errores fatales del sistema operativo                                     */
/* ------------------------------------------------------------------------- */
/* Escribe por la UART sin interrupciones (el sistema puede estar roto). */
static void uart_error(const __FlashStringHelper *msg, const char *detalle)
{
    cli();
    const char *p = (const char *)msg;
    char c;
    while ((c = pgm_read_byte(p++)) != 0) {
        while (!(UCSR0A & _BV(UDRE0))) { }
        UDR0 = c;
    }
    while (detalle && *detalle) {
        while (!(UCSR0A & _BV(UDRE0))) { }
        UDR0 = *detalle++;
    }
    while (!(UCSR0A & _BV(UDRE0))) { }
    UDR0 = '\r';
    while (!(UCSR0A & _BV(UDRE0))) { }
    UDR0 = '\n';
}

extern "C" void vApplicationMallocFailedHook(void)
{
    uart_error(F("\r\n*** ERROR: memoria RAM insuficiente ***"), NULL);
    for (;;) { }
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    uart_error(F("\r\n*** ERROR: desborde de pila en la tarea "), pcTaskName);
    for (;;) { }
}

/* ------------------------------------------------------------------------- */
/* Arduino                                                                   */
/* ------------------------------------------------------------------------- */
void setup()
{
    DDRB  |=  0x0F;     /* PB0..PB3 como salidas */
    PORTB &= ~0x0F;     /* y en bajo             */

    Serial.begin(BAUDIOS);
    Serial.println(F("\r\n=== Banco de pruebas STR - FreeRTOS ==="));
    Serial.print(F("Conjunto: Ejercicio "));
    Serial.println(CONJUNTO);

    if (xTaskCreate(tarea_monitor, "Mon", PILA_MONITOR, NULL,
                    PRIO_MONITOR, NULL) != pdPASS) {
        Serial.println(F("ERROR: sin memoria para la tarea monitor"));
    }
    /* La biblioteca arranca el planificador al salir de setup(). */
}

void loop()
{
    /* Se ejecuta dentro de la tarea idle: no poner nada acá. */
}
