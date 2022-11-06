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

int createEdges(char *input, int number_edges, char *edge[number_edges][2])
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
void shuffle(int vertices[], int maxIndex)
{
    int i, j, temp;

    // init random with time
    srand(time(NULL));

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

bool inOrder(char *v1, char *v2, int vertices[], int maxIndex)
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

int find_fb_arc_set(int vertices[], int number_edges, char *edge[number_edges][2], int maxIndex, char *fb_arc_set[8][2])
{
    int fb_counter = 0;

    //add all edges which are not in order to fb arc set and increment fb_counter
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
    char input[] = {"0-1 1-2 1-3 1-4 2-4 3-6 4-3 4-5 6-0"};

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

    // create array with all occurring vertices stored as a topological order
    int vertices[maxIndex + 1];
    for (int i = 0; i < maxIndex + 1; i++)
    {
        vertices[i] = i;
    }

    // shuffle vertices-array
    shuffle(vertices, maxIndex);

    for (int i = 0; i < maxIndex + 1; i++)
    {
        printf("%d", vertices[i]);
    }

    char *fb_arc_set[8][2];

    int fb_amount = find_fb_arc_set(vertices, number_edges, edge, maxIndex, fb_arc_set);
    printf("\n%d", fb_amount);

    for (int i = 0; i < fb_amount; i++)
    {
        printf("\n%s %s", fb_arc_set[i][0], fb_arc_set[i][1]);
    }

    return 0;
}
