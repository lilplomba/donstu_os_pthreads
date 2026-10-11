#include "pthreadfuncs.h"

// usage: ./app [mode]
//   (no mode)  4 threads write to output.log          (tasks 18-21)
//   nojoin     the same without pthread_join()        (task 9)
//   nolock     the same without mutex                 (task 27)
//   nolock-chunk  without mutex, line sent in pieces  (task 27)
//   lock-chunk    the same pieces, but with mutex     (control)
//   detach | retval | heap | counter | counter-lock | prodcons | ring
//   equal | sigwait | sigquit                         (tasks 22-36)
int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "basic";
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

    if (!strcmp(mode, "detach")) exp_detach();
    else if (!strcmp(mode, "retval")) exp_retval();
    else if (!strcmp(mode, "heap")) exp_heap();
    else if (!strcmp(mode, "counter")) exp_counter(0);
    else if (!strcmp(mode, "counter-lock")) exp_counter(1);
    else if (!strcmp(mode, "prodcons")) prodcons_one();
    else if (!strcmp(mode, "ring")) prodcons_ring();
    else if (!strcmp(mode, "equal")) exp_equal();
    else if (!strcmp(mode, "sigwait")) exp_sigwait();
    else if (!strcmp(mode, "sigquit")) exp_sigquit();
    else {
        if (!strcmp(mode, "nolock")) g_use_lock = 0;
        if (!strcmp(mode, "nolock-chunk")) { g_use_lock = 0; g_chunk = 1; }
        if (!strcmp(mode, "lock-chunk")) g_chunk = 1;

        // create structs for threads
        for (int i = 0; i < COUNT_THREADS; ++i) {
            args[i].id = i + 1;
            snprintf(args[i].tag, sizeof(args[i].tag), "T%d", i);
            snprintf(args[i].message, sizeof(args[i].message), "hello from main to T%d", i);
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
        if (strcmp(mode, "nojoin")) {
            for (int i = 0; i < COUNT_THREADS; i++) {
                pthread_join(threads[i], NULL);
            }
        }
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
