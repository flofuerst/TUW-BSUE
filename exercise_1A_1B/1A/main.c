/**
 * @file main.c
 * @author Florian Fuerst (12122096)
 * @brief Checks if the input is a palindrom and returns the appropriate results
 * @date 2022-11-01
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <errno.h>

/**
 * @brief Checks if string is a palindrom
 *
 * @param input The string which gets modified
 * @return true if input string is a palindrom
 * @return false if input string is not a palindrom
 */
static bool isPalindrom(char *input)
{
    size_t strLength = strlen(input);
    if (strLength == 0)
        return false;

    int i = 0;

    // compare char at the front with char at the back and then go to next char
    while (i < strLength / 2)
    {
        char charBegin = input[i];
        char CharEnd = input[strLength - 1 - i];

        // return false if char at the front is not the same as char at the back
        if (charBegin != CharEnd)
            return false;
        i++;
    }

    return true;
}

/**
 * @brief Sets all letters to lowercase
 *
 * @param input The string which gets modified
 */
static void lowerLetters(char *input)
{
    while ((*input = tolower(*input)))
    {
        input++;
    }
}

/**
 * @brief Removes all whitespaces from string
 *
 * @param input The string which gets modified
 */
static void removeWhitespaces(char *input)
{
    char *temp_input = input;

    // loop till null character is reached
    while (*input)
    {
        // skip all whitespaces
        while (*temp_input == ' ')
        {
            temp_input++;
        }

        // replace char of input string with char of new string (which has no whitespaces) and go to next char of string
        *input++ = *temp_input++;
    }
}

/**
 * @brief Removes newspaces at end of line and replace it with '\0'
 *
 * @param input The string which gets modified
 */
static void removeNewspaces(char *input)
{
    size_t strLength = strlen(input);
    if (strLength == 0)
        exit(EXIT_FAILURE);

    if (input[strLength - 1] == '\n')
        input[strLength - 1] = '\0';
}

/**
 * @brief reads the input file (or stdin) line by line, removes Newspaces at the end and prints result of
 * isPalindrom into outfile
 *
 * @param input The given input file(s) or input from stdin
 * @param caseInsensitive Bool if strings should be case insensitive
 * @param ignoreWhitespaces Bool if whitespaces should be ignored in string
 * @param outfile The file where the result of isPalindrom should be written; It is either a file or stdout
 */
static void readInputFile(FILE *input, bool caseInsensitive, bool ignoreWhitespaces, FILE *outfile, char *argv[])
{
    // string and size to read lines with getline
    char *line = NULL;
    size_t size = 0;

    bool palindrom;
    // read line by line
    while (getline(&line, &size, input) != -1)
    {
        // remove newspaces at end of line (input string cannot be modified so it needs to be duplicated)
        char *newLine = strdup(line);
        removeNewspaces(newLine);

        // duplicate new string again to apply modifications according to the given options
        char *tempLine = strdup(newLine);
        if (caseInsensitive)
            lowerLetters(tempLine);
        if (ignoreWhitespaces)
            removeWhitespaces(tempLine);

        // check modified string for palindrom
        palindrom = isPalindrom(tempLine);

        // print result with original string (not with modified string)
        if (palindrom)
            fprintf(outfile, "%s is a palindrom\n", newLine);
        else
            fprintf(outfile, "%s is not a palindrom\n", newLine);

        free(newLine);
        free(tempLine);
    }

    // line needs to get freed
    free(line);

    // close file
    if (fclose(input) == EOF)
    {
        fprintf(stderr, "%s Error while closing file! %s", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief checks correct program call; takes actions based on the specief options; calls appropriate functions to
 * make the program work how it should;
 *
 * @param argc number of arguments of the program call
 * @param argv array of the arguments of the program call
 * @return int return value of main method to tell when program is finished (and therefore can be removed from memory)
 */
int main(int argc, char *argv[])
{
    char *outfile = NULL;
    bool caseInsensitive = false;
    bool ignoreWhitespaces = false;

    // get options and arguments
    int c;
    while ((c = getopt(argc, argv, "sio:")) != -1)
    {
        switch (c)
        {
        case 's':
            ignoreWhitespaces = true;
            break;
        case 'i':
            caseInsensitive = true;
            break;
        case 'o':
            outfile = optarg;
            break;
        default:
            fprintf(stderr, "SYNOPSIS:\n%s [-s] [-i] [-o outfile] [file...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    FILE *output = stdout;
    if (outfile != NULL)
    {
        output = fopen(outfile, "w");

        if (output == NULL)
        {
            fprintf(stderr, "%s Error while opening file: %s\n", argv[0], strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // read from stdin if no input file given
    if (argc - optind == 0)
    {
        // read content from stdin
        while (true)
        {
            readInputFile(stdin, caseInsensitive, ignoreWhitespaces, output, &argv[0]);
        }
    }
    // read from input file(s) (if given)
    else
    {
        int i = optind;
        while (i < argc)
        {
            // open file
            FILE *f = fopen(argv[i], "r");
            if (f == NULL)
            {
                fprintf(stderr, "%s Error while opening file %s: %s\n", argv[0], argv[i], strerror(errno));
                exit(EXIT_FAILURE);
            }

            // read content from file
            readInputFile(f, caseInsensitive, ignoreWhitespaces, output, &argv[0]);
            i++;
        }
    }
    if (fclose(output) == EOF)
    {
        fprintf(stderr, "%s Error while closing file! %s", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    return 0;
}
