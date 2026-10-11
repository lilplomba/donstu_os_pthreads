#include "pthreadfuncs.h"

// small helper: write a formatted line to the log and to the console
static void say(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#include <stdarg.h>
static void say(const char *fmt, ...) {
    char buf[200];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    write_line(buf);
    fputs(buf, stdout);
}

static void make_args(struct ThreadArgs *a) {
    for (int i = 0; i < COUNT_THREADS; ++i) {
        a[i].id = i + 1;
        snprintf(a[i].tag, sizeof(a[i].tag), "T%d", i);
        snprintf(a[i].message, sizeof(a[i].message), "hello from main to T%d", i);
    }
}

// ---- task 22: a thread detaches itself, pthread_join() must fail
void exp_detach(void) {
    pthread_t th[COUNT_THREADS];
    struct ThreadArgs a[COUNT_THREADS];
    make_args(a);
    g_detach_id = 4;                          // T3 calls pthread_detach(pthread_self())
    for (int i = 0; i < COUNT_THREADS; ++i) pthread_create(&th[i], NULL, func_thread, &a[i]);
    sleep_ms(50);                             // let T3 detach itself
    int rc = pthread_join(th[3], NULL);       // pthread_join returns the error code, errno is not set
    say("pthread_join(T3, detached) -> rc = %d (%s)\n", rc, strerror(rc));
    for (int i = 0; i < 3; ++i) {
        rc = pthread_join(th[i], NULL);
        say("pthread_join(T%d) -> rc = %d (%s)\n", i, rc, strerror(rc));
    }
    sleep_ms(300);                            // the detached thread finishes by itself
}

// ---- task 23: return a value via pthread_exit; task 25: return a malloc'ed string
static void *exit_thread(void *arg) {
    (void)arg;
    pthread_exit((void *)42L);
}

static void *string_thread(void *arg) {
    int iters = *(int *)arg;
    char *s = malloc(64);                     // lives in the heap, not on the thread stack
    snprintf(s, 64, "thread made %d iterations", iters);
    return s;
}

void exp_retval(void) {
    pthread_t t1, t2;
    void *ret = NULL;
    pthread_create(&t1, NULL, exit_thread, NULL);
    pthread_join(t1, &ret);                   // pass the address of the pointer
    say("task 23: thread returned %ld via pthread_exit\n", (long)ret);

    int iters = COUNT_ITERATIONS;
    pthread_create(&t2, NULL, string_thread, &iters);
    pthread_join(t2, &ret);
    say("task 25: thread returned a string: \"%s\"\n", (char *)ret);
    free(ret);                                // the receiver frees the memory
}

// ---- task 26: detached thread + structure in malloc
static void *heap_thread(void *arg) {
    struct ThreadArgs *t = arg;
    sleep_ms(100);                            // by now start_detached() has already returned
    char buf[160];
    snprintf(buf, sizeof(buf), "[detached %s] id = %d msg = \"%s\"\n", t->tag, t->id, t->message);
    write_line(buf);
    free(t);                                  // the thread owns the structure and frees it
    return NULL;
}

static void start_detached(int id) {
    struct ThreadArgs *t = malloc(sizeof(*t)); // a local variable here would die at return
    t->id = id;
    snprintf(t->tag, sizeof(t->tag), "D%d", id);
    snprintf(t->message, sizeof(t->message), "structure from the heap");
    pthread_t th;
    pthread_create(&th, NULL, heap_thread, t);
    pthread_detach(th);
}

void exp_heap(void) {
    for (int i = 1; i <= 3; ++i) start_detached(i);
    printf("main: start_detached() returned, threads are still working\n");
    sleep_ms(400);                            // detached threads cannot be joined: wait a little
}

// ---- task 29: counter++ without and with mutex
static void *counter_thread(void *arg) {
    int use_lock = *(int *)arg;
    for (int i = 0; i < 100000; ++i) {
        if (use_lock) pthread_mutex_lock(&g_lock);
        g_counter++;                          // read, add, write: three steps
        if (use_lock) pthread_mutex_unlock(&g_lock);
    }
    return NULL;
}

void exp_counter(int use_lock) {
    pthread_t th[COUNT_THREADS];
    g_counter = 0;
    for (int i = 0; i < COUNT_THREADS; ++i) pthread_create(&th[i], NULL, counter_thread, &use_lock);
    for (int i = 0; i < COUNT_THREADS; ++i) pthread_join(th[i], NULL);
    say("counter (%s mutex) = %ld, expected %d\n", use_lock ? "with" : "without", g_counter, COUNT_THREADS * 100000);
}

// ---- task 33: pthread_equal instead of ==
static void *idle_thread(void *arg) { (void)arg; sleep_ms(100); return NULL; }

void exp_equal(void) {
    pthread_t a, b, me = pthread_self();
    pthread_create(&a, NULL, idle_thread, NULL);
    pthread_create(&b, NULL, idle_thread, NULL);
    say("pthread_self() of main = %lu, a = %lu, b = %lu\n", (unsigned long)me, (unsigned long)a, (unsigned long)b);
    say("pthread_equal(a, a) = %d (non-zero: same thread)\n", pthread_equal(a, a) != 0);
    say("pthread_equal(a, b) = %d (zero: different threads)\n", pthread_equal(a, b) != 0);
    say("pthread_equal(main, pthread_self()) = %d\n", pthread_equal(me, pthread_self()) != 0);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
}

// ---- task 34: a thread takes SIGINT through sigwait()
static void *sigwait_thread(void *arg) {
    sigset_t *set = arg;
    int sig = 0;
    sigwait(set, &sig);                       // waits for a blocked signal, no handler is called
    say("sigwait thread (tid = %d) received signal %d (%s)\n", getThreadID(), sig, strsignal(sig));
    return NULL;
}

void exp_sigwait(void) {
    static sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    // the signal must be blocked BEFORE creating threads (they inherit the mask),
    // otherwise the default action will simply terminate the program
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    pthread_t th;
    pthread_create(&th, NULL, sigwait_thread, &set);
    say("main: pid = %d, main tid = %d, waiting for SIGINT\n", getpid(), getThreadID());
    if (!getenv("WAIT_EXTERNAL")) {           // by default send the signal ourselves
        sleep_ms(100);
        kill(getpid(), SIGINT);               // the same as: kill -INT <pid> from another terminal
    }
    pthread_join(th, NULL);
}

// ---- task 35: SIGQUIT handler, which thread runs it?
static void quit_handler(int sig) {
    // only async-signal-safe calls: write() and a raw syscall
    char buf[64] = "SIGQUIT handled in tid = ";
    size_t n = strlen(buf);
    long tid = syscall(SYS_gettid);
    char digits[16];
    int d = 0;
    while (tid > 0) { digits[d++] = '0' + tid % 10; tid /= 10; }
    while (d > 0) buf[n++] = digits[--d];
    buf[n++] = '\n';
    (void)sig;
    if (write(STDOUT_FILENO, buf, n) < 0) {}
}

static void *worker_thread(void *arg) {
    (void)arg;
    for (int i = 0; i < 5; ++i) sleep_ms(100);
    return NULL;
}

void exp_sigquit(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = quit_handler;
    sigaction(SIGQUIT, &sa, NULL);            // the handler is common for the whole process
    pthread_t th[3];
    for (int i = 0; i < 3; ++i) pthread_create(&th[i], NULL, worker_thread, NULL);
    if (getenv("BLOCK_MAIN")) {               // block SIGQUIT only in the main thread (after creating workers)
        sigset_t s;
        sigemptyset(&s);
        sigaddset(&s, SIGQUIT);
        pthread_sigmask(SIG_BLOCK, &s, NULL);
    }
    say("main tid = %d, worker tids are different (see ps -L)\n", getThreadID());
    sleep_ms(100);
    kill(getpid(), SIGQUIT);                  // signal for the whole process
    sleep_ms(50);
    for (int i = 0; i < 3; ++i) pthread_join(th[i], NULL);
}
