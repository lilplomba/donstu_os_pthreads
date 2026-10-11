#include "pthreadfuncs.h"

// common resources - is a file for logging
int g_fd = -1;
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

int g_use_lock = 1;
int g_chunk = 0;
int g_detach_id = 0;
long g_counter = 0;

// git ID of current thread
pid_t getThreadID(void) {
    return (pid_t) syscall(SYS_gettid);
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

// write a string from thread with mutex; returns 0 on success, -1 on error
int write_line(const char *msg) {
    if (g_use_lock) pthread_mutex_lock(&g_lock);
    ssize_t n = 0;
    size_t len = strlen(msg);
    if (g_chunk) {
        // experiment: the line leaves in pieces of 8 bytes (several write() calls)
        for (size_t off = 0; off < len && n >= 0; off += 8) {
            size_t part = len - off < 8 ? len - off : 8;
            n = write(g_fd, msg + off, part);
            sched_yield();
        }
    } else {
        n = write(g_fd, msg, len);
    }
    if (n < 0) {
        fprintf(stderr, "write() failed: %s, [file descr = %d]\n", strerror(errno), g_fd);
    }
    if (g_use_lock) pthread_mutex_unlock(&g_lock);
    return n < 0 ? -1 : 0;
}

// function for thread
void *func_thread(void *arg){
    struct ThreadArgs *t = (struct ThreadArgs *)arg;
    char buf[160];
    // task 22: one thread detaches itself
    if (t->id == g_detach_id) pthread_detach(pthread_self());
    // write something in opened file
    for (int i = 0; i < COUNT_ITERATIONS; ++i) {
        // here we write process id, parent process id, kernel thread id (TID),
        // library thread id (pthread_self), message from main and iteration
        snprintf(buf, sizeof(buf),
                 "[tag = %s] pid = %d ppid = %d tid = %d self = %lu msg = \"%s\" iter = %d\n",
                 t->tag, getpid(), getppid(), getThreadID(),
                 (unsigned long)pthread_self(), t->message, i);
        write_line(buf);
        // imitation of something
        sleep_ms(delay_ms());
    }
    return NULL;
}

void about()  {
    printf("Pthread example\n");
}
