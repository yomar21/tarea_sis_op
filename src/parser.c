#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "parser.h"

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

    *total_actividades = 0;
    char linea[256];

    while (fgets(linea, sizeof(linea), archivo) != NULL) {
        linea[strcspn(linea, "\r\n")] = '\0';
        if (strlen(linea) == 0) {
            continue;
        }

        char *token = linea;
        char *id     = trim(strsep(&token, ":"));
        char *nombre = trim(strsep(&token, ":"));
        char *tiempo = trim(strsep(&token, ":"));
        char *deps   = trim(strsep(&token, ":"));

        int idx = *total_actividades;

        strncpy(actividades[idx].id, id ? id : "", sizeof(actividades[idx].id) - 1);
        actividades[idx].id[sizeof(actividades[idx].id) - 1] = '\0';

        strncpy(actividades[idx].nombre, nombre ? nombre : "", sizeof(actividades[idx].nombre) - 1);
        actividades[idx].nombre[sizeof(actividades[idx].nombre) - 1] = '\0';

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

        // Inicializar contadores del grafo en cero
        actividades[idx].dependencias_restantes = 0;
        actividades[idx].cant_sucesores = 0;

        (*total_actividades)++;
    }

    fclose(archivo);
    return 0;
}