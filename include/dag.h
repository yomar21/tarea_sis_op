#ifndef DAG_H
#define DAG_H

#define MAX_ACTIVIDADES 10005
#define MAX_SUCESORES MAX_ACTIVIDADES
//ESTADOS DE UNA ACTIVIDAD
#define ESTADO_PENDIENTE 0
#define ESTADO_EJECUCION 1
#define ESTADO_COMPLETADA 2
#define ESTADO_FALLIDA 3


typedef struct {
    char id[32];
    char nombre[128];
    int tiempo;
    char deps[256];

    int total_deps; // Cantidad fija de insumos que el proceso hijo debe leer por el pipe
    int dependencias_restantes;// num para que el padre haga el fork
    int sucesores[MAX_SUCESORES];//para informr a los hijos
    int cant_sucesores;

    int estado;

    int pipe_fd[2];
} Actividad;

int buscar_actividad_por_id(const Actividad actividades[], int total_actividades, const char *id_buscado);
int construir_dag(Actividad actividades[], int total_actividades);
void imprimir_dag( const Actividad actividades[], int total_actividades);

#endif