#include "scheduler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "simulation.h"

//Variables globales


//FIFO(First In First Out)
int fifo_scheduler(proc_info_t *procs_info, int procs_count, int curr_time,
                   int curr_pid) {
  // Se devuelve el PID del primer proceso de todos los disponibles (los
  // procesos están ordenados por orden de llegada).
  return procs_info[0].pid;
}

//SJF(Shortest Job First)
int sjf_scheduler(proc_info_t *procs_info, int procs_count, int curr_time,
                     int curr_pid) {
  if(procs_count <= 0)
  {
    return -1;
  }

  int minTime = INT_MAX;
  int pid;
  int duration;
  int pidFinal;

  for(int var=0; var<procs_count; var++)
  {
    pid = procs_info[var].pid;
    duration = process_total_time(pid); 
    if(pid == curr_pid)
    {
      return pid;
    }
    if(duration < minTime)
    {
      minTime = duration;
      pidFinal = pid;
    }
  }

  return pidFinal;
}

//STCF(Shortest Time-to-Completion First)
int stcf_scheduler(proc_info_t *procs_info, int procs_count, int curr_time,
                     int curr_pid) {
  if(procs_count <= 0)
  {
    return -1;
  }

  int minTimeRemaining = INT_MAX;
  int pid;
  int remaining_time;
  int pidFinal;

  for(int var=0; var<procs_count; var++)
  {
    pid = procs_info[var].pid;
    remaining_time = process_total_time(pid)-procs_info[var].executed_time;
    if(remaining_time < minTimeRemaining)
    {
      minTimeRemaining = remaining_time;
      pidFinal = pid;
    }
  }

  return pidFinal;
}

//Posicion en el array 'procs_info' que tenia el ultimo proceso que se ejecuto
int pos_last = 0;

//RR(Round Robin)
int rr_scheduler(proc_info_t *procs_info, int procs_count, int curr_time,
                     int curr_pid) {
  if(procs_count <= 0)
  {
    return -1;
  }
  
  if(curr_pid != -1)
  {
    if(pos_last+1 >= procs_count)
    {
      pos_last = 0;
    }
    else
    {
      pos_last++;
    }
  }
  else
  {
    if (pos_last >= procs_count)
    {
      pos_last = 0;
    }
  }

  return procs_info[pos_last].pid;
}

/*-----------------------------MLFQ----------------------------*/

//Estructura que identifica a un nodo de la cola que queremos implementar para el MLFQ.
typedef struct Process {
  proc_info_t proc;
  struct Process *next;
  struct Process *previous;
  int isNull;
}Process;

/* 
  El objetivo de este MLFQ es llevar tres colas que identificarán los tres niveles de prioridad para este MLFQ. 
Para eso llevamos un array con los tres punteros que indican el inicio y el final de cada una de las colas. De esta forma podemos acceder a cada
uno de los niveles de prioridad y representar la prioridad de los procesos de una manera mas entendible
*/ 
Process* init[3] = {NULL, NULL, NULL};
Process* end[3] = {NULL, NULL, NULL};

//Variables para el MLFQ 
Process last_proc_exec;
int number_queue_last_proc = -1;
int time_priority_boost = 1000;

//Metodos necesarios:

//.Inserta un proceso a la cola especificada
void Insert(proc_info_t x, Process **root, Process **final)
{
  Process *new;
  new = (Process*)malloc(sizeof(Process));
  new->proc = x;
  new->next = NULL;
  new->previous = *final; 
  new->isNull = 0;
  if (*root == NULL)
  {
    *root = new;
    *final = new;
  }
  else
  {
    (*final)->next = new;
    *final = new;
  }
}

//.Obteniendo el proceso que esta en CPU
Process* Get_proc(Process *root)
{
  Process *bor = root;
  while(bor!=NULL && (bor->proc).on_io)
  {
    bor = bor->next;
  }

  return bor;
}
//.Extraer un proceso de la cola
Process Extract(Process **root,  Process **final, Process *bor)
{
  Process result;
  //Si no esta en la cola o todos los procesos estan en IO
  if(bor==NULL)
  {
    result.isNull = 1;
    return result;
  }
  
  if(bor == *root)//Si el proceso es el principio de la cola
  {
    if (*root == *final) //Un solo elemento en la cola
    {
      *root = NULL;
      *final = NULL;
    }
    else //Mas de un elemento en la cola
    {
      (bor->next)->previous = NULL;
      *root = (*root)->next;
    }
  }
  else if(bor == *final)//Si el proceso es el final de la cola
  {
    ((*final)->previous)->next = NULL;
    *final = (*final)->previous;
  }
  else //Si el proceso esta en el medio de la cola
  {
    (bor->previous)->next = bor->next;
    (bor->next)->previous = bor->previous;
  }

  result = *bor;
  free(bor);
  return result;
}

//.Buscar un proceso en una cola
int Search (proc_info_t proc) 
{
  Process *pointer;
  for(int i=0; i<3; i++)
  {
    pointer = init[i];
    while(pointer!=NULL)
    {
      if(proc.pid == (pointer->proc).pid) return 1;
      pointer = pointer->next;
    }
  }

  return 0;
}

//MLFQ (Multi-level Feedback Queue)
int mlfq_scheduler(proc_info_t *procs_info, int procs_count, int curr_time,
                     int curr_pid) {
  if(procs_count == 0) 
  {
    return -1;
  }

  printf("El curr_pid es: %d\n", curr_pid);
  printf("El procs_count es: %d\n", procs_count);
  //1.Procesos nuevos (insertar todos los procesos que entraron en este interrupt_time en la cola de mayor prioridad)              
  for(int k=0; k<procs_count; k++)
  {
    if(!Search(procs_info[k]))
    {
      Insert(procs_info[k], &init[0], &end[0]);
    }
  }

  //2.Bajarle la prioridad si el proceso que se ejecutó anteriormente no ha terminado 
  if(curr_pid != -1)
  {
    if(number_queue_last_proc < 2)
    {
      Insert(last_proc_exec.proc, &init[number_queue_last_proc+1], &end[number_queue_last_proc+1]);
    }
    else{
    Insert(last_proc_exec.proc, &init[2], &end[2]);
    }
  }

  //3.Poniendo todos los procesos en la cola de mayor prioridad cada un cierto tiempo: 'time_priority_boost'   
  if(curr_time%time_priority_boost == 0)
  {
    for(int j=2; j>0; j--)
    {
      while(init[j]!=NULL)
      {
        Process change_priority = Extract(&init[j], &end[j], init[j]);
        Insert(change_priority.proc, &init[0], &end[0]);
      }
    }
  }

  //4.Extraer el proceso de mayor prioridad
  int i=0;
  do{
    last_proc_exec = Extract(&init[i], &end[i], Get_proc(init[i]));
    number_queue_last_proc = i;
    i++;
  }while(last_proc_exec.isNull && i<3);
  
  
  //5.Ejecutar 
  if(last_proc_exec.isNull)
  {
    // number_queue_last_proc = -1;
    return -1;
  } 

  printf("Proceso a ejecutar: %d\n", (last_proc_exec.proc).pid);
  printf("La cola es: %d\n", number_queue_last_proc);
  return (last_proc_exec.proc).pid;
}

// Esta función devuelve la función que se ejecutará en cada timer-interrupt
// según el nombre del scheduler.
schedule_action_t get_scheduler(const char *name) {
  // Si necesitas inicializar alguna estructura antes de comenzar la simulación
  // puedes hacerlo aquí.

  if (strcmp(name, "fifo") == 0) return *fifo_scheduler;

  if (strcmp(name, "sjf") == 0) return *sjf_scheduler;
  
  if (strcmp(name, "stcf") == 0) return *stcf_scheduler;

  if (strcmp(name, "rr") == 0) return *rr_scheduler;

  if (strcmp(name, "mlfq") == 0) return *mlfq_scheduler;

  fprintf(stderr, "Invalid scheduler name: '%s'\n", name);
  exit(1);
}