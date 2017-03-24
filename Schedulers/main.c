#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>

#include "mythread.h"


void quantum_1 (int global_index) {
    int a,b;
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    mythread_exit();
    return;
 }

// Threads de alta prioridad creadas a parte, bcs necesrio.
void thread_high (int global_index) {
    if(mythread_create(quantum_1 ,HIGH_PRIORITY) == -1){
        printf("thread failed to initialize\n");
        exit(-1);
    }
    if(mythread_create(quantum_1 ,HIGH_PRIORITY) == -1){
        printf("thread failed to initialize\n");
        exit(-1);
    }

    if(mythread_create(quantum_1 ,HIGH_PRIORITY) == -1){
        printf("thread failed to initialize\n");
        exit(-1);
    }
    mythread_exit();
    return;
 }


int main(int argc, char *argv[]) {
    int a,b;

    mythread_setpriority(HIGH_PRIORITY);

    if(mythread_create(quantum_1 ,LOW_PRIORITY) == -1){
        printf("thread failed to initialize\n");
        exit(-1);
    }
    if(mythread_create(quantum_1 ,LOW_PRIORITY) == -1){
        printf("thread failed to initialize\n");
        exit(-1);
    }
    if(mythread_create(quantum_1 ,LOW_PRIORITY) == -1){
        printf("thread failed to initialize\n");
        exit(-1);
    }
    if(mythread_create(quantum_1 ,LOW_PRIORITY) == -1){
        printf("thread failed to initialize\n");
        exit(-1);
    }
    if(mythread_create(quantum_1 ,LOW_PRIORITY) == -1){
        printf("thread failed to initialize\n");
        exit(-1);
    }
    if(mythread_create(quantum_1 ,LOW_PRIORITY) == -1){
        printf("thread failed to initialize\n");
        exit(-1);
    }
    if(mythread_create(quantum_1 ,HIGH_PRIORITY) == -1){
        printf("thread failed to initialize\n");
        exit(-1);
    }

    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    for (a=0; a<10; ++a) for (b=0; b<25000000; ++b);
    if(mythread_create(quantum_1 ,HIGH_PRIORITY) == -1){
        printf("thread failed to initialize\n");
        exit(-1);
    }
    mythread_exit();

    printf("This program should never come here\n");
    return 0;
} /****** End main() ******/
