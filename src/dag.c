#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include "dag.h"

static char *trim_local(char *str) {
    if (str == NULL) return NULL;
    while (isspace((unsigned char)*str)) {
        str++;
    }
    if (*str == '\0') {
        return str;
    }
    char *fin = str + strlen(str) - 1;
    while (fin > str && isspace((unsigned char)*fin)) {
        *fin = '\0';
        fin--;
    }
    return str;
}
int buscar_actividad_por_id(const Actividad actividades[], int total, const char *id_buscado){
for(int i=0; i<total; i++){
if(strcmp(actividades[i].id, id_buscado)==0){
    return i;
}

}
return -1;

}

int construir_dag(Actividad actividades[], int total){
    for (int i=0; i<total; i++){

        // Inicializa los valores necesarios para cada actividad.
        actividades[i].total_deps = 0;
        actividades[i].estado = ESTADO_PENDIENTE;
        actividades[i].pipe_fd[0] = -1;
        actividades[i].pipe_fd[1] = -1;

        if(strlen(actividades[i].deps)==0){
            continue;
        }

        char buffer_deps[256];
        strncpy(buffer_deps, actividades[i].deps, sizeof(buffer_deps)-1);
        buffer_deps[sizeof(buffer_deps)-1] = '\0';

        char *dep_raw = strtok(buffer_deps, ",");
        while(dep_raw != NULL){
            char *dep = trim_local(dep_raw);
            if(dep == NULL || strlen(dep) == 0){
                dep_raw = strtok(NULL, ",");
                continue;
            }

            int idx_padre= buscar_actividad_por_id(actividades, total, dep);
            if(idx_padre==-1){
                fprintf(stderr, "Error: Dependencia '%s' no encontrada para la actividad '%s'\n", dep, actividades[i].id);
                return -1;
            }

            actividades[i].dependencias_restantes++;
            
            if (actividades[idx_padre].cant_sucesores < MAX_SUCESORES) {
                int pos = actividades[idx_padre].cant_sucesores;
                actividades[idx_padre].sucesores[pos] = i;
                actividades[idx_padre].cant_sucesores++;
            } else {
                fprintf(stderr, "Error: la actividad '%s' supero el limite de sucesores\n",
                        actividades[idx_padre].id);
                return -1;
            }
            dep_raw = strtok(NULL, ",");
        }
        actividades[i].total_deps = actividades[i].dependencias_restantes;
    }
    return 0;
}

void imprimir_dag(const Actividad actividades[], int total) {
    printf("\n=== Estructura del Grafo (DAG) ===\n");
    for (int i = 0; i < total; i++) {
        printf("[%d] Actividad '%s' (%s):\n", i, actividades[i].id, actividades[i].nombre);
        printf("    - Dependencias pendientes: %d\n", actividades[i].dependencias_restantes);
        printf("    - Desbloquea a (%d sucesores): ", actividades[i].cant_sucesores);

        if (actividades[i].cant_sucesores == 0) {
            printf("Ninguno (nodo final)");
        } else {
            for (int s = 0; s < actividades[i].cant_sucesores; s++) {
                int idx_sucesor = actividades[i].sucesores[s];
                printf("['%s'] ", actividades[idx_sucesor].id);
            }
        }
        printf("\n");
    }
}




    
