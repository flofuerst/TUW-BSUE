/**
 * @file client.c
 * @author Florian Fürst (12122096)
 * @brief Client program which uses HTTP 1.1. The client can take an URL as input, connects to the corresponding server
 * and also requests the files specified in the URL. The transmitted data can be written to stdout or to file.
 * It is also possible to specify a directory of a file, where the transmitted data should be written to.
 * @version 0.1
 * @date 2023-01-14
 *
 * @copyright Copyright (c) 2023
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
#include <getopt.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

/**
 * @brief the program name which is used for usage and error messages
 *
 */
static char *argv_name;

/**
 * @brief Prints the correct usage(synopsis) of the program to stderr
 *
 */
static void usage(void)
{
    fprintf(stderr, "SYNOPSIS:\n%s [-p PORT] [ -o FILE | -d DIR ] URL\n", argv_name);
    exit(EXIT_FAILURE);
}

/**
 * @brief Checks if the given url is correct and splits the url into the host and request parts. Corrects
 * the request string/part if it is NULL or doesn't start with an '/'. Also checks if hostname is valid (non-empty).
 *
 * @param url The whole url which is given through input
 * @param host The host of the url, which begins after the "http://" and ends before the first occurence of one of the
 * following characters ';/?:@=&'.
 * @param request The request part of the url. If it starts with an '?', a '/' gets added at the front.
 */
static void parse_url(char *url, char *host, char *request)
{
    // check if url starts with http://
    if (strncmp(url, "http://", strlen("http://")) != 0)
    {
        fprintf(stderr, "%s Error, invalid url\n", argv_name);
        exit(EXIT_FAILURE);
    }

    char host_total[strlen(url)];

    // Remove "http://" from host; host is still not correct after this
    strcpy(host_total, url + strlen("http://"));

    // Find Occurence of ";/?:@=&" characters
    char *request_temp = strpbrk(host_total, ";/?:@=&");
    // Create new request string if characters above do not exist
    if (request_temp == NULL)
    {
        request[0] = '/';
        request[1] = '\0';

        strcpy(host, host_total);
    }
    // If character(s) above exist: copy all characters after first occurrence to request string
    else
    {
        int corrected = 0;
        // add / in front of request if not existing
        if (request_temp[0] == '?')
        {
            request[0] = '/';
            request[1] = '\0';
            strcat(request, request_temp);
            corrected = 1;
        }
        else
            strcpy(request, request_temp);

        // index corrected is used to index the correct first element of request
        //(if / was missing in front, index 0 would be false)
        char *e = strchr(host_total, request[corrected]);
        int index = (int)(e - host_total);
        strncpy(host, host_total, index);
        host[index] = '\0';
    }

    // check if invalid hostname
    if (host[0] == '\0')
    {
        fprintf(stderr, "%s Error, invalid hostname\n", argv_name);
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Setup for the server connection. Returns the given file decriptor which is needed for further implementation.
 *
 * @param host The host which is used by getaddrinfo
 * @param port The port which is used by getaddrinfo
 * @return int Returns the file descriptor of the socket
 */
static int socket_setup(char *host, char *port)
{
    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int res = getaddrinfo(host, port, &hints, &ai);
    if (res != 0)
    {
        fprintf(stderr, "%s Error, while using getaddrinfo(): %s\n", argv_name, gai_strerror(res));
        exit(EXIT_FAILURE);
    }

    int socket_fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (socket_fd < 0)
    {
        fprintf(stderr, "%s Error, while using socket(): %s\n", argv_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (connect(socket_fd, ai->ai_addr, ai->ai_addrlen) < 0)
    {
        fprintf(stderr, "%s Error, while using connect(): %s\n", argv_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(ai);

    return socket_fd;
}

/**
 * @brief Reads the header, parses its content and checks if header is valid. Prints error messages and
 * returns appropriate exit code
 *
 * @param sockfile The file which is used to read the content
 * @return int Returns the appropriate exit code of the program. If everything is fine, then exit code 0 is returned
 */
static int validate_response_header(FILE *sockfile)
{
    char *line = NULL;
    size_t size = 0;
    char *version;
    char *code;
    char *message;

    if (getline(&line, &size, sockfile) == -1)
    {
        fprintf(stderr, "%s Error, while using getline(): %s\n", argv_name, strerror(errno));
        free(line);
        exit(EXIT_FAILURE);
    }

    char *rest = NULL;
    version = strtok_r(line, " ", &rest);
    code = strtok_r(NULL, " ", &rest);
    message = strtok_r(NULL, "\r", &rest);

    char *end_char;
    if (strncmp(version, "HTTP/1.1", strlen("HTTP/1.1")) != 0 || code == NULL || strtol(code, &end_char, 10) == 0)
    {
        fprintf(stderr, "%s Protocol error!\n", argv_name);
        free(line);
        return 2;
    }
    else if (strncmp(code, "200", strlen("200")) != 0)
    {
        fprintf(stderr, "%s %s: %s\n", argv_name, code, message);
        free(line);
        return 3;
    }

    while (getline(&line, &size, sockfile) != -1)
    {
        if (strncmp(line, "\r\n", strlen("\r\n")) == 0)
            break;
    }

    free(line);
    return 0;
}

/**
 * @brief Reads the body content of the reponse and writes this content to the output file
 *
 * @param output The output file where the content gets written to. Can be stdout or a file
 * @param sockfile The file which is used to read the content
 */
static void read_body_content(FILE *output, FILE *sockfile)
{
    uint8_t buf[1024];
    size_t read;

    while ((read = fread(buf, sizeof(uint8_t), 1024, sockfile)) != 0)
    {
        fwrite(buf, sizeof(uint8_t), read, output);
    }
    fflush(output);
}

/**
 * @brief Reads the input and checks if the program is called correctly and handles the whole process of this exercise
 *
 * @param argc number of arguments of the program call
 * @param argv array of the arguments of the program call
 * @return int return value of main method to tell when program is finished (and therefore can be removed from memory)
 */
int main(int argc, char *argv[])
{
    argv_name = argv[0];
    bool port_set = false, file_set = false, dir_set = false;

    // default values
    char *port = "80";
    char *filename;
    char *dirname;
    char *url;
    FILE *output = stdout;
    int exit_code;

    if (argc <= 1)
    {
        fprintf(stdout, "SYNOPSIS:\n%s [-p PORT] [ -o FILE | -d DIR ] URL\n", argv_name);
        exit(EXIT_FAILURE);
    }

    int c;
    while ((c = getopt(argc, argv, "p:o:d:")) != -1)
    {
        switch (c)
        {
        case 'p':
            if (port_set)
                usage();

            // check if valid port (only consists of digits)
            char *port_inputOriginal = strdup(optarg);
            char *port_input = port_inputOriginal;
            while (*port_input != '\0')
            {
                if (isdigit(*port_input) == 0)
                    usage();
                port_input++;
            }
            free(port_inputOriginal);

            port = optarg;
            port_set = true;
            break;
        case 'o':
            if (file_set || dir_set)
                usage();
            filename = optarg;
            file_set = true;
            break;
        case 'd':
            if (file_set || dir_set)
                usage();
            dirname = optarg;
            dir_set = true;
            break;

        case '?':
            usage();
            break;
        default:
            assert(0);
        }
    }

    if (argc - optind != 1)
    {
        usage();
    }
    url = argv[optind];
    char host[strlen(url)];
    char request[strlen(url)];

    parse_url(url, host, request);

    // write to stdout if no filename or directory specified
    if (!file_set && !dir_set)
    {
        // write to stdout
    }
    else
    {
        char *filepath;

        // set filepath to filename if filename specified
        if (file_set)
            filepath = filename;

        // set filepath to dirname/filename if directory specified

        char *filenameOriginal;
        if (dir_set)
        {
            filenameOriginal = strdup(request + 1);
            filename = filenameOriginal;

            if (filename[0] == '\0' || filename[0] == '?')
            {
                filename = "index.html";
            }
            else
            {
                filename = strtok(filename, "?");
            }

            filepath = strdup(dirname);
            strcat(filepath, "/");
            strcat(filepath, filename);
        }
        output = fopen(filepath, "w");

        // free filepath if directory is set (because of strdup)
        if (dir_set)
        {
            free(filenameOriginal);
            free(filepath);
        }

        if (output == NULL)
        {
            fprintf(stderr, "%s Error while opening file: %s\n", argv_name, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // set up connection
    int socket_fd = socket_setup(host, port);
    if (socket_fd < 0)
    {
        fprintf(stderr, "%s Error, while setting up socket: %s\n", argv_name, strerror(errno));
        close(socket_fd);
        if (output != stdout)
            fclose(output);
        exit(EXIT_FAILURE);
    }

    FILE *sockfile = fdopen(socket_fd, "r+");
    if (sockfile == NULL)
    {
        fprintf(stderr, "%s Error while opening socket file with fd: %s\n", argv_name, strerror(errno));
        close(socket_fd);
        if (output != stdout)
            fclose(output);
        exit(EXIT_FAILURE);
    }

    // request
    fprintf(sockfile, "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: bsue-http-client/1.0\r\nConnection: close\r\n\r\n", request, host);
    fflush(sockfile);

    // validate header and terminate with according exit code
    if ((exit_code = validate_response_header(sockfile)) != 0)
    {
        fclose(sockfile);
        if (output != stdout)
            fclose(output);
        exit(exit_code);
    }

    // read content
    read_body_content(output, sockfile);

    // close everything
    fclose(sockfile);
    if (output != stdout)
        fclose(output);
    exit(EXIT_SUCCESS);

    return 0;
}