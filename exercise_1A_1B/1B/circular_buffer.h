#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#define BUFFER_SIZE (20)
#define SHM_NAME "/fb_arc_set_shm"
#define SEM_FREE_NAME "/SEM_FREE_NAME"
#define SEM_USED_NAME "/SEM_USED_NAME"
#define SEM_BLOCKED_NAME "/SEM_BLOCKED_NAME"

// init instance of circ_buffer, semaphores and shm
struct circ_buffer *buff = NULL;
sem_t *free_sem = NULL;
sem_t *used_sem = NULL;
sem_t *blocked_sem = NULL;
int shm_fd = 0;

// define structs
struct edge
{
    long vertex_u;
    long vertex_v;
};

struct element
{
    int edge_number;
    struct edge fb_arc_set[8];
};

struct circ_buffer
{
    bool state;
    int wr_pos;
    struct element buffer[BUFFER_SIZE];
};
