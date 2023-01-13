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

static char *argv_name;

static void usage(void)
{
    fprintf(stderr, "SYNOPSIS:\n%s [-p PORT] [ -o FILE | -d DIR ] URL\n", argv_name);
    exit(EXIT_FAILURE);
}

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
        // printf("host: %s\n", host);
        // printf("request: %s\n", request);
    }
    // If character(s) above exist: copy all characters after first occurrence to request string
    else
    {
        strcpy(request, request_temp);

        // printf("%s\n", host_total);
        // printf("request: %s\n", request);
        // printf("request[0]: %c\n", request[0]);

        char *e = strchr(host_total, request[0]);
        // printf("%s\n", e);
        int index = (int)(e - host_total);
        // printf("%d\n", index);
        // printf("%c\n", host_total[index]);

        strncpy(host, host_total, index);
        // printf("%s\n", host);
        host[index] = '\0';
        // printf("host: %s\n", host);
    }

    // check if invalid hostname
    if (host[0] == '\0')
    {
        fprintf(stderr, "%s Error, invalid hostname\n", argv_name);
        exit(EXIT_FAILURE);
    }
}

static int socket_setup(char *host, char *request, char *port)
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
    message = strtok_r(NULL, " ", &rest);
    fprintf(stderr, "http-version=%s status=%s text-status=%s\n", version, code, message);

    if (strncmp(version, "HTTP/1.1", strlen("HTTP/1.1")) != 0)
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

static void read_body_content(FILE* output, FILE* sockfile){
    uint8_t buf[1024];
    size_t read;

    while((read = fread(buf, sizeof(uint8_t), 1024, sockfile))!= 0){
        fwrite(buf, sizeof(uint8_t), read, output);
    }
    fflush(output);
}

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
        usage();
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
                if (!isdigit(*port_input))
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
    // printf("host: %s | request: %s\n", host, request);

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
        if (dir_set)
        {
            filename = request + 1;
            if (filename[0] == '\0')
            {
                filename = "index.html";
            }
            filepath = strdup(filename);
            strcpy(filepath, dirname);
            strcat(filepath, "/");
            strcat(filepath, filename);
            free(filepath);
        }

        output = fopen(filepath, "w");

        if (output == NULL)
        {
            fprintf(stderr, "%s Error while opening file: %s\n", argv_name, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // set up connection
    int socket_fd = socket_setup(host, request, port);
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