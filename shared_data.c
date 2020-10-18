#include <stdio.h>
#include <pthread.h>
#include <assert.h>

static int counter = 0;

void *entry_point(void *arg)
{
    printf("Thread %s: begin\n", (char *)arg);
    for (int i = 0; i < 1e7; ++i)
    {
        counter += 1;
    }
    printf("Thread %s: done\n", (char *)arg);
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t p1, p2;
    printf("main: begin with counter = %d\n", counter);
    int rc1, rc2;
    rc1 = pthread_create(&p1, NULL, entry_point, (void *)"A");
    assert(rc1 == 0);
    rc2 = pthread_create(&p2, NULL, entry_point, (void *)"B");
    assert(rc2 == 0);

    // join waits for the threads to finish
    rc1 = pthread_join(p1, NULL);
    assert(rc1 == 0);
    rc2 = pthread_join(p2, NULL);
    assert(rc2 == 0);
    printf("main: done with counter = %d\n", counter);
    return 0;
}