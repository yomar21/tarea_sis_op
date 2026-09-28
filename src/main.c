#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include "parser.h"
#include "dag.h"
#include "scheduler.h"
void maximizar_descriptores() {
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        rl.rlim_cur = rl.rlim_max;
        setrlimit(RLIMIT_NOFILE, &rl);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <archivo_plan> <K>\n", argv[0]);
        return 1;
    }

    int K = atoi(argv[2]);
    if(K <= 0){
        fprintf(stderr, "Error: El nivel de concurrencia K debe ser mayor a 0.\n");
        return 1;
    }

    maximizar_descriptores();
    srand(time(NULL));
    // Permite mostrar la salida inmediatamente.
    setbuf(stdout, NULL);

    Actividad *actividades = calloc(MAX_ACTIVIDADES,sizeof(Actividad));
    if (actividades == NULL) {
        fprintf(stderr, "Error: RAM insuficiente para alojar %d actividades.\n", MAX_ACTIVIDADES);
        return 1;
    }

    int total_actividades = 0;

    if (parsear_archivo(argv[1], actividades, &total_actividades) != 0) {
        free(actividades);
        return 1;
    }

    if (construir_dag(actividades, total_actividades) != 0) {
        free(actividades);
        return 1;
    }

    imprimir_dag(actividades, total_actividades);

    // Ejecuta el planificador.
    if (ejecutar_planificador(actividades, total_actividades, K) != 0) {
        free(actividades);
        return 1;
    }

    // Libera la memoria reservada para las actividades.
    free(actividades);

    return 0;
}   