#include "pthreadfuncs.h"

// common resources - is a file for logging
int g_fd = -1;
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

// git ID of current thread
pid_t getThreadID(void) {
    return (pid_t) syscall(SYS_gettid);
}

// write a string from thread with mutex
// returns 0 on success, -1 on error
int write_line(const char *msg) {
    pthread_mutex_lock(&g_lock);
    ssize_t n = write(g_fd, msg, strlen(msg));
    if (n < 0) {
        fprintf(stderr, "write() failed: %s, [file descr = %d]\n", strerror(errno), g_fd);
    }
    pthread_mutex_unlock(&g_lock);
    return n < 0 ? -1 : 0;
}

// pause in milliseconds (nanosleep instead of the obsolete usleep)
void sleep_ms(long ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

// delay between iterations: environment variable SLEEP_MS (default 100 ms)
static long delay_ms(void) {
    const char *e = getenv("SLEEP_MS");
    return e ? atol(e) : 100;
}

// function for thread
void *func_thread(void *arg){
struct ThreadArgs *t = (struct ThreadArgs *)arg;
    char buf[128];
    // write something in opened file
    for (int i = 0; i < COUNT_ITERATIONS; ++i) {
        // here we write process id, parent process id, thread id, and name of thread with iteration
        snprintf(buf, sizeof(buf),
                 "[tag = %s] pid = %d ppid = %d tid = %d iter = %d\n", t->tag, getpid(), getppid(), getThreadID(), i);
        write_line(buf);
        // imitation of something
        sleep_ms(delay_ms());
    }
    return NULL;
}

void about()  {
    printf("Pthread example\n");
}