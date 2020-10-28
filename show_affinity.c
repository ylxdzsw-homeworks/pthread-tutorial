
#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  int rc;
  cpu_set_t cpuset;
  pthread_t thread;

  thread = pthread_self();

  rc = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
  assert(rc == 0);
  printf("Before setting: pthread_getaffinity_np() contained:\n");
  for (int j = 0; j < CPU_SETSIZE; j++)
    if (CPU_ISSET(j, &cpuset))
      printf("    CPU %d\n", j);

  printf("Set affinity mask to include CPUs (1, 3, 5, ...  2n+1)\n");
  CPU_ZERO(&cpuset);
  for (int j = 0; j < 8; j++)
    if (j % 2 == 1)
      CPU_SET(j, &cpuset);
  rc = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
  assert(rc == 0);

  rc = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
  assert(rc == 0);
  printf("After setting: pthread_getaffinity_np() contained:\n");
  for (int j = 0; j < CPU_SETSIZE; j++)
    if (CPU_ISSET(j, &cpuset))
      printf("    CPU %d\n", j);

  return 0;
}