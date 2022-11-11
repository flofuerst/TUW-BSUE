/**
 * @file supervisor.c
 * @author Florian Fuerst (12122096)
 * @brief Checks if the input is a palindrom and returns the appropriate results
 * @date 2022-11-04
 *
 */

#include "circular_buffer.h"

void handle_signal(int signal) { buff->state = 0; }

// cleanup shared memory:
void cleanup_shm(int cleanup_state, int exit_code)
{
    switch (cleanup_state)
    {
    // cleanup semaphores
    case 6:
        if (sem_close(blocked_sem) == -1)
        {
            printf("Error while closing semaphore %d\n", errno);
        }
        if (sem_unlink(SEM_BLOCKED_NAME) == -1)
        {
            printf("Error while unlinking semaphore %d\n", errno);
        }

    case 5:
        if (sem_close(used_sem) == -1)
        {
            printf("Error while closing semaphore %d\n", errno);
        }
        if (sem_unlink(SEM_USED_NAME) == -1)
        {
            printf("Error while unlinking semaphore %d\n", errno);
        }

    case 4:
        if (sem_close(free_sem) == -1)
        {
            printf("Error while closing semaphore %d\n", errno);
        }
        if (sem_unlink(SEM_FREE_NAME) == -1)
        {
            printf("Error while unlinking semaphore %d\n", errno);
        }
 
    // cleanup shm
    case 3:
        // unmap shared memory object
        if (munmap(buff, sizeof(*buff)) == -1)
        {
            printf("Error while unmapping shared memory object %d\n", errno);
        }

    case 2:
        //  close shared memory
        if (close(shm_fd) == -1)
        {
            printf("Error while closing shared memory %d\n", errno);
        }

    case 1:
        // remove shared memory object
        if (shm_unlink(SHM_NAME) == -1)
        {
            printf("Error while unlinking memory object %d\n", errno);
        }

    default:
        exit(exit_code);
        break;
    }
}

static int shm_setup(void)
{
    // create and/or open the shared memory object
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR | O_EXCL, 0600);
    if (shm_fd == -1)
    {
        printf("Error while creating and/or opening a new/existing shared memory object %d\n", errno);
        exit(EXIT_FAILURE);
    }

    // set the size of the shared memory
    if (ftruncate(shm_fd, 2 * sizeof(struct circ_buffer)) == -1)
    {
        printf("Error while setting up size of shared memory %d\n", errno);
        cleanup_shm(1, EXIT_FAILURE);
    }
    return shm_fd;
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        printf("No arguments allowed!\n");
        exit(EXIT_FAILURE);
    }

    // set signal handler
    struct sigaction sa = {.sa_handler = handle_signal};
    if (sigaction(SIGINT, &sa, NULL) + sigaction(SIGTERM, &sa, NULL) < 0)
    {
        printf("Error while initializing signal handler %d\n", errno);
        exit(EXIT_FAILURE);
    }

    //setup shm
    int shm_fd = shm_setup();

    //  map shared memory object
    if ((buff = mmap(NULL, sizeof(*buff), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
        printf("Error while mapping shared memory object\n");
        cleanup_shm(2, EXIT_FAILURE);
    }

    //  open semaphores
    free_sem = sem_open(SEM_FREE_NAME, O_CREAT | O_EXCL, 0600, BUFFER_SIZE);
    if (free_sem == SEM_FAILED)
    {
        printf("Error while creating and/or opening (a) new/existing semaphore(s) free %d\n", errno);
        cleanup_shm(3, EXIT_FAILURE);
    }
    used_sem = sem_open(SEM_USED_NAME, O_CREAT | O_EXCL, 0600, 0);
    if (used_sem == SEM_FAILED)
    {
        printf("Error while creating and/or opening (a) new/existing semaphore(s) used\n");
        cleanup_shm(4, EXIT_FAILURE);
    }
    blocked_sem = sem_open(SEM_BLOCKED_NAME, O_CREAT | O_EXCL, 0600, 1);
    if (blocked_sem == SEM_FAILED)
    {
        printf("Error while creating and/or opening (a) new/existing semaphore(s) blocked\n");
        cleanup_shm(5, EXIT_FAILURE);
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
        //check for state 0 again
        if (!buff->state)
        {
            printf("success\n");
            break;
        }

        if (sem_wait(used_sem) == -1)
        {
            if (errno != EINTR)
            {
                printf("Error while semaphore is waiting\n");
                exit(EXIT_FAILURE);
            }

            //true if "cancelled" program with SIGINT
            cancel = 1;
        }

        struct element el = buff->buffer[rd_pos];

        //only compare if not canceled with SIGINT
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
                    printf("%d-%d\n", el.fb_arc_set[i].vertex_u, el.fb_arc_set[i].vertex_v);
                }
            }
            printf("\n----------------\n\n");
        }
        sem_post(free_sem); 
        rd_pos++;
        rd_pos %= BUFFER_SIZE;
    }
    cleanup_shm(6, EXIT_SUCCESS);

    return 0;
}
