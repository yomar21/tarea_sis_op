#ifndef DAG_H
#define DAG_H

#define MAX_ACTIVIDADES 100
#define MAX_SUCESORES 100

typedef struct {
    char id[32];
    char nombre[128];
    int tiempo;
    char deps[256];

    
    int dependencias_restantes;
    int sucesores[MAX_SUCESORES];
    int cant_sucesores;
} Actividad;

int buscar_actividad_por_id(const Actividad actividades[], int total_actividades, const char *id_buscado);
int construir_dag(Actividad actividades[], int total_actividades);
void imprimir_dag( const Actividad actividades[], int total_actividades);

#endif