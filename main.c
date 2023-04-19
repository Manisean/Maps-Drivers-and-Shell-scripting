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

    close(fd);

    fd = open(filename, O_RDWR);
    if (fd < 0) {
        perror("open() failed");
        exit(1);
    }

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
	int i = 0;
	while (i < height) {
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
	        int k = 0;
	        while (j < n && k < width) {
	            if (buf[j] == '\n') {
	                break;
	            }
	            map[i][k] = buf[j];
	            j++;
	            k++;
	        }
	        for (; k < width; k++) {
	            map[i][k] = ' ';
	        }
	        while (j < n && buf[j] != '\n') {
	            j++;
	        }
	        if (j < n) {
	            j++;
	        }
	        lseek(fd, j-n, SEEK_CUR);
	    }
	    map[i][width] = '\0';
	    i++;
	}
	close(fd);
	
	for (int i = 0; i < height; i++) {
	    printf("%s\n", map[i]);
	}
	
	char* data = "***This is the end of the map. Congrats!!***\n";
	// Append new data to existing map
    char buffer[MAX_LINE_LEN * height + strlen(data)];
    int offset = 0;
    for (int i = 0; i < height; i++) {
        memcpy(buffer + offset, map[i], width);
        offset += width;
        buffer[offset] = '\n';
        offset++;
    }
    strcpy(buffer + offset, data);

    // Write updated map to file
    fd = open(filename, O_RDWR | O_TRUNC);
    if (fd < 0) {
        perror("open() failed");
        exit(1);
    }

    int bytes_written = write(fd, buffer, strlen(buffer));
    if (bytes_written < 0) {
        perror("write() failed");
        exit(1);
    }

    close(fd);
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s FILENAME [LINE_NUM] [WIDTH] [HEIGHT]\n", argv[0]);
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        char* filename = argv[i];
        int line_num = 1;
        int width = 1;
        int height = 1;

    	line_num = atoi(argv[i+1]);
    	width = atoi(argv[i+2]);
    	height = atoi(argv[i+3]);

        pid_t pid = fork();
        if (pid == -1) {
            fprintf(stderr, "Failed to fork process for %s\n", filename);
            continue;
        } else if (pid == 0) {
			printf("\n");
            generate_map(filename, line_num, width, height);
			printf("\n");
            exit(0);
        } else {
            int status;
            waitpid(pid, &status, 0);
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                fprintf(stderr, "Failed to generate map for %s\n", filename);
            }
        }
    }

    return 0;
}
