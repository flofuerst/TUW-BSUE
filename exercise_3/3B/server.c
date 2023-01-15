/**
 * @file server.c
 * @author Florian Fürst (12122096)
 * @brief Server program which uses HTTP 1.1. The server waits for connections from clients and transmits
 * the requested files. It is possible to specify a port on which the server shall listen for incoming connections.
 * It is also possible to specify the index filename of the file on which the server shall attempt to transmit.
 * @version 0.1
 * @date 2023-01-15
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
#include <signal.h>
#include <sys/stat.h>
#include <time.h>

/**
 * @brief the program name which is used for usage and error messages
 *
 */
static char *argv_name;

/**
 * @brief A boolean which is needed for signal handling
 *
 */
volatile bool state = true;

/**
 * @brief Function to handle signals;
 *        In this case 'state' will be set to false if a signal gets detected
 *
 * @param signal signal number to which the handling function is set
 */
static void handle_signal(int signal) { state = false; }

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
 * @brief Setup of the server. Returns the given file decriptor which is needed for further implementation.
 *
 * @param port The port which is used by getaddrinfo
 * @return int Returns the file descriptor of the socket
 */
static int socket_setup(char *port)
{
    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int res = getaddrinfo(NULL, port, &hints, &ai);
    if (res != 0)
    {
        fprintf(stderr, "%s Error, while using getaddrinfo(): %s\n", argv_name, gai_strerror(res));
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sockfd < 0)
    {
        fprintf(stderr, "%s Error, while using socket(): %s\n", argv_name, strerror(errno));
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0)
    {
        fprintf(stderr, "%s Error, while using bind(): %s\n", argv_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 5) < 0)
    {
        fprintf(stderr, "%s Error, while using listen(): %s\n", argv_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(ai);
    return sockfd;
}

/**
 * @brief Parses the header of the client and checks if the header is valid
 *
 * @param line The current line of the header which gets read
 * @param size The size of the input buffer 'line'
 * @param sockfile The file which is used to read the content
 * @param path The path to the file which should get read from
 * @param filename The file which was eventually specified through input. Default value is "index.html"
 * @param doc_root_dir The directory which was specified through input
 * @return int Returns the appropriate status code
 */
static int parse_and_validate_header(char *line, size_t size, FILE *sockfile, char *path, char *filename, char *doc_root_dir)
{
    char *temp_lineOriginal = strdup(line);
    char *temp_line = temp_lineOriginal;

    char *rest = NULL;
    char *request_method = strtok_r(temp_line, " ", &rest); // GET

    char *filepath_temp = strtok_r(NULL, " ", &rest); // /index.html or ends with '/' (dir specified)
    if (filepath_temp == NULL)
        filepath_temp = "";

    char *version = strtok_r(NULL, " ", &rest); // HTTP/1.1
    if (version == NULL)
        version = "";

    char *last = strtok_r(NULL, "\r\n", &rest); // \r\n

    char *filepathOriginal = strdup(filepath_temp);

    char *filepath = filepathOriginal;

    // add 'index.html' if filepath ends with '/' (dir specified)
    if (filepath[strlen(filepath) - 1] == '/')
    {
        strcat(filepath, filename);
    }

    // build up path
    strcat(path, doc_root_dir);
    strcat(path, filepath);

    // read rest and ignore content
    while (getline(&line, &size, sockfile) != -1)
    {
        if (strncmp(line, "\r\n", strlen("\r\n")) == 0)
            break;
    }

    // check if header is invalid
    if (request_method == NULL || filepath == NULL || version == NULL || last != NULL || strcmp(version, "HTTP/1.1\r\n"))
    {
        free(temp_lineOriginal);
        if (filepath_temp != NULL)
            free(filepathOriginal);

        return 400;
    }
    // check if different request method
    if (strcmp(request_method, "GET") != 0)
    {
        free(temp_lineOriginal);
        free(filepathOriginal);
        return 501;
    }

    free(temp_lineOriginal);
    free(filepathOriginal);
    return 200;
}

/**
 * @brief Builds up and writes the header, checks the appropriate extension of the file
 *  and writes the content to the wanted file
 *
 * @param sockfile The file which is used to read the content
 * @param file_transmit The file which gets written to
 * @param path The path to the wanted file
 * @param status_code The status code which is included in the header
 */
static void write_to_transmit(FILE *sockfile, FILE *file_transmit, char *path, int status_code)
{
    // get size of file with stat
    struct stat attribute;
    int length, state;
    if ((state = stat(path, &attribute)) == 0)
    {
        length = attribute.st_size;
    }
    else
    {
        length = -1;
    }

    // get date and time
    char date_and_time[1024];
    time_t now = time(NULL);
    struct tm *time_gmt = gmtime(&now);
    strftime(date_and_time, 1024, "%a, %d %b %Y %H:%M:%S GMT", time_gmt);

    char *message = "OK";
    // default value of content type; If no extension is matching, then the Content-Type stays empty and will not
    // be included in the header
    char *type = "";

    // get last occurrence of '.' --> therefore strrchar (and not strchar)
    char *file_extension = strrchr(path, '.');

    // only check extension if not null
    if (file_extension != NULL)
    {
        file_extension++;

        if (strcmp(file_extension, "html") == 0 || strcmp(file_extension, "htm") == 0)
        {
            type = "Content-Type: text/html\r\n";
        }
        else if (strcmp(file_extension, "css") == 0)
        {
            type = "Content-Type: text/css\r\n";
        }
        else if (strcmp(file_extension, "js") == 0)
        {
            type = "Content-Type: application/javascript\r\n";
        }
    }

    if (fprintf(sockfile, "HTTP/1.1 %d %s\r\nDate: %s\r\n%sContent-Length: %d\r\nConnection: close\r\n\r\n",
                status_code, message, date_and_time, type, length) < 0)
        fprintf(stderr, "%s Error, while using fprintf(): %s\n", argv_name, strerror(errno));

    fflush(file_transmit);

    uint8_t buf[1024];
    size_t read = 0;

    while ((read = fread(buf, sizeof(uint8_t), 1024, file_transmit)) != 0)
    {
        fwrite(buf, sizeof(uint8_t), read, sockfile);
    }
    fflush(sockfile);
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
    bool port_set = false, file_set = false;

    // default values
    char *port = "8080";
    char *filename = "index.html";
    char *doc_root_dir;
    int status_code;

    char *line = NULL;
    size_t size = 0;

    if (argc <= 1)
    {
        fprintf(stdout, "SYNOPSIS:\n%s [-p PORT] [ -o FILE | -d DIR ] URL\n", argv_name);
        exit(EXIT_FAILURE);
    }

    int c;
    while ((c = getopt(argc, argv, "p:i:")) != -1)
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

        case 'i':
            if (file_set)
                usage();
            filename = optarg;
            file_set = true;
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
    doc_root_dir = argv[optind];

    // set signal handler
    struct sigaction sa = {.sa_handler = handle_signal};
    if (sigaction(SIGINT, &sa, NULL) + sigaction(SIGTERM, &sa, NULL) < 0)
    {
        fprintf(stderr, "%s Error while initializing signal handler: %s\n", argv_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // socket setup
    int sockfd = socket_setup(port);

    //loop till SIGINT or SIGTERM happens
    while (state)
    {
        int connfd = accept(sockfd, NULL, NULL);

        if (connfd < 0)
        {
            if (errno != EINTR)
            {
                fprintf(stderr, "%s Error, while using accept(): %s\n", argv_name, strerror(errno));
                continue;
            }
        }

        // break if state is false (down)
        if (!state)
        {
            break;
        }
        //open sockfile and check if NULL
        FILE *sockfile = fdopen(connfd, "r+");
        if (sockfile == NULL)
        {
            fprintf(stderr, "%s Error while opening file with fd: %s\n", argv_name, strerror(errno));
            close(connfd);
            continue;
        }

        //read sockfile
        if (getline(&line, &size, sockfile) == -1)
        {
            fprintf(stderr, "%s Error, while using getline(): %s\n", argv_name, strerror(errno));
            fclose(sockfile);
            continue;
        }

        //make path array "long" enough
        char path[strlen(line) + strlen(doc_root_dir) + strlen(filename)];
        strcpy(path, "");

        status_code = parse_and_validate_header(line, size, sockfile, path, filename, doc_root_dir);

        FILE *file_transmit = fopen(path, "r");

        // set status code to 404 if server cannot open the resulting filepath
        if (status_code == 200)
        {
            if (file_transmit == NULL)
            {
                status_code = 404;
            }
        }

        //check if still status code 200 (or if status code got changed above to 404)
        if (status_code == 200)
        {
            write_to_transmit(sockfile, file_transmit, path, status_code);
        }
        else
        {
            //set message based on status code
            char *message = "";
            if (status_code == 400)
                message = "Bad Request";
            else if (status_code == 404)
                message = "Not Found";
            else if (status_code == 501)
                message = "Not Implemented";

            if (fprintf(sockfile, "HTTP/1.1 %d %s\r\nConnection: close\r\n\r\n", status_code, message) < 0)
                fprintf(stderr, "%s Error, while using fprintf(): %s\n", argv_name, strerror(errno));
            fflush(sockfile);
        }

        //cleanup
        if (file_transmit != NULL)
        {
            fclose(file_transmit);
        }
        fclose(sockfile);
    }
    free(line);

    return 0;
}