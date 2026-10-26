# Sistemas de Tiempo Real

Repositorio de las resoluciones de las prácticas de la materia y del desarrollo del Trabajo Práctico Integrador (TPI). Incluye ejercicios, documentación y versiones sucesivas de una simulación basada en agentes implementada en NetLogo.

## Contenido del repositorio

El material se organiza en cuatro carpetas:

- **Tres carpetas de prácticas:** contienen las resoluciones y los archivos correspondientes a cada práctica de la materia.
- **Una carpeta del trabajo integrador:** reúne las versiones del modelo de NetLogo, imágenes y mapas de referencia, y la propuesta del trabajo en formato PDF.

Las versiones del modelo se conservan para registrar su evolución, desde los primeros prototipos hasta la representación de la línea completa.

## Trabajo Práctico Integrador: simulación de la línea D

El trabajo propone representar la circulación de trenes y pasajeros de la línea D del subte de Buenos Aires mediante una **simulación basada en agentes (ABMS)**. El objetivo principal es analizar cómo el aumento de la frecuencia del servicio modifica los tiempos de espera, la ocupación de las formaciones y la acumulación de pasajeros en las estaciones.

La pregunta que orienta el desarrollo es:

> ¿Cómo cambian la espera de los pasajeros, la ocupación de los trenes y las colas en las estaciones de la línea D al reducir el intervalo entre salidas?

Para estudiar esta relación, el modelo permite modificar parámetros como la cantidad de trenes disponibles, su capacidad, el intervalo entre salidas y la demanda de pasajeros. Un intervalo menor representa una mayor frecuencia.

### Elementos del modelo

- **Trenes:** representan formaciones completas, con capacidad limitada, sentido de circulación y estados de viaje, detención, maniobra o disponibilidad en terminal.
- **Pasajeros:** cada agente representa un grupo de personas con el mismo origen, destino e instante de llegada. Los grupos esperan, suben si hay capacidad y descienden en su destino.
- **Estaciones:** constituyen el entorno donde interactúan trenes y pasajeros. Mantienen colas separadas por sentido y reciben la demanda de ingreso.

Las formaciones circulan en ambos sentidos entre Catedral y Congreso de Tucumán. En cada parada descienden primero quienes llegaron a destino y luego suben quienes esperan, respetando el orden de llegada. Si un grupo no entra completo, se divide y el resto permanece en la cola. Al finalizar el recorrido, el tren realiza una maniobra en la terminal y queda disponible para regresar por la otra vía.

## Evolución de los modelos de NetLogo

- **Versión 1 — Recorrido básico (`Subte_D_Prototipo_01.nlogox`):** representa tres estaciones ficticias y una formación que realiza el recorrido A → B → C → B → A, con tiempos de viaje y detención configurables. Establece la base del movimiento, los estados del tren y el reloj de la simulación; todavía no incorpora pasajeros ni permite evaluar la frecuencia del servicio.

- **Versión 2 — Pasajeros, capacidad y colas (`Subte_D_Prototipo_02.nlogox`):** agrega grupos de pasajeros con origen y destino, colas por sentido, capacidad limitada y división de grupos cuando no hay lugar para todos. Permite observar cómo se acumulan pasajeros cuando el tren no alcanza a transportar la demanda y comprobar la conservación de personas entre ingresados, esperando, a bordo y viajes finalizados.

- **Versión 3 — Dos vías, varias formaciones y frecuencia (`Subte_D_Prototipo_03.nlogox`):** incorpora una flota fija, salidas desde ambas terminales, maniobras de retorno y un intervalo configurable. Incluye un bloqueo simple para evitar que dos trenes ocupen el mismo tramo o andén en un mismo sentido. En los escenarios de prueba, reducir el intervalo de 120 a 60 segundos disminuyó la espera; también se observó que solicitar mayor frecuencia no garantiza alcanzarla si faltan formaciones o la circulación está limitada.

- **Versión 4 — Línea D completa y trazado geográfico (`Subte_D_Prototipo_04.nlogox`):** reemplaza el recorrido ficticio por las 16 estaciones reales y un trazado curvo basado en datos de SBASE. Agrega nombres, orientación de los trenes sobre las curvas y consulta de colas por estación. Amplía el alcance espacial del modelo, manteniendo los parámetros operativos y la demanda como valores de prueba.

- **Versión 5 — Visualización y lectura de indicadores (`Subte_D_Prototipo_05.nlogox`):** amplía el mapa, separa las vías, simplifica el símbolo de las formaciones y organiza los indicadores y gráficos a la derecha. Muestra las personas a bordo de cada tren y las colas de todas las estaciones, con un selector `estacion-foco` para consultar una estación particular. Las comparaciones realizadas conservaron los resultados de transporte de la versión 4: los cambios se concentran en la presentación y la legibilidad. Incluye ajustes de separación entre números, puntos y nombres.

Los resultados anteriores corresponden a **escenarios sintéticos de prueba**. Sirven para comprobar el comportamiento del modelo y comparar configuraciones, pero todavía no permiten concluir cuál es la frecuencia adecuada para el servicio real.

## Cómo ejecutar la simulación

1. Abrir un archivo `.nlogox` con **NetLogo 7.0.4**. Se recomienda comenzar por la versión más reciente.
2. En la pestaña **Interface**, ajustar los parámetros y pulsar **Preparar** o **Preparar / reiniciar**, según la versión.
3. Utilizar **Un segundo** para avanzar paso a paso o **Simular / pausar** para ejecutar de forma continua.
4. Consultar la pestaña **Info**, donde se explican los controles, las reglas, los supuestos y las pruebas sugeridas.

Un tick representa un segundo simulado. La velocidad de animación no modifica la duración de los viajes. En la versión más reciente, los cambios de parámetros requieren reiniciar con Preparar, excepto la demanda y `estacion-foco`, que pueden modificarse durante la ejecución.

Las versiones 3, 4 y 5 incluyen un experimento denominado **Comparar intervalos**, accesible desde **Tools → BehaviorSpace**.

## Estado actual y próximos pasos

La versión actual utiliza nombres y geometría reales, pero mantiene demanda uniforme, destinos equiprobables y tiempos y capacidades de prueba. También simplifica las maniobras y el bloqueo de circulación.

Los próximos pasos previstos son:

- Incorporar los ingresos de molinetes por estación y horario.
- Definir probabilidades de destino mediante hipótesis explícitas sobre las características de cada zona.
- Ajustar los tiempos de recorrido y la capacidad con fuentes correspondientes al período de estudio.
- Comparar escenarios con distintas frecuencias, manteniendo condiciones de demanda equivalentes.

Los perfiles de hora pico y valle, los factores demográficos y los eventos masivos se mantienen como posibles ampliaciones. No están implementados en la versión actual. El evento masivo es un objetivo secundario.

## Referencias

- [NetLogo](https://www.netlogo.org/).
- [Buenos Aires Data — Subte: Estaciones](https://data.buenosaires.gob.ar/dataset/subte-estaciones): fuente del trazado geográfico integrado en los modelos. Licencia indicada por el catálogo: CC-BY-2.5-AR.
- [Emova — Información de las estaciones](https://emova.com.ar/index.php/informacion-de-las-estaciones/): referencia para los nombres y el orden de las estaciones.

La documentación específica de cada versión y sus supuestos se encuentra en la pestaña **Info** del archivo correspondiente.
