/*
 * FreeRTOS Kernel V11.1.0
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <avr/io.h>

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See https://www.freertos.org/a00110.html.

 *----------------------------------------------------------*/

/* Delay definition - here, the user can choose which delay implementation is required.
 * The default is to change nothing. */
#define configUSE_PORT_DELAY                1

/* And on to the things the same no matter the AVR type... */
/* [STR] Habilitado: planificación expropiativa (consigna del Entorno Común). */
#define configUSE_PREEMPTION                1

#define configCPU_CLOCK_HZ                  ( ( uint32_t ) F_CPU )          // This F_CPU variable set by the environment
#define configTICK_TYPE_WIDTH_IN_BITS       TICK_TYPE_WIDTH_16_BITS

/* [STR] Original: 4. Se amplía a 6 para tener idle (0), tarea de reporte (1)
   y cuatro prioridades distintas para las tareas periódicas (2 a 5). */
#define configMAX_PRIORITIES                6
#define configMAX_TASK_NAME_LEN             16

/* Set the stack depth type to be uint16_t, otherwise it defaults to StackType_t */
#define configSTACK_DEPTH_TYPE              uint16_t

#define configMINIMAL_STACK_SIZE            192
#define configCHECK_FOR_STACK_OVERFLOW      1
#define configUSE_TRACE_FACILITY            0

#define configUSE_MUTEXES                   1
#define configUSE_RECURSIVE_MUTEXES         1
#define configUSE_COUNTING_SEMAPHORES       1
/* [STR] 1 = Round Robin entre tareas de igual prioridad, 0 = FIFO. */
#define configUSE_TIME_SLICING              0

#define configUSE_QUEUE_SETS                0
/* [STR] Original: 0. Cada tarea periódica guarda en su "tag" el bit de su pin
   del Puerto B, para que los ganchos de cambio de contexto de abajo sepan qué
   pin corresponde a la tarea que entra o sale de la CPU. */
#define configUSE_APPLICATION_TASK_TAG      1
#define configUSE_MALLOC_FAILED_HOOK        1
#define configQUEUE_REGISTRY_SIZE           0
#define configSUPPORT_DYNAMIC_ALLOCATION    1
#define configSUPPORT_STATIC_ALLOCATION     0

#define configUSE_IDLE_HOOK                 1
#define configIDLE_SHOULD_YIELD             1
#define configUSE_TICK_HOOK                 0

/* Timer definitions. */
/* [STR] Original: 1. Se desactivan los timers de software (no se usan) para
   ahorrar RAM y no tener una tarea extra del sistema compitiendo por la CPU. */
#define configUSE_TIMERS                    0
#define configTIMER_TASK_PRIORITY           ( configMAX_PRIORITIES-1 )
#define configTIMER_TASK_STACK_DEPTH        92
#define configTIMER_QUEUE_LENGTH            10

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

/* Set the following INCLUDE_* constants to 1 to incldue the named API function,
 * or 0 to exclude the named API function.  Most linkers will remove unused
 * functions even when the constant is 1. */
#define INCLUDE_vTaskPrioritySet               1
#define INCLUDE_uxTaskPriorityGet              1
#define INCLUDE_vTaskDelete                    1
#define INCLUDE_vTaskSuspend                   1
#define INCLUDE_xTaskResumeFromISR             1
#define INCLUDE_xTaskDelayUntil                1
#define INCLUDE_vTaskDelay                     1
#define INCLUDE_xTaskGetSchedulerState         0
#define INCLUDE_xTaskGetCurrentTaskHandle      1
#define INCLUDE_uxTaskGetStackHighWaterMark    1
#define INCLUDE_xTaskGetIdleTaskHandle         0
#define INCLUDE_eTaskGetState                  0
#define INCLUDE_xEventGroupSetBitFromISR       1
#define INCLUDE_xTimerPendFunctionCall         0
#define INCLUDE_xTaskAbortDelay                0
#define INCLUDE_xTaskGetHandle                 0

#define configMAX(a,b)  ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define configMIN(a,b)  ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

/**
 * configASSERT macro: https://www.freertos.org/a00110.html#configASSERT
 */
#ifndef configASSERT
    #define configDEFAULT_ASSERT 0
#else
    /**
     * Enable configASSERT macro if it is defined.
     */
    #ifndef configDEFAULT_ASSERT
        #define configDEFAULT_ASSERT 1
    #endif

    /**
     * Define a hook method for configASSERT macro if configASSERT is enabled.
     */
    #if configDEFAULT_ASSERT == 1
        extern void vApplicationAssertHook();
        #define configASSERT( x ) if (( x ) == 0) { vApplicationAssertHook(); }
    #endif
#endif


/* [STR] Ganchos de cambio de contexto: el pin de una tarea está en alto solo
 * mientras esa tarea realmente ocupa la CPU durante su ráfaga. Si la tarea es
 * expropiada (o se le termina el cuanto en Round Robin), su pin baja y vuelve
 * a subir cuando retoma. Las funciones están en src/main.cpp. Estas macros se
 * expanden dentro de tasks.c, donde pxCurrentTCB es la tarea que sale/entra. */
#ifndef __ASSEMBLER__
    #ifdef __cplusplus
    extern "C" {
    #endif
    void vSTRTareaSale( void );
    void vSTRTareaEntra( unsigned char ucBitPin );
    #ifdef __cplusplus
    }
    #endif
#endif
#define traceTASK_SWITCHED_OUT()    vSTRTareaSale()
#define traceTASK_SWITCHED_IN()     vSTRTareaEntra( ( unsigned char ) ( unsigned int ) pxCurrentTCB->pxTaskTag )

#endif /* FREERTOS_CONFIG_H */
