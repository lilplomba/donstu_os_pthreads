#include "pthreadfuncs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>


int main(void) {
    // headline
    about();
    // array with threads
    pthread_t threads[COUNT_THREADS];
    struct ThreadArgs args[COUNT_THREADS];

    // sys call - open
    // file, modes, rights
    g_fd = open("output.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (g_fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    char line[128];
    snprintf(line, sizeof(line), "main: pid = %d, ppid = %d, opened file: 'output.log' (fd = %d)\n",
             getpid(), getppid(), g_fd);
    write_line(line);

    // create structs for threads
    for (int i = 0; i < COUNT_THREADS; ++i) {
        args[i].id = i + 1;
        snprintf(args[i].tag, sizeof(args[i].tag), "T%d", i);
    }
    
    // create threads
    for (int i = 0; i < COUNT_THREADS; i++) {
        int rc = pthread_create(&threads[i], NULL, func_thread, &args[i]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            return EXIT_FAILURE;
        }
    }

    // wait stoping all thread
    for (int i = 0; i < COUNT_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // sys call for close file
    if (close(g_fd) < 0) {
        perror("close");
        return EXIT_FAILURE;
    }
    // remove mutex
    pthread_mutex_destroy(&g_lock);
    printf("main: all threads finished, file closed\n");
    return EXIT_SUCCESS;
}