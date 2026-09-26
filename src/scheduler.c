#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/wait.h"
#include "sys/types.h"
#include "scheduler.h"

//aqui se define la estructura pare registrar que proceso hijo ejecuta que activdad
typedef struct {
pid_t pid;
int actividad_idx;
} ProcesoEnEjecucion;

int ejecutar_planificador(Actividad actividades[], int total, int K){
int listos[MAX_ACTIVIDADES];
int frente_cola = 0;
int fin_cola= 0;


//las actividades que no tengan dependencias se agregan a la cola de actividades listas
for (int i=0; i<total; i++){
    if(actividades[i].dependencias_restantes==0){
        listos[fin_cola++]=i;
    }
}

ProcesoEnEjecucion procesos_activos[K];
int num_procesos_activos=0;
int actividades_completadas=0;

printf("\n Planificador iniciado con K=%d\n", K);

//aqui definimos un bucle principal que se ejecuta mientras haya actividades pendientes o procesos activos

while(actividades_completadas<total){
    //ejecutamos las actividades en la cola de listos mientras haya espacio para los procesos activos y elementos en la cola de listos
 while(num_procesos_activos<K && frente_cola<fin_cola){
    int idx= listos[frente_cola++];
    
    pid_t pid= fork();

    if(pid<0){
        perror("Error al ejecutar el fork");
        return -1;
    }
    if(pid==0){
     printf ("Pid  del proceso hijo: %d, se encuentra ejecutando actividad %s (%d ms)\n", getpid(), actividades[idx].nombre, actividades[idx].tiempo);
      usleep(actividades[idx].tiempo*1000);
      printf("Pid del proceso hijo: %d, ha terminado la actividad %s\n", getpid(), actividades[idx].nombre);
      exit(0);

    } else{
        procesos_activos[num_procesos_activos].pid=pid;
        procesos_activos[num_procesos_activos].actividad_idx=idx;
        num_procesos_activos++;
    }
 }

//si tenemos procesos activos, esperamos a que alguno termine y actualizamos las dependencias de sus sucesores
if(num_procesos_activos>0){
int status;

pid_t pid_terminado= waitpid(-1,&status,0);

if(pid_terminado>0){
actividades_completadas++;


//buscamos cual actividad termino

int idx_terminado=-1;
int pos_array=-1;
for(int i=0;i<num_procesos_activos;i++){
    if(procesos_activos[i].pid==pid_terminado){
        idx_terminado = procesos_activos[i].actividad_idx;
        pos_array=i;
        break;
    }
}
//liberamos el espacio en el array de procesos activos
if(pos_array!=-1){
procesos_activos[pos_array]=procesos_activos[num_procesos_activos-1];
num_procesos_activos--;
}

//actualizamos las dependencias de los sucesores de la actividad que termino
if(idx_terminado!=-1){
Actividad *act_term=&actividades[idx_terminado];
for(int s =0; s<act_term->cant_sucesores; s++){
int idx_sucesor= act_term->sucesores[s];
actividades[idx_sucesor].dependencias_restantes--;

//si ya no tiene dependencias pendeintes se agrega a la cola de listos
if(actividades[idx_sucesor].dependencias_restantes==0){
    listos[fin_cola++]=idx_sucesor;
}
}
}
}
}
}
printf("\nPlanificador ha terminado de ejecutar todas las actividades.\n");
return 0; 
}