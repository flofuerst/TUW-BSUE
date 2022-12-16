/**
 * @file intmul.c
 * @author Florian Fürst (12122096)
 * @brief multiplies two hexadecimal integers using fork and pipes
 * @version 0.1
 * @date 2022-12-11
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

int outPipe[4][2];
int childindex[4];

int forks = 0;

/**
 * @brief adds leading zeros to A and B if A and B have not the equal length or the number of digits is not a power of two
 *
 * @param correctFirstInput char array of first valid input
 * @param correctSecondInput char array of second valid input
 * @param firstHexInt string of first non valid input
 * @param secondHexInt string of second non valid input
 * @param neededLength the length of the valid strings
 */
static void inputCorrection(char correctFirstInput[], char correctSecondInput[],
                            char *firstHexInt, char *secondHexInt, int neededLength)
{
    int lenFirst = strlen(firstHexInt);
    int lenSecond = strlen(secondHexInt);

    for (int i = 0; i < neededLength - lenFirst; i++)
    {
        correctFirstInput[i] = '0';
    }
    correctFirstInput[neededLength - lenFirst] = '\0';

    for (int i = 0; i < neededLength - lenSecond; i++)
    {
        correctSecondInput[i] = '0';
    }
    correctSecondInput[neededLength - lenSecond] = '\0';

    // build up string
    strcat(correctFirstInput, firstHexInt);
    strcat(correctSecondInput, secondHexInt);
}

/**
 * @brief converts a hexadecimal string to an integer
 *
 * @param hexInt hexadecimal string
 * @param argv simple hand over of program name argv[0] for error messages
 * @return int the converted integer
 */
static int convertHexToInt(char *hexInt, char *argv)
{
    int hexAsInt;

    // convert single hexadecimal integer (char) to int
    hexAsInt = strtol(hexInt, NULL, 16);

    return hexAsInt;
}

/**
 * @brief converts a hexadecimal char to an integer
 *
 * @param hexInt hexadecimal string
 * @param argv simple hand over of program name argv[0] for error messages
 * @return int the converted integer
 */
static int convertSingleHexToInt(char hexInt, char *argv)
{
    int hexAsInt;

    // convert single hexadecimal integer (char) to int
    if (hexInt >= 'A' && hexInt <= 'F')
    {
        hexAsInt = (hexInt - 'A' + 10);
    }
    else if (hexInt >= 'a' && hexInt <= 'f')
    {
        hexAsInt = (hexInt - 'a' + 10);
    }
    else
    {
        hexAsInt = (hexInt - '0');
    }

    return hexAsInt;
}

/**
 * @brief converts a single integer to hexadecimal char
 *
 * @param number the integer which needs to be converted
 * @param argv simple hand over of program name argv[0] for error messages
 * @return char the converted char
 */
static char convertIntToSingleHex(int number, char *argv)
{
    number %= 16;
    char output;
    // check if input is valid hexadecimal integer
    if (number < 0 || number > 15)
    {
        fprintf(stderr, "%s Invalid hexadecimal integer! %d\n ", argv, number);
        exit(EXIT_FAILURE);
    }

    // convert int to hexadecimal integer (char)
    if (number >= 10)
    {
        output = 'a' + (number - 10);
    }
    else
    {
        output = '0' + number;
    }

    return output;
}

/**
 * @brief Split the given string in two, equal long, strings
 *
 * @param wholeString the whole string which needs to be splitted
 * @param part_h one half of the string
 * @param part_l one half of the string
 * @param length the length of the whole string
 * @param length_h the length of one string part
 * @param length_l the length of one string part
 */
static void createHalfStrings(char wholeString[], char part_h[], char part_l[], int length, int length_h, int length_l)
{
    for (int i = 0; i < length_h; i++)
    {
        part_h[i] = wholeString[i];
    }
    part_h[length_h] = '\0';

    for (int i = 0, x = length_h; i < length_l; i++, x++)
    {
        part_l[i] = wholeString[x];
    }
    part_l[length_l] = '\0';
}

/**
 * @brief Method to write values into a pipe
 *
 * @param fd the file descriptor where values needs to be written
 * @param A char array A, which gets written into pipe
 * @param B char array B, which gets written into pipe
 * @param argv simple hand over of program name argv[0] for error messages
 */
static void writeToPipe(int fd, char A[], char B[], char *argv)
{
    FILE *out = fdopen(fd, "w");
    fprintf(out, "%s\n", A);
    fprintf(out, "%s\n", B);

    fflush(out);

    if (fclose(out) == EOF)
    {
        fprintf(stderr, "%s Error while closing file! %s\n", argv, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Method to initialize forking and pipe; also calls other method to write into pipe
 *
 * @param A char array A, which gets parsed to children
 * @param B char array B, which gets parsed to children
 * @param length the length of the input string
 * @param argv simple hand over of program name argv[0] for error messages
 * @param pid process id for forking
 */
static void forkAndPipe(char A[], char B[], int length, char *argv, pid_t pid[])
{
    int length_h = length / 2;
    int length_l = length / 2;

    // length + 1 because of \0
    char Ah[length_h + 1];
    char Al[length_l + 1];
    char Bh[length_h + 1];
    char Bl[length_l + 1];

    createHalfStrings(A, Ah, Al, length, length_h, length_l);
    createHalfStrings(B, Bh, Bl, length, length_h, length_l);

    int inPipe[4][2];

    for (int i = 0; i < 4; i++)
    {
        pipe(inPipe[i]);
        pipe(outPipe[i]);

        forks++;
        pid[i] = fork();

        switch (pid[i])
        {
        case -1:
            fprintf(stderr, "%s Cannot fork!\n", argv);
            exit(EXIT_FAILURE);
            break;

        // child tasks
        case 0:
            close(inPipe[i][1]);
            close(outPipe[i][0]);

            if (dup2(inPipe[i][0], STDIN_FILENO) == -1 || dup2(outPipe[i][1], STDOUT_FILENO) == -1)
            {
                fprintf(stderr, "%s Cannot redirect pipe!\n", argv);
                exit(EXIT_FAILURE);
            }

            close(inPipe[i][0]);
            close(outPipe[i][1]);

            if (execlp("./intmul", "./intmul", NULL) == -1)
            {
                fprintf(stderr, "%s Unable to execlp\n", argv);
                exit(EXIT_FAILURE);
            }
            break;

        // parent tasks
        default:
            close(inPipe[i][0]);
            close(outPipe[i][1]);
            int fd = inPipe[i][1];

            switch (i)
            {
            case 0:
                writeToPipe(fd, Ah, Bh, argv);
                break;
            case 1:
                writeToPipe(fd, Ah, Bl, argv);
                break;
            case 2:
                writeToPipe(fd, Al, Bh, argv);
                break;
            case 3:
                writeToPipe(fd, Al, Bl, argv);
                break;
            default:
                fprintf(stderr, "%s Unallowed default-case reached\n", argv);
                exit(EXIT_FAILURE);
                break;
            }

            break;
        }
    }
}

/**
 * @brief Method to read values from pipe
 *
 * @param result the array where the results (from the reading process) get written into
 * @param argv simple hand over of program name argv[0] for error messages
 */
static void readFromPipe(char *result[], char *argv)
{
    int count = 0;

    while (count < 4)
    {
        result[count] = NULL;
        size_t size = 0, read = 0;
        FILE *out = fdopen(outPipe[count][0], "r");
        if ((read = getline(&result[count], &size, out)) == -1)
        {
            fprintf(stderr, "%s Error while reading from pipe\n", argv);
            exit(EXIT_FAILURE);
        }

        size = 0;

        childindex[count] = read;
        result[count][read - 1] = '\0';

        if (fclose(out) == EOF)
        {
            fprintf(stderr, "%s Error while closing file! %s\n", argv, strerror(errno));
            exit(EXIT_FAILURE);
        }
        count++;
    }
}

/**
 * @brief waits for each child pid to wait for
 *
 * @param pid process id which is needed to identify process
 * @param argv simple hand over of program name argv[0] for error messages
 */
static void waitForChildren(pid_t pid[], char *argv)
{
    for (int i = 0; i < 4; i++)
    {
        int status;

        if (waitpid(pid[i], &status, 0) == -1)
        {
            fprintf(stderr, "%s Error while waiting for child\n", argv);
            exit(EXIT_FAILURE);
        }

        if (WEXITSTATUS(status) != EXIT_SUCCESS)
        {
            fprintf(stderr, "%s Error while child exited\n", argv);
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief Method to merge calculation-results of childs
 *
 * @param result the array where the results are written into
 * @param resultsOfChildren the different results from all childs
 * @param length the length of the input
 * @param argv simple hand over of program name argv[0] for error messages
 */
static void calculateResult(char result[], char *resultsOfChildren[], int length, char *argv)
{
    int ahbh_index = childindex[0] - 2;
    int ahbl_index = childindex[1] - 2;
    int albh_index = childindex[2] - 2;
    int albl_index = childindex[3] - 2;

    int carry = 0;

    for (int i = length * 2 - 1; i >= 0; i--)
    {
        if (i >= length + length / 2)
        {
            result[i] = albl_index >= 0 ? resultsOfChildren[3][albl_index--] : 0;
        }
        else if (i >= length) // ahbh0, ahbl1, albh2, albl3
        {
            int ahbl = ahbl_index >= 0 ? convertSingleHexToInt(resultsOfChildren[1][ahbl_index--], argv) : 0;
            int albh = albh_index >= 0 ? convertSingleHexToInt(resultsOfChildren[2][albh_index--], argv) : 0;
            int albl = albl_index >= 0 ? convertSingleHexToInt(resultsOfChildren[3][albl_index--], argv) : 0;

            int partResult = ahbl + albh + albl + carry;

            result[i] = convertIntToSingleHex(partResult, argv);
            carry = partResult / 16;
        }
        else
        {
            int ahbh = ahbh_index >= 0 ? convertSingleHexToInt(resultsOfChildren[0][ahbh_index--], argv) : 0;
            int ahbl = ahbl_index >= 0 ? convertSingleHexToInt(resultsOfChildren[1][ahbl_index--], argv) : 0;
            int albh = albh_index >= 0 ? convertSingleHexToInt(resultsOfChildren[2][albh_index--], argv) : 0;
            int albl = albl_index >= 0 ? convertSingleHexToInt(resultsOfChildren[3][albl_index--], argv) : 0;

            int partResult = ahbh + ahbl + albh + albl + carry;

            result[i] = convertIntToSingleHex(partResult, argv);
            carry = partResult / 16;
        }
    }

    result[length * 2] = '\0';

    for (int i = 0; i < 4; i++)
    {
        free(resultsOfChildren[i]);
    }

    fprintf(stdout, "%s\n", result);
    fflush(stdout);
}

/**
 * @brief checks if the input is valid
 *
 * @param input the input which needs to be checked
 * @param argv simple hand over of program name argv[0] for error messages
 */
static void checkInput(char *input, char *argv)
{
    for (int i = 0; i < strlen(input); i++)
    {
        // remove newspaces of input
        if (input[i] == '\n')
        {
            input[i] = '\0';
            break;
        }

        if (!isdigit(input[i]) && !(input[i] >= 'A' && input[i] <= 'F') && !(input[i] >= 'a' && input[i] <= 'f'))
        {
            fprintf(stderr, "%s Input has to be a valid hexadecimal integer!\n", argv);

            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief reads input and handles the whole process of this exercise
 *
 * @param argc number of arguments of the program call
 * @param argv array of the arguments of the program call
 * @return int return value of main method to tell when program is finished (and therefore can be removed from memory)
 */
int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        fprintf(stderr, "SYNOPSIS:\n%s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t size = 0;

    char *firstHexInt;
    char *secondHexInt;

    int count = 0;

    // read line from stdin
    while (getline(&line, &size, stdin) != -1)
    {
        if (count == 0)
        {
            firstHexInt = strdup(line);
            count++;
        }

        else if (count == 1)
        {
            secondHexInt = strdup(line);
            count++;
            break;
        }
    }

    if (count != 2)
    {
        fprintf(stderr, "SYNOPSIS:\n%s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // check if input is valid
    checkInput(firstHexInt, argv[0]);
    checkInput(secondHexInt, argv[0]);

    free(line);

    int lenFirst = strlen(firstHexInt);
    int lenSecond = strlen(secondHexInt);

    if (lenFirst < 1 || lenSecond < 1)
    {
        fprintf(stderr, "%s Input of hexadecimal integers must not be nothing\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int neededLength = 1;
    int max = lenFirst > lenSecond ? lenFirst : lenSecond;
    while (neededLength < max)
    {
        neededLength *= 2;
    }

    char correctFirstInt[neededLength + 1];
    char correctSecondInt[neededLength + 1];

    // make input a 'valid' input by filling it up with missing zeros
    inputCorrection(correctFirstInt, correctSecondInt, firstHexInt, secondHexInt, neededLength);

    free(firstHexInt);
    free(secondHexInt);

    // multiply A and B if both consists of only 1 hexadecimal digit --> base-case
    if (neededLength == 1)
    {
        int first = convertHexToInt(correctFirstInt, argv[0]);
        int second = convertHexToInt(correctSecondInt, argv[0]);

        int result = first * second;

        fprintf(stdout, "%02x\n", result);
        fflush(stdout);
        exit(EXIT_SUCCESS);
    }
    pid_t pid[4];

    forkAndPipe(correctFirstInt, correctSecondInt, neededLength, argv[0], pid);

    waitForChildren(pid, argv[0]);

    char *resultsOfChildren[4];

    char result[neededLength * 2 + 1];

    readFromPipe(resultsOfChildren, argv[0]);

    calculateResult(result, resultsOfChildren, neededLength, argv[0]);

    return 0;
}