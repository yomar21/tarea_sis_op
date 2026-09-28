#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/wait.h"
#include "sys/types.h"
#include <signal.h>
#include <string.h>
#include "scheduler.h"
#include <errno.h>

volatile sig_atomic_t seremi_llego = 0;
void manejador_seremi(int sig) {
    (void)sig; // Para evitar warnings del compilador
    seremi_llego = 1;
}

void abortar_rama_recursivo(Actividad actividades[], int idx_fallido, int *completadas) {
    if (actividades[idx_fallido].estado == ESTADO_FALLIDA) return; // Ya estaba cancelada

    actividades[idx_fallido].estado = ESTADO_FALLIDA;
    (*completadas)++; // Sumamos para que el bucle while principal no se quede infinito

    printf("   [ABORTADA] Actividad '%s' cancelada por fallo en dependencias.\n", actividades[idx_fallido].id);

    // Propagamos la cancelación a todos sus hijos
    for (int i = 0; i < actividades[idx_fallido].cant_sucesores; i++) {
        int hijo_idx = actividades[idx_fallido].sucesores[i];
        abortar_rama_recursivo(actividades, hijo_idx, completadas);
    }
}

// Aqui se define la estructura para registrar que proceso hijo ejecuta que actividad
typedef struct {
    pid_t pid;
    int actividad_idx;
} ProcesoEnEjecucion;

int ejecutar_planificador(Actividad actividades[], int total, int K) {
    struct sigaction sa;
    sa.sa_handler = manejador_seremi;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, 0);

    int listos[MAX_ACTIVIDADES];
    int frente_cola = 0;
    int fin_cola = 0;

    // Las actividades que no tengan dependencias se agregan a la cola de actividades listas
    for (int i = 0; i < total; i++) {
        if (actividades[i].dependencias_restantes == 0) {
            listos[fin_cola++] = i;
        }
    }

    ProcesoEnEjecucion procesos_activos[K];
    int num_procesos_activos = 0;
    int actividades_completadas = 0;

    printf("\n Planificador iniciado con K=%d\n", K);

    // Bucle principal: se ejecuta mientras haya actividades pendientes o procesos activos
    while (actividades_completadas < total) {

        // Si apreta Ctrl+C
        if (seremi_llego) {
            printf("\n>>> Llego la inspeccion de la SEREMI (SIGINT): abortando todas las actividades...\n");

            for (int p = 0; p < num_procesos_activos; p++) {
                kill(procesos_activos[p].pid, SIGTERM);
            }

            for (int p = 0; p < num_procesos_activos; p++) {
                waitpid(procesos_activos[p].pid, NULL, 0);
            }

            printf("\nPlanificador abortado por SIGINT.\n");
            return 0;
        }

        // Ejecutamos las actividades en la cola de listos
        while (num_procesos_activos < K && frente_cola < fin_cola) {
            int idx = listos[frente_cola++];

            if (actividades[idx].estado == ESTADO_FALLIDA) continue;

            // Crea los pipes de los sucesores cuando la actividad está lista para ejecutarse.
            for (int s = 0; s < actividades[idx].cant_sucesores; s++) {
                int suc_idx = actividades[idx].sucesores[s];
                if (actividades[suc_idx].pipe_fd[0] == -1) {
                    if (pipe(actividades[suc_idx].pipe_fd) == -1) {
                        perror("Error al crear pipe JIT");
                        return -1;
                    }
                }
            }

            pid_t pid = fork();

            if (pid < 0) {
                perror("Error al ejecutar el fork");
                return -1;
            }

            if (pid == 0) {
                // Cerrar los descriptores heredados que este proceso no necesita.
                for (int k = 0; k < total; k++) {

                    if (actividades[k].pipe_fd[0] != -1 && k != idx) {
                        close(actividades[k].pipe_fd[0]);
                    }

                    int es_mi_sucesor = 0;

                    for (int s = 0; s < actividades[idx].cant_sucesores; s++) {
                        if (actividades[idx].sucesores[s] == k) {
                            es_mi_sucesor = 1;
                            break;
                        }
                    }

                    if (actividades[k].pipe_fd[1] != -1 && !es_mi_sucesor) {
                        close(actividades[k].pipe_fd[1]);
                    }
                }

                // ESPERAR INSUMOS
                for (int j = 0; j < actividades[idx].total_deps; j++) {
                    char buffer[256] = {0};
                    ssize_t recibidos = read(actividades[idx].pipe_fd[0], buffer, sizeof(buffer));

                    if (recibidos == -1) {
                        perror("Error al recibir mensaje por pipe");
                        exit(EXIT_FAILURE);
                    }

                    if (recibidos == 0) {
                        fprintf(stderr, "Error: el pipe se cerro antes de recibir el insumo.\n");
                        exit(EXIT_FAILURE);
                    }

                    printf("    <- '%s' recibe insumo: %s\n", actividades[idx].nombre, buffer);
                }

                printf("Pid  del proceso hijo: %d, se encuentra ejecutando actividad %s (%d ms)\n", getpid(), actividades[idx].nombre, actividades[idx].tiempo);
                usleep(actividades[idx].tiempo * 1000);


                // AVISAR A SUCESORES
                char msg[256] = {0};
                snprintf(msg, sizeof(msg), "insumo de '%s' (pid %d) listo", actividades[idx].nombre, getpid());
                for (int i = 0; i < actividades[idx].cant_sucesores; i++) {
                    int suc_idx = actividades[idx].sucesores[i];
                    ssize_t escritos = write(actividades[suc_idx].pipe_fd[1], msg, sizeof(msg));

                    if (escritos != sizeof(msg)) {
                        perror("Error al enviar mensaje por pipe");
                        exit(EXIT_FAILURE);
                    }
                }
                printf("Pid del proceso hijo: %d, ha terminado la actividad %s\n", getpid(), actividades[idx].nombre);

                exit(EXIT_SUCCESS);

            } else {
                actividades[idx].estado = ESTADO_EJECUCION;
                if (actividades[idx].pipe_fd[0] != -1) {
                    close(actividades[idx].pipe_fd[0]);
                    actividades[idx].pipe_fd[0] = -1;
                }

                if (actividades[idx].pipe_fd[1] != -1) {
                    close(actividades[idx].pipe_fd[1]);
                    actividades[idx].pipe_fd[1] = -1;
                }
                procesos_activos[num_procesos_activos].pid = pid;
                procesos_activos[num_procesos_activos].actividad_idx = idx;
                num_procesos_activos++;
            }
        }

        // Si tenemos procesos activos, esperamos a que alguno termine
        if (num_procesos_activos > 0) {
            int status;
            pid_t pid_terminado = waitpid(-1, &status, 0);

            if (pid_terminado == -1) {
                if (errno == EINTR) {
                    continue;
                }

                perror("Error en waitpid");
                return -1;
            }

            if (pid_terminado > 0) {
                // Buscamos cuál actividad terminó
                int idx_terminado = -1;
                int pos_array = -1;
                for (int i = 0; i < num_procesos_activos; i++) {
                    if (procesos_activos[i].pid == pid_terminado) {
                        idx_terminado = procesos_activos[i].actividad_idx;
                        pos_array = i;
                        break;
                    }
                }

                // Liberamos el espacio en el array de procesos activos
                if (pos_array != -1) {
                    procesos_activos[pos_array] = procesos_activos[num_procesos_activos - 1];
                    num_procesos_activos--;
                }

                // Actualizamos dependencias
                if (idx_terminado != -1) {
                    Actividad *act_term = &actividades[idx_terminado];

                    // AISLAMIENTO DE ERRORES
                    if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) {
                        printf("[ERROR] Actividad %s fallo internamente.\n", act_term->nombre);
                        abortar_rama_recursivo(actividades, idx_terminado, &actividades_completadas);
                    }else if (WIFSIGNALED(status)) {
                        printf("[ERROR] Actividad %s termino por una señal (%d).\n",
                               act_term->nombre, WTERMSIG(status));
                        abortar_rama_recursivo(actividades, idx_terminado, &actividades_completadas);
                    }
                    // ÉXITO
                    else if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
                        actividades_completadas++;
                        act_term->estado = ESTADO_COMPLETADA;
                        for (int s = 0; s < act_term->cant_sucesores; s++) {
                            int idx_sucesor = act_term->sucesores[s];
                            actividades[idx_sucesor].dependencias_restantes--;

                            // Agregar la actividad a la cola cuando todas sus dependencias terminaron.
                            if (actividades[idx_sucesor].dependencias_restantes == 0 && actividades[idx_sucesor].estado != ESTADO_FALLIDA) {
                                listos[fin_cola++] = idx_sucesor;
                            }
                        }
                    }
                }
            }
        }
    }
    printf("\nPlanificador ha terminado de ejecutar todas las actividades.\n");
    return 0;
}   