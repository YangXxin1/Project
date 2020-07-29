#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pwd.h>


#define ERR_EXIT(m) \
do{\
    perror(m);\
    exit(EXIT_FAILURE);\
}while(0)

#define MAX_BUFFER_SIZE 1024
#define MAX_COMMAND_SIZE 1024
#define MAX_COMMAND 32
#define MAX_ARG 1024

#endif //_COMMON_H