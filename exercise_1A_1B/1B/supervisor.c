/**
 * @file supervisor.c
 * @author Florian Fuerst (12122096)
 * @brief Sets up the shared memory and semaphores; The supervisor reads the results from the generator(s)
 *        from the circular buffer and saves the best solution, which gets also printed to standard output
 * @date 2022-11-12
 *
 */

#include "circular_buffer.h"

/**
 * @brief Function to handle signals;
 *        In this case 'state' in circular_buffer will be set to false if signal gets detected
 *
 * @param signal signal number to which the handling function is set
 */
static void handle_signal(int signal) { buff->state = false; }

/**
 * @brief Function to clean up shared memory and semaphores before exiting the program
 *
 * @param cleanup_state describes "how much" needs to be cleaned up; higher numbers also contain cases below
 * @param exit_code the exit code of how the program has to terminate; (is either EXIT_FAILURE or EXIT_SUCCESS)
 * @param argv simple hand over of program name argv[0] for error messages
 */
static void cleanup_shm_sem(int cleanup_state, int exit_code, char *argv[])
{
    switch (cleanup_state)
    {
    // cleanup semaphores
    case 6:
        // close and unlink semaphores; print error message if not successful
        if (sem_close(blocked_sem) == -1)
        {
            fprintf(stderr, "%s Error while closing semaphore: %s\n", argv[0], strerror(errno));
        }
        if (sem_unlink(SEM_BLOCKED_NAME) == -1)
        {
            fprintf(stderr, "%s Error while unlinking semaphore: %s\n", argv[0], strerror(errno));
        }

    case 5:
        if (sem_close(used_sem) == -1)
        {
            fprintf(stderr, "%s Error while closing semaphore: %s\n", argv[0], strerror(errno));
        }
        if (sem_unlink(SEM_USED_NAME) == -1)
        {
            fprintf(stderr, "%s Error while unlinking semaphore: %s\n", argv[0], strerror(errno));
        }

    case 4:
        if (sem_close(free_sem) == -1)
        {
            fprintf(stderr, "%s Error while closing semaphore: %s\n", argv[0], strerror(errno));
        }
        if (sem_unlink(SEM_FREE_NAME) == -1)
        {
            fprintf(stderr, "%s Error while unlinking semaphore: %s\n", argv[0], strerror(errno));
        }

    // cleanup shm
    case 3:
        // unmap shared memory object; print error message if not successful
        if (munmap(buff, sizeof(*buff)) == -1)
        {
            fprintf(stderr, "%s Error while unmapping shared memory object: %s\n", argv[0], strerror(errno));
        }

    case 2:
        //  close shared memory; print error message if not successful
        if (close(shm_fd) == -1)
        {
            fprintf(stderr, "%s Error while closing shared memory: %s\n", argv[0], strerror(errno));
        }

    case 1:
        // remove shared memory object; print error message if not successful
        if (shm_unlink(SHM_NAME) == -1)
        {
            fprintf(stderr, "%s Error while unlinking memory object: %s\n", argv[0], strerror(errno));
        }

    default:
        // exit with according error code
        exit(exit_code);
        break;
    }
}

/**
 * @brief Sets up the shared memory and semaphores
 *
 * @param argv simple hand over of program name argv[0] for error messages
 */
static void shm_sem_setup(char *argv[])
{
    // create and/or open the shared memory object; print error message if not successful
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR | O_EXCL, 0600);
    if (shm_fd == -1)
    {
        fprintf(stderr, "%s Error while creating and opening a new shared memory object: %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    // set the size of the shared memory; cleanup and print error message if not successful
    if (ftruncate(shm_fd, 2 * sizeof(struct circ_buffer)) == -1)
    {
        fprintf(stderr, "%s Error while setting up size of shared memory: %s\n", argv[0], strerror(errno));
        cleanup_shm_sem(1, EXIT_FAILURE, &argv[0]);
    }

    //  map shared memory object; cleanup and print error message if not successful
    if ((buff = mmap(NULL, sizeof(*buff), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
        fprintf(stderr, "%s Error while mapping shared memory object: %s\n", argv[0], strerror(errno));
        cleanup_shm_sem(2, EXIT_FAILURE, &argv[0]);
    }

    //  open semaphores; cleanup and print error message if not successful
    free_sem = sem_open(SEM_FREE_NAME, O_CREAT | O_EXCL, 0600, BUFFER_SIZE);
    if (free_sem == SEM_FAILED)
    {
        fprintf(stderr, "%s Error while creating and/or opening (a) new/existing semaphore(s) free: %s\n", argv[0], strerror(errno));
        cleanup_shm_sem(3, EXIT_FAILURE, &argv[0]);
    }
    used_sem = sem_open(SEM_USED_NAME, O_CREAT | O_EXCL, 0600, 0);
    if (used_sem == SEM_FAILED)
    {
        fprintf(stderr, "%s Error while creating and/or opening (a) new/existing semaphore(s) used: %s\n", argv[0], strerror(errno));
        cleanup_shm_sem(4, EXIT_FAILURE, &argv[0]);
    }
    blocked_sem = sem_open(SEM_BLOCKED_NAME, O_CREAT | O_EXCL, 0600, 1);
    if (blocked_sem == SEM_FAILED)
    {
        fprintf(stderr, "%s Error while creating and/or opening (a) new/existing semaphore(s) blocked: %s\n", argv[0], strerror(errno));
        cleanup_shm_sem(5, EXIT_FAILURE, &argv[0]);
    }
}

/**
 * @brief Sets up signal handling; checks correct program call; reads data from the circular buffer
 *        and saves and prints best solution to standard output; calls appropriate functions to make the
 *        program work how it should;
 *
 * @param argc number of arguments of the program call
 * @param argv array of the arguments of the program call
 * @return int return value of main method to tell when program is finished (and therefore can be removed from memory)
 */
int main(int argc, char *argv[])
{
    // check if arguments specified
    if (argc > 1)
    {
        fprintf(stderr, "%s No arguments allowed!\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // set signal handler
    struct sigaction sa = {.sa_handler = handle_signal};
    if (sigaction(SIGINT, &sa, NULL) + sigaction(SIGTERM, &sa, NULL) < 0)
    {
        fprintf(stderr, "%s Error while initializing signal handler: %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE); 
    }

    // setup shm and semaphores
    shm_sem_setup(&argv[0]);

    // init readin position
    int rd_pos = 0;

    // set best number of edges greater than max (== 8)
    int best_number_edges = 9;

    // set state to up
    buff->state = true;
    buff->wr_pos = 0;
    bool cancel = false;
    while (buff->state)
    {
        // check for state=false again
        if (!buff->state)
        {
            break;
        }

        if (sem_wait(used_sem) == -1)
        {
            // error message if errno is not interrupt
            if (errno != EINTR)
            {
                fprintf(stderr, "%s Error while semaphore is waiting: %s\n", argv[0], strerror(errno));
                exit(EXIT_FAILURE);
            }

            // true if "cancelled" program with SIGINT
            cancel = true;
        }

        // create object of current element in circular buffer
        struct element el = buff->buffer[rd_pos];

        // compares current best number of edges with number of edges of current element which is
        // stored in circular buffer

        // only compare if not canceled with SIGINT
        if (el.edge_number < best_number_edges && !cancel)
        {
            best_number_edges = el.edge_number;

            // print that graph is acyclic if best number of edges is zero
            if (best_number_edges == 0)
            {
                printf("This graph is already acyclic!\n");
                buff->state = false;
            }
            // print solution of current element if number of edges is greater than zero
            else
            {
                printf("Current best solution with %d edges: \n", best_number_edges);
                for (int i = 0; i < best_number_edges; i++)
                {
                    printf("%ld-%ld\n", el.fb_arc_set[i].vertex_u, el.fb_arc_set[i].vertex_v);
                }
            }
            printf("\n----------------\n\n");
        }
        sem_post(free_sem);

        // increment readin position
        rd_pos++;
        rd_pos %= BUFFER_SIZE;
    }
    // call function for cleanup process
    cleanup_shm_sem(6, EXIT_SUCCESS, &argv[0]);

    return 0;
}
