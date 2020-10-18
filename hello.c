#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    int tid;
} thread_data_t;

void *entry_point(void *arg)
{
    thread_data_t *data = (thread_data_t *)(arg);
    printf("Hello World from thread %d\n", data->tid);
    return NULL;
}

int main()
{
    const int N = 8;
    pthread_t thr[N];
    thread_data_t data[N];
    for (int i = 0; i < N; ++i)
    {
        data[i].tid = i;
        if (pthread_create(&thr[i], NULL, &entry_point, &data[i]))
        {
            perror("Could not create thread!\n");
            exit(1);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        if (pthread_join(thr[i], NULL))
        {
            perror("Could not join thread!\n");
            exit(1);
        }
    }
    return 0;
}