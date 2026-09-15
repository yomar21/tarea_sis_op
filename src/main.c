#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "parser.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <archivo_plan> [K]\n", argv[0]);
        return 1;
    }

    srand(time(NULL));

    Actividad actividades[MAX_ACTIVIDADES];
    int total_actividades = 0;

    if (parsear_archivo(argv[1], actividades, &total_actividades) != 0) {
        return 1;
    }

    if (construir_dag(actividades, total_actividades) != 0) {
        return 1;
    }

    imprimir_dag(actividades, total_actividades);

    return 0;
}