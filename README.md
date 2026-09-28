# Planificador de Actividades — Tarea 1 Sistemas Operativos

## 1. Descripción

Este proyecto implementa un planificador de actividades en C utilizando procesos, tuberías (pipes) y señales.

Las actividades se representan como un **Grafo Acíclico Dirigido (DAG)**. Cada actividad puede depender de una o más actividades anteriores y solo puede comenzar cuando todas sus dependencias hayan finalizado correctamente.

El programa recibe un archivo de planificación y un límite de concurrencia `K`.

El formato de ejecución es:

```bash
./planificador plan.txt K
```

Donde:

* `plan.txt`: archivo que contiene las actividades, sus tiempos y dependencias.
* `K`: cantidad máxima de procesos de actividades que pueden estar ejecutándose simultáneamente.

El formato esperado para cada actividad es:

```text
ID_Actividad : Nombre_Actividad : tiempo_ms : Dependencia1, Dependencia2, ...
```

Si el tiempo de una actividad se encuentra vacío, se asigna aleatoriamente un valor entre **100 y 5000 ms**, según lo indicado en el enunciado.

---

## 2. Estructura del proyecto

```text
.
├── include/
│   ├── dag.h
│   ├── parser.h
│   └── scheduler.h
├── src/
│   ├── dag.c
│   ├── main.c
│   ├── parser.c
│   └── scheduler.c
├── Makefile
├── plan.txt
└── README.md
```

---

## 3. Funciones implementadas

### `parsear_archivo()` — `parser.c`

Lee el archivo de planificación línea por línea y obtiene:

* ID de la actividad.
* Nombre.
* Tiempo de ejecución.
* Lista de dependencias.

Se utiliza `strchr()` para separar los campos y `trim()` para eliminar espacios innecesarios.

Cuando el tiempo está vacío, se genera aleatoriamente entre 100 y 5000 milisegundos.

También se controla que no se supere el máximo de actividades definido por `MAX_ACTIVIDADES`.

---

### `buscar_actividad_por_id()` — `dag.c`

Busca una actividad dentro del arreglo utilizando su identificador.

Retorna el índice correspondiente a la actividad o `-1` si no se encuentra.

---

### `construir_dag()` — `dag.c`

Construye las relaciones de dependencia entre las actividades.

Para cada dependencia:

1. Busca la actividad de origen mediante su ID.
2. Incrementa `dependencias_restantes` de la actividad dependiente.
3. Registra la actividad dependiente dentro del arreglo de sucesores de la actividad de origen.

De esta manera, cuando una actividad termina, el planificador puede identificar qué actividades pueden quedar desbloqueadas.

También se inicializan el estado de cada actividad y sus descriptores de pipes.

---

### `imprimir_dag()` — `dag.c`

Muestra por pantalla la estructura construida del grafo, incluyendo:

* Actividad.
* Nombre.
* Cantidad de dependencias pendientes.
* Actividades sucesoras.

Esto permite verificar visualmente que las relaciones del archivo fueron interpretadas correctamente.

---

### `ejecutar_planificador()` — `scheduler.c`

Es el motor principal del sistema.

Sus responsabilidades son:

* Mantener una cola de actividades listas.
* Crear procesos mediante `fork()`.
* Respetar el límite de concurrencia `K`.
* Esperar mediante `waitpid()` a que terminen los procesos.
* Actualizar las dependencias restantes.
* Crear y utilizar pipes para comunicar actividades.
* Propagar los mensajes de finalización hacia las actividades dependientes.
* Manejar la señal `SIGINT`.
* Detectar fallos de procesos y abortar únicamente las ramas dependientes.

El límite `K` se controla manteniendo un registro de los procesos activos. Cuando se alcanza el límite, el proceso padre utiliza:

```c
waitpid(-1, &status, 0);
```

para esperar de manera bloqueante hasta que termine alguno de los procesos activos.

De esta forma no se utiliza espera activa (*busy-waiting*).

---

### `abortar_rama_recursivo()` — `scheduler.c`

Se utiliza para el aislamiento de errores.

Cuando una actividad falla, se marca como `ESTADO_FALLIDA` y se recorren recursivamente sus sucesores.

Las actividades que dependen de la actividad fallida son canceladas, mientras que las ramas independientes continúan ejecutándose normalmente.

---

## 4. Comunicación mediante Pipes

Las actividades se comunican mediante pipes.

Cuando una actividad termina correctamente, genera un mensaje de texto acotado indicando que su resultado está disponible y lo envía a cada uno de sus sucesores.

Por ejemplo:

```text
insumo de 'prender_carbon' (pid 1234) listo
```

Una actividad que posee varias dependencias realiza una lectura por cada insumo que debe recibir antes de comenzar su ejecución.

Los mensajes utilizan un tamaño fijo de 256 bytes.

### Creación de pipes

Para evitar crear innecesariamente una gran cantidad de descriptores al inicio de la ejecución, los pipes se crean **justo antes de ejecutar una actividad**, cuando se conocen sus sucesores y el pipe será necesario.

Después de realizar `fork()`, cada proceso hijo cierra los descriptores que no necesita. Esto evita mantener abiertos extremos de pipes pertenecientes a otras actividades y permite liberar descriptores correctamente.

---

## 5. Control de concurrencia

El planificador recibe un parámetro `K` que representa el máximo de procesos de actividades que pueden estar ejecutándose simultáneamente.

Ejemplo:

```bash
./planificador plan.txt 2
```

Con `K=2`, el planificador no crea más de dos procesos de actividades simultáneamente.

Cuando un proceso termina:

1. El padre recibe su estado mediante `waitpid()`.
2. Se libera un espacio de concurrencia.
3. Se actualizan las dependencias de sus sucesores.
4. Las actividades que ya no tienen dependencias pendientes se agregan a la cola de actividades listas.

---

## 6. Manejo de errores

Si una actividad termina con un estado distinto de éxito o termina debido a una señal, el planificador considera que dicha actividad falló.

En este caso se aborta únicamente la rama de actividades que depende de ella.

Las ramas independientes continúan normalmente.

Esto evita que el fallo de una actividad provoque la cancelación completa del plan.

---

## 7. Manejo de SIGINT

Cuando el usuario presiona `Ctrl+C`, se recibe la señal `SIGINT`.

El planificador utiliza `sigaction()` y una bandera de tipo `volatile sig_atomic_t` para registrar la señal.

Posteriormente, el proceso padre:

1. Detecta la solicitud de interrupción.
2. Envía `SIGTERM` a los procesos de actividades que se encuentran activos.
3. Espera a que estos procesos finalicen mediante `waitpid()`.
4. Finaliza la ejecución del planificador.

Esto permite abortar las actividades activas de manera controlada.

---

## 8. Manejo de memoria

La estructura `Actividad` contiene un arreglo de sucesores con capacidad para `MAX_SUCESORES`.

Actualmente:

```c
#define MAX_ACTIVIDADES 10005
#define MAX_SUCESORES MAX_ACTIVIDADES
```

Por esta razón, cada estructura `Actividad` ocupa aproximadamente **40.464 bytes** en el entorno de compilación utilizado.

La medición realizada con `sizeof(Actividad)` entrega:

```text
sizeof(Actividad) = 40464 bytes
MAX_ACTIVIDADES = 10005
Memoria para MAX_ACTIVIDADES = 386.09 MB
```

Debido a este tamaño, el arreglo principal de actividades se reserva dinámicamente en el heap mediante:

```c
Actividad *actividades = calloc(MAX_ACTIVIDADES, sizeof(Actividad));
```

Esto evita colocar una estructura de este tamaño directamente en el stack.

---

## 9. Pruebas realizadas

Se realizaron pruebas de compilación y ejecución utilizando:

```bash
make clean && make
```

con las siguientes opciones:

```text
-Wall -Wextra -std=c17
```

### Ejecución normal

Se ejecutó:

```bash
./planificador plan.txt 2
```

La planificación terminó correctamente, respetando las dependencias y el límite de concurrencia.

### Prueba de SIGINT

Se ejecutó el planificador y se presionó `Ctrl+C` durante la ejecución.

El planificador detectó la señal, terminó los procesos activos y finalizó controladamente.

### Prueba de aislamiento de errores

Se realizó una prueba provocando el fallo de una actividad.

El resultado esperado se verificó:

* La actividad fallida fue marcada como fallida.
* Sus actividades dependientes fueron abortadas.
* Las ramas independientes continuaron ejecutándose.
* El planificador terminó correctamente.

### Prueba de estrés

Se probaron planificaciones de:

```text
1000 actividades
10000 actividades
```

utilizando una cadena de dependencias.

Ambas pruebas terminaron correctamente sin que el planificador quedara bloqueado.

---

## 10. Compilación

Para compilar:

```bash
make
```

Para eliminar los archivos generados:

```bash
make clean
```

El proyecto utiliza compilación C17 con:

```text
gcc -Wall -Wextra -std=c17
```

y enlaza con:

```text
-lpthread
```

El programa no utiliza hilos (`threads`) ni mecanismos de sincronización de hilos.

---

## 11. Decisiones de diseño

Las principales decisiones de diseño fueron:

* Representar las actividades mediante un DAG utilizando índices del arreglo para relacionar padres y sucesores.
* Mantener `dependencias_restantes` para determinar cuándo una actividad queda habilitada.
* Utilizar una cola de actividades listas para controlar el orden de ejecución.
* Utilizar `fork()` para ejecutar cada actividad como un proceso independiente.
* Utilizar `waitpid()` de forma bloqueante para controlar la concurrencia sin *busy-waiting*.
* Utilizar pipes para comunicar la finalización de actividades a sus sucesores.
* Crear pipes cuando son necesarios y cerrar en cada proceso los descriptores que no utiliza.
* Utilizar una función recursiva para propagar fallos únicamente por la rama dependiente.
* Utilizar `sigaction()` para manejar la interrupción mediante `Ctrl+C`.
* Reservar dinámicamente el arreglo principal debido al tamaño de la estructura `Actividad`.

Estas decisiones permiten implementar los mecanismos solicitados de procesos, comunicación mediante pipes, control de concurrencia, señales y tolerancia a fallos.
