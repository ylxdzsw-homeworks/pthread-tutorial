#include <stdio.h>
#include <pthread.h>

static volatile int counter = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *entry_point_slow(void *arg) {
    printf("Thread %s: begin\n", (char *)arg);
    for (int i=0; i < 1e7; ++i) {
        pthread_mutex_lock(&lock);
        counter += 1;
        pthread_mutex_unlock(&lock);
    }
    printf("Thread %s: done\n", (char *)arg);
    return NULL;
}

void *entry_point(void *arg) {
    printf("Thread %s: begin\n", (char *)arg);
    pthread_mutex_lock(&lock);
    for (int i = 0; i < 1e7; ++i) {
        counter += 1;
    }
    pthread_mutex_unlock(&lock);
    printf("Thread %s: done\n", (char *)arg);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p1, p2;

    printf("main: begin with counter = %d\n", counter);

    if ( pthread_create(&p1, NULL, entry_point, "A") != 0 )
    {
        fprintf(stderr, "pthread_create failed for thread A.\n");
    }
    if ( pthread_create(&p2, NULL, entry_point, "B") != 0 )
    {
        fprintf(stderr, "pthread_create failed for thread B.\n");
    }

    // join waits for the threads to finish
    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    printf("main: done with counter = %d\n", counter);
    return 0;
}
