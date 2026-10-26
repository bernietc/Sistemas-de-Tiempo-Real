/*
 * config_tareas.h
 * ---------------------------------------------------------------------------
 * Configuración del experimento. Para pasar de un ejercicio a otro se cambia
 * SOLO este archivo (y, si la consigna lo pide, FreeRTOSConfig.h de la
 * biblioteca en lib/FreeRTOS/src).
 */
#pragma once
#include <stdint.h>

/* ===================== 1. Conjunto de tareas ============================== */
/* 1 = conjunto del Ejercicio 1, 2 = conjunto del Ejercicio 2 */
#define CONJUNTO            1

/* ===================== 2. Tareas activas ================================== */
#define TAREA_A             (1 << 0)    /* PB0 */
#define TAREA_B             (1 << 1)    /* PB1 */
#define TAREA_C             (1 << 2)    /* PB2 */
#define TAREA_D             (1 << 3)    /* PB3 */
#define TODAS               (TAREA_A | TAREA_B | TAREA_C | TAREA_D)

/* Calibración (Ej. 3 b): solo la tarea D. Para el conjunto completo: TODAS */
#define TAREAS_ACTIVAS      (TODAS)

/* ===================== 3. Escala de tiempos =============================== */
/* 1 unidad de los enunciados = 4 ticks de FreeRTOS */
#define TICKS_POR_UNIDAD    4

/* ===================== 4. Calibración del lazo de espera activa =========== */
/* 0  = calibración automática al arrancar: se mide cuántos bloques de trabajo
 *      entran en TICKS_CALIBRACION ticks reales del sistema operativo.
 * >0 = valor fijo de bloques por tick (para ajustar a mano si el analizador
 *      lógico muestra un ancho de pulso distinto del teórico). */
#define BLOQUES_POR_TICK_FIJO   0
#define TICKS_CALIBRACION       20

/* ===================== 5. Reporte por puerto serie ======================== */
#define BAUDIOS                     9600
/* Cada cuántas unidades se imprime el reporte (conviene un hiperperíodo). */
#define PERIODO_REPORTE_UNIDADES    ((CONJUNTO == 1) ? 24 : 60)

/* ===================== 6. Tabla de tareas ================================= */
/*
 * C y T en UNIDADES del enunciado. Plazo D = T.
 *
 * PRIORIDADES: en FreeRTOS un número MAYOR es MAYOR prioridad (al revés que
 * la convención de la clase). Valores válidos para las tareas: 2 a 5.
 *   0 = tarea idle del sistema, 1 = tarea de reporte (la de menor prioridad).
 * Los valores de abajo corresponden a Rate Monotonic (menor período -> mayor
 * prioridad). Para los ejercicios con tareas de igual prioridad (FIFO / Round
 * Robin) se modifican acá.
 */
/* Para los ejercicios con todas las tareas a la misma prioridad (Round Robin /
 * FIFO): poner acá esa prioridad (2 a 5) y se ignora la columna 'prio'.
 * 0 = usar la columna 'prio' de la tabla.
 * El reparto entre tareas de igual prioridad se elige con configUSE_TIME_SLICING
 * en lib/FreeRTOS/src/FreeRTOSConfig.h (1 = Round Robin, 0 = FIFO). */
#define PRIORIDAD_UNICA     2       /* Ejercicio 4: Round Robin */

typedef struct {
    char    nombre;
    uint8_t bit;        /* bit del Puerto B (PB0..PB3) */
    uint8_t C;          /* tiempo de cómputo [unidades] */
    uint8_t T;          /* período = plazo [unidades]   */
    uint8_t prioridad;  /* prioridad FreeRTOS (2..5)    */
} DefTarea;

#if CONJUNTO == 1
/*                      nombre bit  C   T  prio */ //menor periodo, mayor prioridad
#define TABLA_TAREAS { { 'A',   0,  2, 12,  3 }, \
                       { 'B',   1,  2, 24,  2 }, \
                       { 'C',   2,  2,  6,  4 }, \
                       { 'D',   3,  1,  3,  5 } }
#elif CONJUNTO == 2
#define TABLA_TAREAS { { 'A',   0,  1, 10,  4 }, \
                       { 'B',   1,  3, 12,  3 }, \
                       { 'C',   2,  7, 20,  2 }, \
                       { 'D',   3,  1,  5,  5 } }
#else
#error "CONJUNTO debe ser 1 o 2"
#endif
