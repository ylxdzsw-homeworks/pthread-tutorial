#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <unistd.h>
#include <numa.h>
#include <numaif.h>

#define C_MEMCPY "C library: memcpy"
#define SINGLE_THREAD "Singlethreading"
#define MULTI_THREAD "Multithreading"
#define MULTI_AFFINITY "Multithreading with affinity"
#define MEM_LOCAL "Multithreading with numa_alloc_local"
#define MEM_INTER "Multithreading with numa_alloc_interleaved"

/* You may need to define struct here */

/*!
 * \brief subroutine function
 *
 * \param arg, input arguments pointer
 * \return void*, return pointer
 */
void *mt_memcpy(void *arg) {
    /* TODO: Your code here. */
}

/*!
 * \brief wrapper function
 *
 * \param dst, destination pointer
 * \param src, source pointer
 * \param size, copy bytes
 */
void multi_thread_memcpy(void *dst, const void *src, size_t size, int k) {
    /* TODO: Your code here. */
}

/*!
 * \brief bind new threads to the same NUMA node as the main
 * thread and then do multithreading memcy
 *
 * \param dst, destination pointer
 * \param src, source pointer
 * \param size, copy bytes
 */
void multi_thread_memcpy_with_affinity(void *dst, const void *src, size_t size, int k) {
    /* TODO: Your code here. */
}


/*!
 * \brief (Bonus Question) subroutine function for the
 * bonus question.
 *
 * \param arg, input arguments pointer
 * \return void*, return pointer
 */
void *mt_page_memcpy(void *arg) {
    /* TODO: (Bonus Question) Your code here. */
}

/*!
 * \brief (Bonus Question) bind new threads to different 
 * NUMA nodes. For 32 threads, you bind each node with 16
 * threads. Run your code with two memory policies,
 * 1) *local*, 2) *interleave*.
 *
 * \param dst, destination pointer
 * \param src, source pointer
 * \param size, size of the data
 * \param k, # of threads
 */
void multi_thread_memcpy_with_interleaved_affinity(void *dst, const void *src, size_t size, int k) {
  /* TODO: (Bonus Question) Your code here. */
}

/* benchmark: single-threaded version */
void single_thread_memcpy(void *dst, const void *src, size_t size) {
  float *in = (float *)src;
  float *out = (float *)dst;

  for (size_t i = 0; i < size / 4; ++i) {
    out[i] = in[i];
  }
  if (size % 4) {
    memcpy(out + size / 4, in + size / 4, size % 4);
  }
}

int execute(const char *command, int len, int k)
{
  /* allocate memory */
  float *dst = (float *) malloc( len * sizeof(float) );
  float *src = (float *) malloc( len * sizeof(float) );
  assert(dst != NULL);
  assert(src != NULL);

  /* warmup */
  memcpy(dst, src, len * sizeof(float));

  /* timing the memcpy */
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  if ( strcmp(command, C_MEMCPY)==0 )
  {
    memcpy(dst, src, len*sizeof(float));
  }
  else if ( strcmp(command, SINGLE_THREAD)==0 )
  {
    single_thread_memcpy(dst, src, len*sizeof(float));
  }
  else if ( strcmp(command, MULTI_THREAD)==0 )
  {
    multi_thread_memcpy(dst, src, len*sizeof(float), k);
  }
  else if ( strcmp(command, MULTI_AFFINITY)==0 )
  {
    multi_thread_memcpy_with_affinity(dst, src, len*sizeof(float), k);
  }
  else
  {
    fprintf(stderr, "execution failure.\n");
    goto out;
  }
  clock_gettime(CLOCK_MONOTONIC_RAW, &end);

  /* check correctness (with "warmup" disabled) */
  assert( memcmp(src, dst, len*sizeof(float)) == 0 );

  float delta_us = (end.tv_sec - start.tv_sec) * 1.0e6 +
                     (end.tv_nsec - start.tv_nsec) * 1.0e-3;
  printf("[%s]\tThe throughput is %.2f Gbps.\n",
          command, len*sizeof(float)*8 / (delta_us*1000.0) );

out: 
  /* free the memory */
  free(dst);
  free(src);

  return 0;
}

int execute_numa(const char *command, int len, int k)
{
  /* allocate memory */
  float *dst, *src;
  if ( strcmp(command, MEM_LOCAL)==0 )
  {
    dst = numa_alloc_local( len * sizeof(float) );
    src = numa_alloc_local( len * sizeof(float) );
  }
  else if ( strcmp(command, MEM_INTER)==0 )
  {
    dst = numa_alloc_interleaved( len * sizeof(float) );
    src = numa_alloc_interleaved( len * sizeof(float) );
  }
  else
  {
    fprintf(stderr, "numa execution failure.\n");
    return -1;
  }
  assert(dst != NULL);
  assert(src != NULL);

  /* warmup */
  memcpy(dst, src, len * sizeof(float));

  /* timing the memcpy */
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  multi_thread_memcpy_with_interleaved_affinity(dst, src, len*sizeof(float), k);
  clock_gettime(CLOCK_MONOTONIC_RAW, &end);

  /* check correctness (with "warmup" disabled) */
  assert( memcmp(src, dst, len*sizeof(float)) == 0 );

  float delta_us = (end.tv_sec - start.tv_sec) * 1.0e6 +
                     (end.tv_nsec - start.tv_nsec) * 1.0e-3;
  printf("[%s]\tThe throughput is %.2f Gbps.\n",
          command, len*sizeof(float)*8 / (delta_us*1000.0) );
  
  /* free the memory */
  numa_free(src, len*sizeof(float));
  numa_free(dst, len*sizeof(float));

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr,
            "Error: The program accepts exact 2 intergers.\n The first is the "
            "vector size and the second is the number of threads.\n");
    exit(1);
  }
  const int len = atoi(argv[1]);
  const int k = atoi(argv[2]);
  if (len < 0 || k < 1) {
    fprintf(stderr, "Error: invalid arguments.\n");
    exit(1);
  }
  // printf("Vector size=%d\tthreads len=%d.\n", len, k);

  /* C library's memcpy (1 byte) */
  execute(C_MEMCPY, len, k);
  /* single-threaded memcpy (4 bytes) */
  execute(SINGLE_THREAD, len, k);
  /* multi-threaded memcpy */
  execute(MULTI_THREAD, len, k);
  /* multi-threaded memcpy with affinity set */
  execute(MULTI_AFFINITY, len, k);

  /* Bonus: multi-threaded memcpy with local NUMA memory policy */
  execute_numa(MEM_LOCAL, len, k);
  /* Bonus: multi-threaded memcpy with interleaved NUMA memory policy */
  execute_numa(MEM_INTER, len, k);

  return 0;
}
