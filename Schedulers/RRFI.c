#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>

#include "mythread.h"
#include "interrupt.h"
#include "queue.h"

/******************************************************************************
 *                           CABECERAS DE FUNCIONES                           *
 ******************************************************************************/
TCB* scheduler();
void activator();
void timer_interrupt(int sig);

/******************************************************************************
 *                             VARIABLES GLOBALES                             *
 ******************************************************************************/
 long hungry = 0L;       /* Variable para saber si un proceso esta hambriento */
 static TCB t_state[N];  /* Array de TCBs */
 static TCB* running;    /* Hilo que se encuentra en estado de ejecución */
 static int current = 0; /* ID del hilo que se esta ejecutando */
 static int init = 0;    /* Indica si la libreria 'mythread' ha sido iniciada */

 /******************************************************************************
   *                             VARIABLES PROPIAS                           *
   ******************************************************************************/
 struct queue *cola_LOW;  /* Cola para tratar los procesos no prioritarios */
 struct queue *cola_HIGH; /* Cola para tratar los procesos prioritarios */


/*
 * Inicializar la lista de TCBs, para ello se introduce un hilo 'testigo' que no
 * hace nada. También se ponen todos los hilos con estado 'FREE'.
 */
void init_mythreadlib() {
    int i;
    t_state[0].state = INIT;
    t_state[0].priority = LOW_PRIORITY;
    t_state[0].ticks = QUANTUM_TICKS;
    if(getcontext(&t_state[0].run_env) == -1){
        perror("getcontext in my_thread_create");
        exit(5);
    }
    for(i=1; i<N; i++){
        t_state[i].state = FREE;
    }
    t_state[0].tid = 0;

    cola_LOW = queue_new();     /* Inicialización de la cola de procesos no prioritarios */
    cola_HIGH = queue_new();    /* Inicialización de la cola de procesos prioritarios */
    running = &t_state[0];      /* Ponemos el hilo 'testigo' a ejecutar */

    init_interrupt();
}

/* Create and intialize a new thread with body fun_addr and one integer argument */
int mythread_create (void (*fun_addr)(), int priority) {
    int i;

    if (!init) { init_mythreadlib(); init=1;}
    for (i=0; i<N; i++)
        if (t_state[i].state == FREE) break;
    if (i == N) return(-1);
    if(getcontext(&t_state[i].run_env) == -1){
        perror("getcontext in my_thread_create");
        exit(-1);
    }

    t_state[i].ticks = QUANTUM_TICKS; /* Numero de ticks de un proceso */
    t_state[i].state = INIT;
    t_state[i].priority = priority;
    t_state[i].function = fun_addr;
    t_state[i].run_env.uc_stack.ss_sp = (void *)(malloc(STACKSIZE));
    if(t_state[i].run_env.uc_stack.ss_sp == NULL){
        printf("thread failed to get stack space\n");
        exit(-1);
    }

    t_state[i].tid = i;
    t_state[i].run_env.uc_stack.ss_size = STACKSIZE;
    t_state[i].run_env.uc_stack.ss_flags = 0;
    makecontext(&t_state[i].run_env, fun_addr, 1);

    /* Añadir el nuevo proceso a la cola */
    /* Si el nuevo proceso es prioritario */
    if (priority == HIGH_PRIORITY) {
        /* Y el actual es prioritario, se encola en los prioritarios */
        if (running->priority == HIGH_PRIORITY) {
            disable_interrupt();
            enqueue(cola_HIGH, &t_state[i]);
            enable_interrupt();
        }
        /* Si no, se expulsa al no prioritario */
        else {
            running->ticks = QUANTUM_TICKS;
            disable_interrupt();
            enqueue(cola_LOW, running);
            enable_interrupt();
            activator(&t_state[i]);
        }
    }
    /* Si el nuevo proceso no es prioritario, se encola es su respectiva cola */
    else {
        disable_interrupt();
        enqueue(cola_LOW, &t_state[i]);
        enable_interrupt();
    }

    return i;
} /****** End my_thread_create() ******/

/* Free terminated thread and exits */
void mythread_exit() {
    int tid = mythread_gettid();

    printf("*** THREAD %d FINISHED\n", tid);
    t_state[tid].state = FREE;
    free(t_state[tid].run_env.uc_stack.ss_sp);


    TCB* next = scheduler(); /* Se pide al scheduler un nuevo proceso */
    activator(next); /* Se cambia el contexto */
}

/* Sets the priority of the calling thread */
void mythread_setpriority(int priority) {
    int tid = mythread_gettid();
    t_state[tid].priority = priority;
}

/* Returns the priority of the calling thread */
int mythread_getpriority(int priority) {
    int tid = mythread_gettid();
    return t_state[tid].priority;
}

/* Get the current thread id.  */
int mythread_gettid() {
    if (!init) {init_mythreadlib(); init=1;}
    return current;
}

/* Timer interrupt. Aqui se implementa el planificador RR */
void timer_interrupt(int sig) {
    /* Comprobamos que las colas esten vacias */
    disable_interrupt();
    int FIFO_vacia = queue_empty(cola_HIGH);
    int RR_vacia = queue_empty(cola_LOW);
    enable_interrupt();

    /* Si ambas colas tienen elementos, aumentamos el valor de hungry */
    if (!FIFO_vacia && !RR_vacia) ++hungry;
    /* Si no, su valor se reinicia */
    else hungry = 0L;

    /* Si el valor de hungry es mayor que el definido en STARVATION */
    if (hungry >= STARVATION){
        /* Mientras la cola de procesos no prioritarios no este vacia */
        while (!RR_vacia) {
            TCB *aux;
            disable_interrupt();
            /* Se desencolan los procesos de la cola no prioritaria */
            aux = dequeue(cola_LOW);
            /* Y se encolan en la cola prioritaria */
            enqueue(cola_HIGH, aux);
            /* Comprobamos el nuevo valor de la cola */
            RR_vacia = queue_empty(cola_LOW);
            enable_interrupt();
            printf("*** THREAD %d PROMOTED TO HIGH PRIORITY QUEUE\n",aux->tid);
        }
    }

    /* Si el proceso es priotario, se ejecuta hasta el final */
    if (running->priority == HIGH_PRIORITY) return;

    /* Si no es priotario, reducimos los ticks */
    running->ticks--;

    /* Comprobamos si la rodaja ha terminado */
    if (running->ticks <= 0) {
        /* Re-establecemos los datos del proceso */
        running->ticks = QUANTUM_TICKS;

        /* Pedimos el siguiente proceso y guardamos este */
        TCB *next = scheduler();
        if (next->tid == running->tid) {
            return; /* Mismo proceso */
        }

        /* Encolamos el proceso */
        disable_interrupt();
        enqueue(cola_LOW, running);
        enable_interrupt();

        /* Realizamos el cambio de contexto */
        activator(next);
    }
}

/* Scheduler: returns the next thread to be executed */
TCB* scheduler() {
    /* Comprobamos si la cola prioritaria esta vacia */
    disable_interrupt();
    int cola_vacia_HIGH = queue_empty(cola_HIGH);
    enable_interrupt();

    /* Si no esta vacia, devolvemos el siguiente proceso prioritario */
    if (!cola_vacia_HIGH) {
        disable_interrupt();
        TCB *next = dequeue(cola_HIGH);
        enable_interrupt();

        return next;
    }

    /* En caso de no haber procesos prioritarios, comprobamos la no prioritaria */
    disable_interrupt();
    int cola_vacia_LOW = queue_empty(cola_LOW);
    enable_interrupt();

    /* Si la cola no prioritaria esta vacia */
    if (cola_vacia_LOW) {
        /* En caso de que haya un proceso en ejecucion, se devuelve el proceso hasta su finalizacion */
        if (running->state == INIT)
            return running;
        /* Si el ultimo proceso ya ha terminado, finaliza el programa */
        else {
            printf("FINISH\n");
            exit(1);
        }
    /* Si la cola no prioritaria no esta vacia */
    } else {
        /* Obtenemos el siguiente elemento de la cola */
        disable_interrupt();
        TCB *next = dequeue(cola_LOW);
        enable_interrupt();

        return next;
    }
}

/* Activator */
void activator(TCB* next) {
    /* Si el proceso ha finalizado, se establece el nuevo proceso */
    if (running->state == FREE) {
        printf("*** THREAD %d FINISHED: SET CONTEXT OF %d\n", running->tid, next->tid);
        /* Establecemos el nuevo proceso como en funcionamiento */
        running = next;
        current = next->tid;
        setcontext(&(running->run_env));
        printf("mythread_free: After setcontext, should never get here!!...\n");
    }
    /* Si un proceso no prioritario ha sido expulsado, guardamos su estado para reestablecerlo posteriormente */
    else if (running->priority == LOW_PRIORITY && next->priority == HIGH_PRIORITY) {
        TCB* previo = running;
        printf("*** THREAD %d EJECTED: SET CONTEXT OF %d\n", previo->tid, next->tid);
        running = next;
        current = next->tid;
        swapcontext(&(previo->run_env), &(next->run_env));
    }
    /* Si aun no ha finalizado, guardamos su estado para reestablecerlo posteriormente */
    else {
        TCB* previo = running;
        printf("*** SWAPCONTEXT FROM %d to %d\n", previo->tid, next->tid);
        /* Establecemos el nuevo proceso como en funcionamiento */
        running = next;
        current = next->tid;
        swapcontext(&(previo->run_env), &(next->run_env));
     }
}
