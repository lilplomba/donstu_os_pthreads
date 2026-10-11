#ifndef PTHREADFUNCS_H
#define PTHREADFUNCS_H

#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

// count of threads
#define COUNT_THREADS 4
// count of iterations (can be overridden: make CFLAGS="... -DCOUNT_ITERATIONS=10000")
#ifndef COUNT_ITERATIONS
#define COUNT_ITERATIONS 3
#endif

// struct send to thread
struct ThreadArgs {
    // my ID of thread
    int  id;
    // something string (with name of thread, f.e.)
    char tag[10];
    // message from main, thread writes it to the log
    char message[64];
};

// common resources - is a file for logging
extern int g_fd;

// mutex
extern pthread_mutex_t g_lock;

// experiment switches (default values = normal work)
extern int g_use_lock;   // 0 only in the experiment of task 27
extern int g_chunk;      // 1: write_line sends the line in small pieces (task 27)
extern int g_detach_id;  // id of the thread that detaches itself (task 22)
extern long g_counter;   // shared counter (task 29)

// pause in milliseconds
void sleep_ms(long ms);

// git ID of current thread
pid_t getThreadID(void);

// write a string from thread with mutex (0 - ok, -1 - error)
int write_line(const char *msg);

// pointer for thread's function
void *func_thread(void *arg);

void about(void);

// ---- experiments.c: tasks 22-26, 29, 33-36
void exp_detach(void);          // 22
void exp_retval(void);          // 23, 25
void exp_heap(void);            // 26
void exp_counter(int use_lock); // 29
void exp_equal(void);           // 33
void exp_sigwait(void);         // 34
void exp_sigquit(void);         // 35

// ---- prodcons.c: tasks 30-31
void prodcons_one(void);        // one-slot buffer, one condition variable
void prodcons_ring(void);       // ring buffer, two condition variables

#endif
