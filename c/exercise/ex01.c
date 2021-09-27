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

#define PRINT_AFFINITY 1
#define WARMUP 1

/* You may need to define struct here */
typedef struct {
  const void* src; // the address of the source whole buffer
  void* dst; // the address of the whole target buffer
  size_t size; // the size that this thread should copy
  int rank; // the rank of this thread
} MtMemcpyArg;

/*!
 * \brief subroutine function
 *
 * \param arg, input arguments pointer
 * \return void*, return pointer
 */
void *mt_memcpy(void *arg) {
  MtMemcpyArg* mt_memcpy_arg = (MtMemcpyArg*) arg;

#if PRINT_AFFINITY
  int cpu, node;
  int err = syscall(SYS_getcpu, &cpu, &node);
  assert(!err);
  printf("thread %d runs on cpu %d, NUMA node %d\n", mt_memcpy_arg->rank, cpu, node);
#endif

  // the starting address of the part that this thread should copy
  const void* src = mt_memcpy_arg->src + mt_memcpy_arg->size * mt_memcpy_arg->rank;
  void* dst = mt_memcpy_arg->dst + mt_memcpy_arg->size * mt_memcpy_arg->rank;

  // call the single thread function to copy the part of this thread
  single_thread_memcpy(dst, src, mt_memcpy_arg->size);
  return NULL;
}

/*!
 * \brief wrapper function
 *
 * \param dst, destination pointer
 * \param src, source pointer
 * \param size, copy bytes
 */
void multi_thread_memcpy(void *dst, const void *src, size_t size, int k) {
  multi_thread_memcpy_with_attr(dst, src, size, k, NULL);
}

void multi_thread_memcpy_with_attr(void *dst, const void *src, size_t size, int k, pthread_attr_t* attr) {
  size_t size_per_thread = size / k;
  MtMemcpyArg base_arg = { .src = src, .dst = dst, .size = size_per_thread };

  MtMemcpyArg* args = (MtMemcpyArg*) malloc(k * sizeof(MtMemcpyArg));
  pthread_t* thread_handlers = (pthread_t*) malloc(k * sizeof(pthread_t));

  // prepare argument object for each thread and launch them
  for (int i = 0; i < k; i++) {
    args[i] = base_arg;
    args[i].rank = i;
    int err = pthread_create(thread_handlers + i, attr, mt_memcpy, args + i);
    assert(!err);
  }

  // copy the residual bytes in the main thread if any.
  if (size % k)
    single_thread_memcpy(dst + size / k * k, src + size / k * k, size % k);

  // join the threads and free the argument objects.
  for (int i = 0; i < k; i++) {
    int err = pthread_join(thread_handlers[i], NULL);
    assert(!err);
  }

cleanup:
  free(args);
  free(thread_handlers);
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
  assert(k <= get_nprocs() / 2);

  int node;
  assert( syscall(SYS_getcpu, NULL, &node) == 0 );

  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);

  // append the CPUs in the same NUMA node into the cpu set
  for (int cpu_id = node; cpu_id < get_nprocs(); cpu_id += 2)
    CPU_SET(cpu_id, &cpu_set);

  pthread_attr_t pthread_attr;
  int err = pthread_attr_init(&pthread_attr);
  assert(!err);

  err = pthread_attr_setaffinity_np(&pthread_attr, sizeof(cpu_set_t), &cpu_set);
  assert(!err);

  // ensure that the main thread still in the NUMA node.
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);

  multi_thread_memcpy_with_attr(dst, src, size, k, &pthread_attr);
}

/** You may would like to define a new struct for
 *  the bonus question here.
**/

#ifdef BUILD_BONUS

typedef struct {
  const void* src; // the starting address of the whole source buffer
  void* dst; // the starting address of the whole target buffer
  size_t size; // total size
  int world_size; // total number of threads
  int rank; // the rank of this thread
} MtPageMemcpyArg;

/*!
 * \brief (Bonus Question) subroutine function for the
 * bonus question.
 *
 * \param arg, input arguments pointer
 * \return void*, return pointer
 */
void *mt_page_memcpy(void *data) {
  MtPageMemcpyArg* arg = (MtPageMemcpyArg*) data;
  size_t page_size = getpagesize();

  assert(arg->world_size % 2 == 0); // otherwise a thread would copy pages on different nodes

  size_t offset = arg->rank * page_size;
  while (offset + page_size <= arg->size) {
    single_thread_memcpy(arg->dst + offset, arg->src + offset, page_size);
    offset += page_size * arg->world_size;
  }

  return NULL;
}

/*!
 * \brief (Bonus Question) bind new threads to different 
 * NUMA nodes. E.g., for 32 threads, you bind each node
 * with 16 threads. Run your code with two memory policies,
 * 1) *local*, 2) *interleave*.
 *
 * \param dst, destination pointer
 * \param src, source pointer
 * \param size, size of the data
 * \param k, # of threads
 */
void multi_thread_memcpy_with_interleaved_affinity(void *dst, const void *src, size_t size, int k) {
  size_t page_size = getpagesize();
  MtPageMemcpyArg base_arg = { .src = src, .dst = dst, .size = size, .world_size = k };

  MtPageMemcpyArg* args = (MtPageMemcpyArg*) malloc(k * sizeof(MtPageMemcpyArg));
  pthread_t* thread_handlers = (pthread_t*) malloc(k * sizeof(pthread_t));

  // get the cpu_offset so each thread is affixed to the NUMA node of the interleaved pages.
  int cpu_offset;
  int err = get_mempolicy(&cpu_offset, NULL, 0, src, MPOL_F_NODE | MPOL_F_ADDR);
  assert(!err);

  // prepare argument object for each thread and launch them
  for (int i = 0; i < k; i++) {
    args[i] = base_arg;
    args[i].rank = i;

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(i + cpu_offset, &cpu_set);

    pthread_attr_t pthread_attr;
    int err = pthread_attr_init(&pthread_attr);
    assert(!err);

    err = pthread_attr_setaffinity_np(&pthread_attr, sizeof(cpu_set_t), &cpu_set);
    assert(!err);

    err = pthread_create(thread_handlers + i, &pthread_attr, mt_page_memcpy, args + i);
    assert(!err);
  }

  // copy the residual bytes not aligned to pages in the main thread if any.
  if (size % page_size)
    single_thread_memcpy(dst + size / k * k, src + size / k * k, size % k);

  // join the threads and free the argument objects.
  for (int i = 0; i < k; i++) {
    int err = pthread_join(thread_handlers[i], NULL);
    assert(!err);
  }

cleanup:
  free(args);
  free(thread_handlers);
}
#endif

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
#if WARMUP
  memcpy(dst, src, len * sizeof(float));
#endif

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

#ifdef BUILD_BONUS
int execute_numa(const char *command, int len, int k)
{
  /* allocate memory */
  float *dst, *src;
  if ( strcmp(command, MEM_LOCAL)==0 )
  {
    /* allocate memory (`*src`, `*dst`) locally on the current node */
    src = numa_alloc_local(len * sizeof(float));
    dst = numa_alloc_local(len * sizeof(float));
  }
  else if ( strcmp(command, MEM_INTER)==0 )
  {
    /* allocate memory (`*src`, `*dst`) interleaved on each node */
    src = numa_alloc_interleaved(len * sizeof(float));
    dst = numa_alloc_interleaved(len * sizeof(float));
  }
  else
  {
    fprintf(stderr, "numa execution failure.\n");
    return -1;
  }
  assert(dst != NULL);
  assert(src != NULL);

#if PRINT_AFFINITY
    int node;
    size_t page_size = getpagesize();
    int err = get_mempolicy(&node, NULL, 0, src, MPOL_F_NODE | MPOL_F_ADDR);
    assert(!err);
    printf("the first page of src is on node %d\n", node);
    err = get_mempolicy(&node, NULL, 0, src + page_size / sizeof(float), MPOL_F_NODE | MPOL_F_ADDR);
    assert(!err);
    printf("the second page of src is on node %d\n", node);
    err = get_mempolicy(&node, NULL, 0, dst, MPOL_F_NODE | MPOL_F_ADDR);
    assert(!err);
    printf("the first page of dst is on node %d\n", node);
    err = get_mempolicy(&node, NULL, 0, dst + page_size / sizeof(float), MPOL_F_NODE | MPOL_F_ADDR);
    assert(!err);
    printf("the second page of dst is on node %d\n", node);
#endif

  /* warmup */
#if WARMUP
  memcpy(dst, src, len * sizeof(float));
#endif

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
  
  /* free `*dst` and `*src` */
  numa_free(src, len * sizeof(float));
  numa_free(dst, len * sizeof(float));
  return 0;
}
#endif

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

#ifdef BUILD_BONUS
  /* Bonus: multi-threaded memcpy with local NUMA memory policy */
  execute_numa(MEM_LOCAL, len, k);
  /* Bonus: multi-threaded memcpy with interleaved NUMA memory policy */
  execute_numa(MEM_INTER, len, k);
#endif

  return 0;
}
