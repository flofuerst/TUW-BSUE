/**
 * @file generator.c
 * @author Florian Fuerst (12122096)
 * @brief Checks if the input is a palindrom and returns the appropriate results
 * @date 2022-11-04
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <time.h>

static int createEdges(char *input, int number_edges, char *edge[number_edges][2])
{
    // stores rest of the string
    char *full_rest = NULL;

    // get the first edge (example: 1-2)
    char *full_edge_token = strtok_r(input, " ", &full_rest);

    int counter = 0;
    int maxVertexValue = 0;

    // loop as long as edges are in the rest of the string
    while (full_edge_token != NULL)
    {
        // stores rest of the string
        char *single_rest = NULL;

        // get first vertex of edge
        char *single_edge_token = strtok_r(full_edge_token, "-", &single_rest);
        char *first = single_edge_token;

        // convert current first vertex from string to int
        int firstValue = atoi(first);
        // check if current (first) vertex of edge has the highest value (index) of all vertices
        if (firstValue > maxVertexValue)
            maxVertexValue = firstValue;

        // pass NULL to continue splitting string with strtok_r and get second vertex of edge
        char *second = single_edge_token = strtok_r(NULL, " ", &single_rest);

        // convert current second vertex from string to int
        int secondValue = atoi(second);
        // check if current (second) vertex of edge has the highest value (index) of all vertices
        if (secondValue > maxVertexValue)
            maxVertexValue = secondValue;

        // store vertices of edges in array
        edge[counter][0] = first;
        edge[counter++][1] = second;

        // pass NULL to continue splitting string with strtok_r and get other edges
        full_edge_token = strtok_r(NULL, " ", &full_rest);
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
    printf("shuffled\n");
}

static bool inOrder(char *v1, char *v2, int vertices[], int maxIndex)
{
    int index_v1, index_v2;

    // convert string to int
    int value_v1 = atoi(v1);
    int value_v2 = atoi(v2);

    // find indices of values
    for (int i = 0; i < maxIndex + 1; i++)
    {
        if (vertices[i] == value_v1)
            index_v1 = i;
        else if (vertices[i] == value_v2)
            index_v2 = i;
    }

    // check if values are in topological order
    return index_v1 < index_v2;
}

static int find_fb_arc_set(int vertices[], int number_edges, char *edge[number_edges][2], int maxIndex, char *fb_arc_set[8][2])
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

int main(int argc, char *argv[])
{
    //check if at least one edge is given
    if (argc == 0)
    {
        printf("You have to specify at least one edge!");
        return EXIT_FAILURE;
    }

    //index to count through all given arguments
    int count = 1;

    int length = 0;

    //count number of chars in all given arguments
    while (count < argc)
    {
        length += strlen(argv[count++]);
    }

    //add number of whitespaces which are needed between edges to length
    length += argc - 2;

    //reset count
    count = 1;

    //create input char array based on determined length
    char input[length];

    int index = 0;

    //loop through all arguments
    while (count < argc)
    {
        int i = 0;

        //loop through all chars of current argument
        while (argv[count][i])
        {
            //add current char of current argument to char array
            input[index++] = argv[count][i++];
        }

        //increment counter for arguments
        count++;

        //add whitespace to char array (only if not last char of current argument)
        if (index < length)
        {
            input[index++] = ' ';
        }
    }

    // char input[] = {"0-1 1-2 1-3 1-4 2-4 3-6 4-3 4-5 6-0"};
    //  char input[] = {"0-1 1-2 1-3 1-5"};
    //   char input[] = {"0-2 0-9 0-11 1-4 3-2 3-6 4-2 4-9 5-2 5-11 6-2 6-4 7-2 7-4 7-5 7-8 7-16 7-17 8-9 8-12 8-17 10-2 10-9 11-2 12-1 12-6 12-10 13-5 13-6 13-8 14-4 14-12 15-8 15-11 15-13 16-1 16-6 16-17 17-6 17-10 17-11 18-7 18-8 18-11"};

    // count occurrence of '-' (and therefore number of edges)
    int number_edges;
    char *temp_input = strdup(input);
    for (number_edges = 0; temp_input[number_edges]; temp_input[number_edges] == '-' ? number_edges++ : *temp_input++)
    {
    }

    // initialize 2-d array
    char *edge[number_edges][2];

    // stores edges based on input in array and also returns max value (index) of vertices
    int maxIndex = createEdges(input, number_edges, edge);

    int fb_amount, best_fb_amount = 99;
    char *fb_arc_set[8][2];

    // init random number generator with seed which consists of process id and time for rand in shuffle
    srand(time(NULL) ^ getpid());

    // create array with all occurring vertices stored as a topological order
    int vertices[maxIndex + 1];
    for (int i = 0; i < maxIndex + 1; i++)
    {
        vertices[i] = i;
    }

    // find fb_arc_set of ascending (start) array in first run of loop and then loop with shuffled array;
    //  the already shuffled array gets shuffled again every loop
    do
    {

        for (int i = 0; i < maxIndex + 1; i++)
        {
            printf("%d ", vertices[i]);
        }
        printf("\n");

        fb_amount = find_fb_arc_set(vertices, number_edges, edge, maxIndex, fb_arc_set);
        if (fb_amount < best_fb_amount)
            best_fb_amount = fb_amount;

        // shuffle vertices-array
        shuffle(vertices, maxIndex);

        for (int i = 0; i < maxIndex + 1; i++)
        {
            printf("%d ", vertices[i]);
        }
        printf("\n");

        printf("\n%d\n", fb_amount);
        for (int i = 0; i < fb_amount; i++)
        {
            printf("\n%s %s", fb_arc_set[i][0], fb_arc_set[i][1]);
        }

        printf("\n --------best: %d -------- \n", best_fb_amount);
    } while (fb_amount > 0);

    return 0;
}
