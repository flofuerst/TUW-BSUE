/**
 * @file supervisor.c
 * @author Florian Fuerst (12122096)
 * @brief Checks if the input is a palindrom and returns the appropriate results
 * @date 2022-11-04
 *
 */

#include "circular_buffer.h"

static void handle_signal(int signal) { buff->state = 0; }

// cleanup shared memory:
static void cleanup_shm(int cleanup_state, int exit_code, char *argv[])
{
    switch (cleanup_state)
    {
    // cleanup semaphores
    case 6:
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
        // unmap shared memory object
        if (munmap(buff, sizeof(*buff)) == -1)
        {
            fprintf(stderr, "%s Error while unmapping shared memory object: %s\n", argv[0], strerror(errno));
        }

    case 2:
        //  close shared memory
        if (close(shm_fd) == -1)
        {
            fprintf(stderr, "%s Error while closing shared memory: %s\n", argv[0], strerror(errno));
        }

    case 1:
        // remove shared memory object
        if (shm_unlink(SHM_NAME) == -1)
        {
            fprintf(stderr, "%s Error while unlinking memory object: %s\n", argv[0], strerror(errno));
        }

    default:
        exit(exit_code);
        break;
    }
}

static int shm_setup(char *argv[])
{
    // create and/or open the shared memory object
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR | O_EXCL, 0600);
    if (shm_fd == -1)
    {
        fprintf(stderr, "%s Error while creating and/or opening a new/existing shared memory object: %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    // set the size of the shared memory
    if (ftruncate(shm_fd, 2 * sizeof(struct circ_buffer)) == -1)
    {
        fprintf(stderr, "%s Error while setting up size of shared memory: %s\n", argv[0], strerror(errno));
        cleanup_shm(1, EXIT_FAILURE, &argv[0]);
    }
    return shm_fd;
}

int main(int argc, char *argv[])
{
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

    // setup shm
    int shm_fd = shm_setup(&argv[0]);

    //  map shared memory object
    if ((buff = mmap(NULL, sizeof(*buff), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
        fprintf(stderr, "%s Error while mapping shared memory object: %s\n", argv[0], strerror(errno));
        cleanup_shm(2, EXIT_FAILURE, &argv[0]);
    }

    //  open semaphores
    free_sem = sem_open(SEM_FREE_NAME, O_CREAT | O_EXCL, 0600, BUFFER_SIZE);
    if (free_sem == SEM_FAILED)
    {
        fprintf(stderr, "%s Error while creating and/or opening (a) new/existing semaphore(s) free: %s\n", argv[0], strerror(errno));
        cleanup_shm(3, EXIT_FAILURE, &argv[0]);
    }
    used_sem = sem_open(SEM_USED_NAME, O_CREAT | O_EXCL, 0600, 0);
    if (used_sem == SEM_FAILED)
    {
        fprintf(stderr, "%s Error while creating and/or opening (a) new/existing semaphore(s) used: %s\n", argv[0], strerror(errno));
        cleanup_shm(4, EXIT_FAILURE, &argv[0]);
    }
    blocked_sem = sem_open(SEM_BLOCKED_NAME, O_CREAT | O_EXCL, 0600, 1);
    if (blocked_sem == SEM_FAILED)
    {
        fprintf(stderr, "%s Error while creating and/or opening (a) new/existing semaphore(s) blocked: %s\n", argv[0], strerror(errno));
        cleanup_shm(5, EXIT_FAILURE, &argv[0]);
    }

    int rd_pos = 0;

    // set best number of edges to max (== 8)
    int best_number_edges = 9;

    // set state to up
    buff->state = 1;
    buff->wr_pos = 0;
    bool cancel = 0;
    while (buff->state)
    {
        // check for state 0 again
        if (!buff->state)
        {
            break;
        }

        if (sem_wait(used_sem) == -1)
        {
            if (errno != EINTR)
            {
                fprintf(stderr, "%s Error while semaphore is waiting: %s\n", argv[0], strerror(errno));
                exit(EXIT_FAILURE);
            }

            // true if "cancelled" program with SIGINT
            cancel = 1;
        }

        struct element el = buff->buffer[rd_pos];

        // only compare if not canceled with SIGINT
        if (el.edge_number < best_number_edges && !cancel)
        {
            best_number_edges = el.edge_number;
            if (best_number_edges == 0)
            {
                printf("This graph is already acyclic!\n");
                buff->state = 0;
            }
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
        rd_pos++;
        rd_pos %= BUFFER_SIZE;
    }
    cleanup_shm(6, EXIT_SUCCESS, &argv[0]);

    return 0;
}
