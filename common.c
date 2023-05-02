/****
 * common.c
 *
 * Contains generic common global definitions.
 * Serguei Mokhov
 *
 */

#include "common.h"
#include <stdarg.h>


/* Debugging/error/logging functions */

void print(char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}

void log(int fd, char* format, ...) {
    va_list args;
    va_start(args, format);
    time_t t = time(NULL);
    char b[25];
    strcpy(b, ctime(&t));
    b[24]=0;
    fprintf(fd, "[%s]: ", b);
    vfprintf(fd, format, args);
    fflush(fd);
    va_end(args);
}

void slog(FILE* stream, char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
}

void elog(int fd, char* format, ...) {
    va_list args;
    va_start(args, format);
    log(fd, format, args);
    perror("");
    va_end(args);
}

void perrexit(char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    perror("");
    exit(1);
    va_end(args);
}
