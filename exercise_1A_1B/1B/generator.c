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

    //create array with all occurring vertices stored as a topological order
    int vertices[maxIndex + 1];
    for (int i = 0; i < maxIndex + 1; i++)
    {
        vertices[i] = i;
    }

    return 0;
}
