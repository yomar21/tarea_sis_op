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
//revisar si se puede usar un sizre
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
        if(strlen(actividades[i].deps)==0){
            continue;
        }

        char buffer_deps[256];
        strncpy(buffer_deps, actividades[i].deps, sizeof(buffer_deps)-1);
        buffer_deps[sizeof(buffer_deps)-1] = '\0';

        char *ptr= buffer_deps;
        while(ptr!=NULL){
            char *dep= trim_local(strsep(&ptr, ","));
            if(dep==NULL || strlen(dep)==0){
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
        }
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




    
