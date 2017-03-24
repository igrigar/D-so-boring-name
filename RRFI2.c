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

long hungry = 0L; // Supongo que para saber si un hilo esta hambriento,
                  // no es útil en este caso.
static TCB t_state[N]; // Array de TCBs.
static TCB* running; // Hilo que se encuentra en estado de ejecución.
static int current = 0; // ID del hilo que se esta ejecutando.
static int init = 0; // Indica si la libreria 'mythread' ha sido iniciada.


// VARIABLES PROPIAS.
struct queue *cola; // Cola para tratar los procesos no prioritarios.
struct queue *prio; // Cola para tratar los procesos prioritarios.


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
    running = &t_state[0];

    cola = queue_new(); // Inicializamos la cola de procesos no prioritarios.
    prio = queue_new(); // Inicializamos la cola de procesos prioritarios.
    running = &t_state[0]; // Ponemos el hilo 'testigo' a ejecutar.

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

    t_state[i].ticks = QUANTUM_TICKS; // Se necesita para las rodajas de tiempo.
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

    /* Añadir el nuevo hilo a la cola. */
    if (priority == HIGH_PRIORITY) {
        if (running->priority == HIGH_PRIORITY) {
            disable_interrupt();
            enqueue(prio, &t_state[i]);
            enable_interrupt();
        } else {
            running->ticks = QUANTUM_TICKS;
            disable_interrupt(); 
            enqueue(cola, running);
            enable_interrupt(); 
            activator(&t_state[i]);
        }
    } else {
        disable_interrupt();
        enqueue(cola, &t_state[i]);
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


    TCB* next = scheduler(); // Se pide al scheduler un nuevo hilo.
    activator(next); // Se cambia el contexto.
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

/* Timer interrupt. Aqui se implementa el planificador RR. */
void timer_interrupt(int sig) {

    /* Comprobamos que las colas esten vacias */
    disable_interrupt();
    int fifo_vacia = queue_empty(prio);
    int rr_vacia = queue_empty(cola);
    enable_interrupt();

    if (!fifo_vacia && !rr_vacia) ++hungry; // Ambas colas tienen elementos.
    else hungry = 0L; // Una de las dos esta vacia.

    if (hungry >= STARVATION)
        while (!rr_vacia) {
            TCB *t;
            disable_interrupt();
            t = dequeue(cola);
            enqueue(prio, t);
            rr_vacia = queue_empty(cola);
            enable_interrupt();
            printf("*** THREAD %d PROMOTED TO HIGH PRIORITY QUEUE\n",t->tid);
        }

    if (running->priority == HIGH_PRIORITY) return;

    // Reducimos los ticks.
    running->ticks--;

    // Comprobamos si la rodaja ha terminado.
    if (running->ticks <= 0) {
        // Re-establecemos los datos del proceso.
        running->ticks = QUANTUM_TICKS;

        disable_interrupt();
        int cola_vacia = queue_empty(cola);
        int cola_vacia_prio = queue_empty(prio);
        enable_interrupt();

        if (cola_vacia && cola_vacia_prio) return; // Ningún candidato de ejecución.

        // Pedimos el siguiente proceso y guardamos este.
        TCB *next = scheduler();

        // Encolamos el proceso.
        disable_interrupt();
        enqueue(cola, running);
        enable_interrupt();

        // Realizamos el cambio de contexto.
        activator(next);
    }
}

/* Scheduler: returns the next thread to be executed */
TCB* scheduler() {

    int cola_vacia;

    // Caso en el que hay threads prioritarias.
    // Ver si la cola esta vacía.
    disable_interrupt();
    cola_vacia = queue_empty(prio);
    enable_interrupt();

    if (!cola_vacia) {
        disable_interrupt();
        TCB *siguiente = dequeue(prio);
        enable_interrupt();

        return siguiente;
    }

    // No hay threads prioritarias.
    disable_interrupt();
    cola_vacia = queue_empty(cola);
    enable_interrupt();

    if (cola_vacia) {
        // Hay un proceso en ejecución:
        if (running->state == INIT)
            return running;
        else { // El último proceso en ejecución ya ha terminado.
            printf("FINISH\n");
            exit(1);
        }
    } else {
        // Obtener el siguiente elemento de la cola.
        disable_interrupt();
        TCB *siguiente = dequeue(cola);
        enable_interrupt();

        return siguiente;
    }
}

/* Activator */
void activator(TCB* next) {
    if (running->state == FREE) {
        printf("*** THREAD %d FINISHED: SET CONTEXT OF %d\n", running->tid, next->tid);
        // Establecemos el nuevo proceso como en funcionamiento.
        running = next;
        current = next->tid;
        setcontext(&(running->run_env));

        printf("mythread_free: After setcontext, should never get here!!...\n");
    } else if (running->priority == LOW_PRIORITY &&
            next->priority == HIGH_PRIORITY) {
        TCB* previo = running;
        printf("*** THREAD %d EJECTED: SET CONTEXT OF %d", previo->tid, next->tid);
        running = next;
        current = next->tid;
        swapcontext(&(previo->run_env), &(next->run_env));
    } else {
        TCB* previo = running;
        printf("*** SWAPCONTEXT FROM %d to %d\n", previo->tid, next->tid);
        // Establecemos el nuevo proceso como en funcionamiento.
        running = next;
        current = next->tid;
        swapcontext(&(previo->run_env), &(next->run_env));
     }
} 
