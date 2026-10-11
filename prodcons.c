#include "pthreadfuncs.h"

#define ITEMS 10

// ---- task 30: one-slot buffer, ONE condition variable
static int slot, full = 0, done = 0;
static pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv1 = PTHREAD_COND_INITIALIZER;

static void *producer1(void *arg) {
    (void)arg;
    for (int i = 1; i <= ITEMS; ++i) {
        pthread_mutex_lock(&m1);
        while (full) pthread_cond_wait(&cv1, &m1);   // always in a loop: spurious wakeups
        slot = i;
        full = 1;
        pthread_cond_broadcast(&cv1);                // one variable serves both sides
        pthread_mutex_unlock(&m1);
    }
    pthread_mutex_lock(&m1);
    done = 1;
    pthread_cond_broadcast(&cv1);
    pthread_mutex_unlock(&m1);
    return NULL;
}

static void *consumer1(void *arg) {
    (void)arg;
    char buf[64];
    for (;;) {
        pthread_mutex_lock(&m1);
        while (!full && !done) pthread_cond_wait(&cv1, &m1);
        if (!full && done) { pthread_mutex_unlock(&m1); break; }
        int v = slot;
        full = 0;
        pthread_cond_broadcast(&cv1);
        pthread_mutex_unlock(&m1);
        snprintf(buf, sizeof(buf), "consumer: got %d\n", v);
        write_line(buf);
    }
    return NULL;
}

void prodcons_one(void) {
    pthread_t p, c;
    pthread_create(&p, NULL, producer1, NULL);
    pthread_create(&c, NULL, consumer1, NULL);
    pthread_join(p, NULL);
    pthread_join(c, NULL);
}

// ---- task 31: ring buffer of N items, TWO condition variables
#define N 4
#define RING_ITEMS 20
static int ring[N], head = 0, tail = 0, count = 0, finished = 0;
static pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

static void *producer2(void *arg) {
    (void)arg;
    for (int i = 1; i <= RING_ITEMS; ++i) {
        pthread_mutex_lock(&m2);
        while (count == N) pthread_cond_wait(&not_full, &m2);   // wait for free space
        ring[tail] = i;
        tail = (tail + 1) % N;
        count++;
        pthread_cond_signal(&not_empty);                         // wake a consumer
        pthread_mutex_unlock(&m2);
    }
    pthread_mutex_lock(&m2);
    finished = 1;
    pthread_cond_broadcast(&not_empty);
    pthread_mutex_unlock(&m2);
    return NULL;
}

static void *consumer2(void *arg) {
    (void)arg;
    char buf[64];
    for (;;) {
        pthread_mutex_lock(&m2);
        while (count == 0 && !finished) pthread_cond_wait(&not_empty, &m2);  // wait for data
        if (count == 0 && finished) { pthread_mutex_unlock(&m2); break; }
        int v = ring[head];
        head = (head + 1) % N;
        count--;
        pthread_cond_signal(&not_full);                          // wake the producer
        pthread_mutex_unlock(&m2);
        snprintf(buf, sizeof(buf), "ring consumer: got %d\n", v);
        write_line(buf);
    }
    return NULL;
}

void prodcons_ring(void) {
    pthread_t p, c;
    pthread_create(&p, NULL, producer2, NULL);
    pthread_create(&c, NULL, consumer2, NULL);
    pthread_join(p, NULL);
    pthread_join(c, NULL);
}
