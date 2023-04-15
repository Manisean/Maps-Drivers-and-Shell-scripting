#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"

#define MAX_LINE_LEN 1024

int countln(int fd) {
    int count = 0;
    char buf[MAX_LINE_LEN];
    while (read(fd, buf, MAX_LINE_LEN) > 0) {
        for (int i = 0; i < MAX_LINE_LEN; i++) {
            if (buf[i] == '\n') {
                count++;
            }
        }
    }
    return count;
}

void generate_map(char* filename, int line_num, int width, int height) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open() failed");
        exit(1);
    }

    int line_count = countln(fd);
    if (line_count < line_num) {
        line_num = line_count;
    }

    lseek(fd, 0, SEEK_SET);
    for (int i = 1; i < line_num; i++) {
        char buf[MAX_LINE_LEN];
        int n = read(fd, buf, MAX_LINE_LEN);
        if (n < 0) {
            perror("read() failed");
            exit(1);
        }
        for (int j = 0; j < n; j++) {
            if (buf[j] == '\n') {
                line_num--;
                if (line_num == 0) {
                    break;
                }
            }
        }
        if (line_num == 0) {
            break;
        }
    }

    char map[height][width+1];
    for (int i = 0; i < height; i++) {
        char buf[MAX_LINE_LEN];
        int n = read(fd, buf, MAX_LINE_LEN);
        if (n < 0) {
            perror("read() failed");
            exit(1);
        }
        if (n == 0) {
            memset(map[i], ' ', width);
        } else {
            int j = 0;
            for (; j < n && j < width; j++) {
                map[i][j] = buf[j];
            }
            for (; j < width; j++) {
                map[i][j] = ' ';
            }
        }
        map[i][width] = '\0';
    }
    close(fd);

    for (int i = 0; i < height; i++) {
        printf("%s\n", map[i]);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s FILENAME [LINE_NUM] [WIDTH] [HEIGHT]\n", argv[0]);
        exit(1);
    }

    char* filename = argv[1];
    int line_num = 1;
    int width = 80;
    int height = 24;

    if (argc > 2) {
        line_num = atoi(argv[2]);
    }
    if (argc > 3) {
        width = atoi(argv[3]);
    }
    if (argc > 4) {
        height = atoi(argv[4]);
    }

    generate_map(filename, line_num, width, height);

    return 0;
}

