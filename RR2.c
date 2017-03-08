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
struct queue *cola; // Cola para tratar los procesos.
static TCB* previo; // Hilo que que se expulsa en un cambio de contexto.


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

    cola = queue_new(); // Inicializamos la cola de procesos.
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
    disable_interrupt();
    enqueue(cola, &t_state[i]);
    enable_interrupt();

    return i;
} /****** End my_thread_create() ******/

/* Free terminated thread and exits */
void mythread_exit() {
    int tid = mythread_gettid();

    printf("*** THREAD %d FINISHED\n", tid);
    t_state[tid].state = FREE;
    //memcpy(previo, &t_state[tid], sizeof(TCB));
    free(t_state[tid].run_env.uc_stack.ss_sp); 

    previo = &t_state[tid]; // Se guarda el hilo que ha terminado.
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

/*
 * Timer interrupt. Aqui se implementa el planificador RR.
 */
void timer_interrupt(int sig) {
    // Reducimos los ticks.
    running->ticks--;

    // Comprobamos si la rodaja ha terminado.
    if (running->ticks <= 0) {
        // Re-establecemos los datos del proceso.
        running->ticks = QUANTUM_TICKS;
        
        // Pedimos el siguiente proceso y guardamos este.
        TCB *next = scheduler();
        previo = running;

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

    // Ver si la cola esta vacía.
    disable_interrupt();
    int cola_vacia = queue_empty(cola);
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
    if (previo->state == FREE) {
        printf("*** THREAD %d FINISHED: SET CONTEXT OF %d\n", previo->tid, next->tid);
        // Establecemos el nuevo proceso como en funcionamiento.
        running = next;
        current = next->tid;
        setcontext(&(next->run_env));

        printf("mythread_free: After setcontext, should never get here!!...\n");
    } else {
        printf("*** SWAPCONTEXT FROM %d to %d\n", previo->tid, next->tid);
        // Establecemos el nuevo proceso como en funcionamiento.
        running = next;
        current = next->tid;
        swapcontext(&(previo->run_env), &(next->run_env));
    }
}