# Banco de pruebas STR: ATmega328P + FreeRTOS

Firmware base del Entorno de Trabajo Común (ejercicios 3 a 8). Lo usás con PlatformIO y lo simulás en Proteus.

## Estructura

```
str-freertos/
├── platformio.ini
├── include/
│   └── config_tareas.h      ← lo único que se cambia entre experimentos
├── src/
│   └── main.cpp             ← firmware
└── lib/
    └── FreeRTOS/            ← biblioteca feilipu/FreeRTOS 11.1.0-3
        └── src/FreeRTOSConfig.h
```

La biblioteca está copiada dentro del proyecto para poder editar su `FreeRTOSConfig.h`, como pide la consigna. Si la instalás desde el gestor de PlatformIO, cada descarga pisa los cambios. Todas las modificaciones respecto del original están marcadas con `[STR]`.

En `FreeRTOSConfig.h`:

| Opción | Original | Acá | Motivo |
|---|---|---|---|
| `configUSE_PREEMPTION` | 1 | 1 | Exigido por la consigna |
| `configUSE_TIME_SLICING` | 1 | 1 | 1 = Round Robin, 0 = FIFO (se cambia según el ejercicio) |
| `configMAX_PRIORITIES` | 4 | 6 | Idle (0) + reporte (1) + cuatro tareas (2 a 5) |
| `configUSE_TIMERS` | 1 | 0 | No se usan; ahorra RAM y una tarea del sistema |

En `FreeRTOSVariant.h` y `port.c`: **el tick se genera con el Timer1** y no con el watchdog, como trae la biblioteca. Da un tick de **exactamente 15 ms**, el que asume el enunciado (el watchdog da 16 ms nominales con un oscilador impreciso), y además permite medir tiempos con resolución de 4 µs leyendo el propio Timer1. Para volver al watchdog, comentá la línea `#define portUSE_TIMER1_TICK` en `FreeRTOSVariant.h`.

En `port.c`, además: **retorno del cambio de contexto compatible con Proteus**. El modelo AVR de Proteus no entrega ninguna interrupción nueva hasta ejecutar un `RETI` que cierre la interrupción en curso. Cuando la interrupción del tick reanuda una tarea que había cedido la CPU (por ejemplo, con `xTaskDelayUntil`), la biblioteca original vuelve con `RET`, y Proteus deja de atender interrupciones: el puerto serie se corta a mitad de un texto y el programa queda trabado. La versión modificada vuelve con `RETI` cuando la tarea restaurada tiene las interrupciones habilitadas, y con `RET` en los demás casos. En el micro real las dos formas se comportan igual.

## Compilar

Abrí la carpeta en VS Code con PlatformIO y compilá (Build). El archivo para Proteus queda en:

```
.pio/build/ATmega328P/firmware.hex
```

(También se genera `firmware.elf`, que Proteus acepta y permite depurar el código fuente.)

## Circuito en Proteus

- **ATmega328P**:
  - *Program File* = `firmware.hex`
  - *Clock Frequency* = **16 MHz**
  - Fusible **CKDIV8 sin programar** (si queda programado, el micro corre 8 veces más lento y el puerto serie sale con basura)
- **Logic Analyser**: canales 0 a 3 conectados a PB0 (A), PB1 (B), PB2 (C) y PB3 (D). Ajustá la resolución de captura para que entren varios períodos de la tarea más lenta.
- **Virtual Terminal**: su RXD a **TXD (PD1)** del micro, 9600 baudios, 8N1.

## Configurar un experimento (`include/config_tareas.h`)

| Qué | Dónde |
|---|---|
| Conjunto de tareas (Ej. 1 o Ej. 2) | `CONJUNTO` |
| Qué tareas corren | `TAREAS_ACTIVAS`, por ejemplo `(TAREA_D)` o `(TODAS)` |
| Prioridades | Columna `prio` de `TABLA_TAREAS` |
| Ajuste manual del lazo | `BLOQUES_POR_TICK_FIJO` |

**Prioridades:** en FreeRTOS un número **mayor** es **mayor** prioridad, al revés que la convención de la clase. Las tablas vienen cargadas con Rate Monotonic. Para probar FIFO o Round Robin entre tareas de igual prioridad, poné el mismo valor en la columna `prio` y elegí `configUSE_TIME_SLICING` en `FreeRTOSConfig.h`.

## Qué hace el firmware

Hay una **tarea monitor** de prioridad 1, la más baja después de idle. Hace tres cosas en orden:

1. **Calibra la espera activa.** Mide cuánto dura un tick real y cuántos bloques de trabajo entran en un tick. Todavía no existe ninguna otra tarea, así que nada interfiere.
2. **Crea las tareas periódicas** e informa cuáles creó. Si falta memoria para alguna, lo dice.
3. **Imprime un reporte** una vez por hiperperíodo.

Las **tareas periódicas** se activan todas juntas (desfase 0) y después cada T exacto, con `xTaskDelayUntil`. En cada activación:
- ponen su pin en alto,
- ejecutan C unidades de espera activa,
- ponen el pin en bajo.

La espera activa cuenta bloques de trabajo, no tiempo de reloj, así que una tarea expropiada retoma su ráfaga sin perder lo que ya había consumido.

Al arrancar, la terminal debería mostrar:

```
=== Banco de pruebas STR - FreeRTOS ===
Conjunto: Ejercicio 1
Tick: 15000 us (Timer1)
Calibracion: 451 bloques por tick
Creando tareas: D
Primer reporte en un hiperperiodo.
```

Y después, una vez por hiperperíodo:

```
--- Reporte en t = 25 u (100 ticks) ---
Tarea Activ Perd  Rmax[ms] Jitter[ms]
  D      9    0      64.0        0.0
```

| Columna | Significado |
|---|---|
| Activ | Activaciones completadas |
| Perd | Trabajos que terminaron después de su plazo (D = T) |
| Rmax | Tiempo de respuesta máximo: desde la activación teórica hasta el fin de la ráfaga |
| Jitter | Máximo retardo entre la activación teórica y el inicio real de la ejecución |

Si el sistema operativo se queda sin memoria o una tarea desborda su pila, el firmware lo informa por la terminal (`*** ERROR: ...`) y se detiene.

Los tiempos se miden con resolución de 4 µs: el instante actual es el número de tick más la cuenta del Timer1 dentro del tick. Como las activaciones coinciden con ticks, el tiempo desde cada activación se obtiene exacto, sin depender de la resolución del tick.

Antes de arrancar, la tarea monitor espera a que el puerto serie termine de transmitir y crea todas las tareas con el planificador suspendido. Así todas se activan juntas en el instante 0 y el primer hiperperíodo no sufre interferencias.

## Inciso b): valores esperados para la tarea D sola

| | Teórico (1 unidad = 4 ticks = 60 ms) |
|---|---|
| Ancho de pulso (C = 1 unidad) | 60 ms |
| Período (T = 3 unidades) | 180 ms |
| Tiempo en bajo | 120 ms |

En Proteus, la tarea D sola dio en el terminal 0 plazos perdidos, Rmax = 60,4 ms y jitter 0,0 ms. Las variaciones de décimas de milisegundo se deben a las interrupciones del tick y del puerto serie, que le quitan unos microsegundos a la tarea.

## Observaciones para leer el analizador lógico

- **Un pin en alto no implica que esa tarea esté ejecutándose.** Si la tarea es expropiada, su pin sigue en alto hasta que termina su ráfaga. En cada instante ejecuta la tarea de mayor prioridad con el pin en alto. Un instante de expropiación se ve como un flanco ascendente de una tarea más prioritaria mientras otra ya estaba en alto.
- **Cronogramas "justos".** En el conjunto del Ejercicio 1, varias tareas terminan exactamente en el instante en que se activa otra. Diferencias de microsegundos, como la interrupción del tick o las del puerto serie mientras se imprime el reporte, pueden cambiar el orden en esos instantes. En el primer hiperperíodo, sin reportes de por medio, los tiempos de respuesta coinciden con el análisis teórico de Rate Monotonic en los dos conjuntos. En los siguientes, Rmax puede superar el valor teórico sin que se pierdan plazos.
- **La tarea de reporte también interfiere un poco.** Las interrupciones de transmisión serie de cada reporte roban unos microsegundos a la tarea que esté ejecutándose, y eso alcanza para cambiar el orden en los instantes "justos" de arriba.
