#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "parser.h"
#include "dag.h"

static char *trim(char *str) {
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

int parsear_archivo(const char *ruta, Actividad actividades[], int *total_actividades) {
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        return -1;
    }

    srand(time(NULL));

    *total_actividades = 0;
    char linea[512];

    while (fgets(linea, sizeof(linea), archivo) != NULL) {
        linea[strcspn(linea, "\r\n")] = '\0';
        if (strlen(linea) == 0) {
            continue;
        }
        // Verifica que no se supere el limite de actividades.
        if (*total_actividades >= MAX_ACTIVIDADES) {
            fprintf(stderr, "Error: Se supero el limite maximo de actividades (%d).\n", MAX_ACTIVIDADES);
            fclose(archivo);
            return -1;
        }

        // Se separan los campos usando ':'.
        char *id = linea;
        char *nombre = NULL, *tiempo = NULL, *deps = NULL;

        char *p1 = strchr(id, ':');
        if (p1) {
            *p1 = '\0';
            nombre = p1 + 1;

            char *p2 = strchr(nombre, ':');
            if (p2) {
                *p2 = '\0';
                tiempo = p2 + 1;

                char *p3 = strchr(tiempo, ':');
                if (p3) {
                    *p3 = '\0';
                    deps = p3 + 1;
                }
            }
        }

        // Limpiamos los espacios en blanco.
        id = trim(id);
        nombre = trim(nombre);
        tiempo = trim(tiempo);
        deps = trim(deps);

        int idx = *total_actividades;

        strncpy(actividades[idx].id, id ? id : "", sizeof(actividades[idx].id) - 1);
        actividades[idx].id[sizeof(actividades[idx].id) - 1] = '\0';

        strncpy(actividades[idx].nombre, nombre ? nombre : "", sizeof(actividades[idx].nombre) - 1);
        actividades[idx].nombre[sizeof(actividades[idx].nombre) - 1] = '\0';

        // Usa el tiempo indicado o genera uno aleatorio si el campo esta vacio.
        if (tiempo != NULL && strlen(tiempo) > 0) {
            actividades[idx].tiempo = atoi(tiempo);
        } else {
            actividades[idx].tiempo = 100 + rand() % (5000 - 100 + 1);
        }

        if (deps != NULL && strlen(deps) > 0) {
            strncpy(actividades[idx].deps, deps, sizeof(actividades[idx].deps) - 1);
            actividades[idx].deps[sizeof(actividades[idx].deps) - 1] = '\0';
        } else {
            actividades[idx].deps[0] = '\0';
        }

        // Inicializa los contadores del grafo.
        actividades[idx].dependencias_restantes = 0;
        actividades[idx].cant_sucesores = 0;

        (*total_actividades)++;
    }


    fclose(archivo);
    return 0;
}