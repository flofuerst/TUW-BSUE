/**
 * @file generator.c
 * @author Florian Fuerst (12122096)
 * @brief Checks if the input is a palindrom and returns the appropriate results
 * @date 2022-11-04
 *
 */

#include "circular_buffer.h"

int state = 0;
static void handle_signal(int signal)
{
    state = 0;
    buff->state = 0;
}

// cleanup shared memory:
static void cleanup_shm(int cleanup_state, int exit_code, char *argv[])
{
    switch (cleanup_state)
    {
    // cleanup semaphores
    case 5:
        if (sem_close(blocked_sem) == -1)
        {
            fprintf(stderr, "%s Error while closing semaphore: %s\n", argv[0], strerror(errno));
        }

    case 4:
        if (sem_close(used_sem) == -1)
        {
            fprintf(stderr, "%s Error while closing semaphore: %s\n", argv[0], strerror(errno));
        }

    case 3:
        if (sem_close(free_sem) == -1)
        {
            fprintf(stderr, "%s Error while closing semaphore: %s\n", argv[0], strerror(errno));
        }

    // cleanup shm
    case 2:
        // unmap shared memory object
        if (munmap(buff, sizeof(*buff)) == -1)
        {
            fprintf(stderr, "%s Error while unmapping shared memory object: %s\n", argv[0], strerror(errno));
        }

    case 1:
        //  close shared memory
        if (close(shm_fd) == -1)
        {
            fprintf(stderr, "%s Error while closing shared memory: %s\n", argv[0], strerror(errno));
        }

    default:
        exit(exit_code);
        break;
    }
}

static int create_edges(char *input, int number_edges, long edge[number_edges][2], char *argv[])
{
    // stores rest of the string
    char *full_rest = NULL;

    // get the first edge (example: 1-2)
    char *full_edge_token = strtok_r(input, " \n", &full_rest);

    int counter = 0;
    int maxVertexValue = 0;

    char *end_char;

    // loop as long as edges are in the rest of the string
    while (full_edge_token != NULL)
    {
        // stores rest of the string
        char *single_rest = NULL;

        // get first vertex of edge
        char *single_edge_token = strtok_r(full_edge_token, "-", &single_rest);
        char *first = single_edge_token;

        // convert current first vertex from string to int
        long firstValue = strtol(first, &end_char, 10);

        // check if current (first) vertex of edge has the highest value (index) of all vertices
        if (firstValue > maxVertexValue)
            maxVertexValue = firstValue;

        // pass NULL to continue splitting string with strtok_r and get second vertex of edge
        char *second = single_edge_token = strtok_r(NULL, " ", &single_rest);

        // convert current second vertex from string to int
        long secondValue = strtol(second, &end_char, 10);

        // check if current (second) vertex of edge has the highest value (index) of all vertices
        if (secondValue > maxVertexValue)
            maxVertexValue = secondValue;

        // check if vertex is connected to itself --> not allowed
        if (firstValue == secondValue)
        {
            fprintf(stderr, "%s Invalid Edge! At least one vertex is connected to itself!\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        // check if same edge already exists; without this for-loop code works perfectly fine too, but duplicates of
        // edges get saved in edge-array too --> doesn't matter but is maybe inefficient
        for (int i = 0; i < counter; i++)
        {
            if (firstValue == edge[i][0] && secondValue == edge[i][1])
            {
                fprintf(stderr, "%s Invalid Edge! At least one edge already exists!\n", argv[0]);
                exit(EXIT_FAILURE);
            }
        }

        // store vertices of edges in array
        edge[counter][0] = firstValue;
        edge[counter++][1] = secondValue;

        // pass NULL to continue splitting string with strtok_r and get other edges
        full_edge_token = strtok_r(NULL, " \n", &full_rest);
    }

    return maxVertexValue;
}

// shuffle array list using fisher-yates shuffle
static void shuffle(int vertices[], int maxIndex)
{
    int i, j, temp;

    // call rand one time before usage to fix pseudo random effect (often same numbers when first called)
    rand();

    for (i = maxIndex; i > 0; i--)
    {
        j = rand() % (maxIndex + 1);

        // shuffle values
        temp = vertices[j];
        vertices[j] = vertices[i];
        vertices[i] = temp;
    }
}

static bool inOrder(long value_1, long value_2, int vertices[], int maxIndex)
{
    int index_v1, index_v2;

    // find indices of values
    for (int i = 0; i < maxIndex + 1; i++)
    {
        if (vertices[i] == value_1)
            index_v1 = i;
        else if (vertices[i] == value_2)
            index_v2 = i;
    }

    // check if values are in topological order
    return index_v1 < index_v2;
}

static int find_fb_arc_set(int vertices[], int number_edges, long edge[number_edges][2], int maxIndex, long fb_arc_set[8][2])
{
    // init fb_counter
    int fb_counter = 0;

    // add all edges which are not in order to fb arc set and increment fb_counter
    for (int i = 0; i < number_edges; i++)
    {
        if (!inOrder(edge[i][0], edge[i][1], vertices, maxIndex))
        {
            fb_arc_set[fb_counter][0] = edge[i][0];
            fb_arc_set[fb_counter++][1] = edge[i][1];
        }

        if (fb_counter >= 8)
            break;
    }
    return fb_counter;
}

static void shm_sem_setup(char *argv[])
{
    // open the shared memory object
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0600);
    if (shm_fd == -1)
    {
        fprintf(stderr, "%s Error while opening an existing shared memory object: %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((buff = mmap(NULL, sizeof(*buff), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
        fprintf(stderr, "%s Error while mapping shared memory object: %s\n", argv[0], strerror(errno));
        cleanup_shm(1, EXIT_FAILURE, &argv[0]);
    }

    // open semaphores
    free_sem = sem_open(SEM_FREE_NAME, BUFFER_SIZE);
    if (free_sem == SEM_FAILED)
    {
        fprintf(stderr, "%s Error while creating and/or opening (a) new/existing semaphore(s) free: %s\n", argv[0], strerror(errno));
        // fprintf(stderr, "%s Input file %s not found\n", argv[0], argv[i]);
        cleanup_shm(2, EXIT_FAILURE, &argv[0]);
    }
    used_sem = sem_open(SEM_USED_NAME, 0);
    if (used_sem == SEM_FAILED)
    {
        fprintf(stderr, "%s Error while creating and/or opening (a) new/existing semaphore(s) used: %s\n", argv[0], strerror(errno));
        cleanup_shm(3, EXIT_FAILURE, &argv[0]);
    }
    blocked_sem = sem_open(SEM_BLOCKED_NAME, 1);
    if (blocked_sem == SEM_FAILED)
    {
        fprintf(stderr, "%s Error while creating and/or opening (a) new/existing semaphore(s) blocked: %s\n", argv[0], strerror(errno));
        cleanup_shm(4, EXIT_FAILURE, &argv[0]);
    }
}

int main(int argc, char *argv[])
{

    // check if at least one edge is given
    if (argc <= 1)
    {
        fprintf(stderr, "%s You have to specify at least one edge!\n", argv[0]);
        return EXIT_FAILURE;
    }

    // index to count through all given arguments
    int count = 1;

    int length = 0;

    // count number of chars in all given arguments
    while (count < argc)
    {
        length += strlen(argv[count++]);
    }

    // add number of whitespaces which are needed between edges to length
    length += argc - 2;

    // reset count
    count = 1;

    // create input char array based on determined length
    char input[length];

    int index = 0;

    // loop through all arguments
    while (count < argc)
    {
        int i = 0;

        // loop through all chars of current argument
        while (argv[count][i])
        {
            // add current char of current argument to char array
            input[index++] = argv[count][i++];
        }

        // increment counter for arguments
        count++;

        // add whitespace to char array (only if not last char of current argument)
        if (index < length)
        {
            input[index++] = ' ';
        }
    }

    /* short explanation why arguments get appended to string in code above and get splitted in create_edges-methode
    below: implementation of code below happened first, so it was necessary to "build" the string in the code above */

    // count occurrence of '-' (and therefore number of edges)
    int number_edges;
    char *temp_input = strdup(input);

    // free is not necessary/possible because string "deletes" itself
    for (number_edges = 0; temp_input[number_edges]; temp_input[number_edges] == '-' ? number_edges++ : *temp_input++)
    {
    }

    // initialize 2-d array
    long edge[number_edges][2];

    // stores edges based on input in array and also returns max value (index) of vertices
    int maxIndex = create_edges(input, number_edges, edge, &argv[0]);

    int fb_amount, best_fb_amount = 99;
    long fb_arc_set[8][2];

    // init random number generator with seed which consists of process id and time for rand in shuffle
    srand(time(NULL) ^ getpid());

    // create array with all occurring vertices stored as a topological order
    int vertices[maxIndex + 1];
    for (int i = 0; i < maxIndex + 1; i++)
    {
        vertices[i] = i;
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

    //  find fb_arc_set of ascending (start) array in first run of loop and then loop with shuffled array;
    //   the already shuffled array gets shuffled again every loop
    while (buff->state)
    {
        fb_amount = find_fb_arc_set(vertices, number_edges, edge, maxIndex, fb_arc_set);
        if (fb_amount < best_fb_amount)
            best_fb_amount = fb_amount;

        if (sem_wait(blocked_sem) == -1)
        {
            fprintf(stderr, "%s Error while semaphore is waiting: %s\n", argv[0], strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (!buff->state)
        {
            sem_post(used_sem);
            break;
        }

        if (sem_wait(free_sem) == -1)
        {
            sem_post(blocked_sem);
            sem_post(used_sem);
            if (errno != EINTR)
            {
                fprintf(stderr, "%s Error while semaphore is waiting: %s\n", argv[0], strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < fb_amount; i++)
        {
            printf("writing to wr_pos: %d, fb_arc_set-index: %d, edges: %ld-%ld\n", buff->wr_pos, i, fb_arc_set[i][0], fb_arc_set[i][1]);

            buff->buffer[buff->wr_pos].fb_arc_set[i].vertex_u = fb_arc_set[i][0];
            buff->buffer[buff->wr_pos].fb_arc_set[i].vertex_v = fb_arc_set[i][1];
        }
        printf("number of edges: %d\n", fb_amount);

        buff->buffer[buff->wr_pos].edge_number = fb_amount;
        buff->wr_pos++;
        buff->wr_pos %= BUFFER_SIZE;

        sem_post(used_sem);
        sem_post(blocked_sem);

        //------------------------
        // shuffle vertices-array
        shuffle(vertices, maxIndex);
    }
    sem_post(used_sem);
    cleanup_shm(5, EXIT_SUCCESS, &argv[0]);

    return 0;
}
