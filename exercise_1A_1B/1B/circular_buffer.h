/**
 * @file circular_buffer.h
 * @author Florian Fuerst (12122096)
 * @brief header file which includes all headers, defines the names of all resources and initializes the circular buffer,
 *        shm and semaphores and defines all used structs

 * @date 2022-11-12
 *
 * @copyright Copyright (c) 2022
 *
 */

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
#define SHM_NAME "/12122096_SHM"
#define SEM_FREE_NAME "/12122096_FREE"
#define SEM_USED_NAME "/12122096_USED"
#define SEM_BLOCKED_NAME "/12122096_BLOCKED"

// init instance of circ_buffer, semaphores and shm
struct circ_buffer *buff = NULL;
sem_t *free_sem = NULL;
sem_t *used_sem = NULL;
sem_t *blocked_sem = NULL;
int shm_fd = 0;

// define structs

/**
 * @brief defines a struct with one edge with two vertices 'u' and 'v'
 *
 */
struct edge
{
    long vertex_u;
    long vertex_v;
};

/**
 * @brief defines a struct with 8 edges of the feedback arc set and the number of edges of the current fb arc set
 *
 */
struct element
{
    int edge_number;
    struct edge fb_arc_set[8];
};

/**
 * @brief defines a struct of the circular buffer;
 *        the state bool notifies all generators to terminate (before the supervisore terminates);
 *        the writing position tells the generator(s) where to write on the circular buffer
 *
 */
struct circ_buffer
{
    bool state;
    int wr_pos;
    struct element buffer[BUFFER_SIZE];
};
