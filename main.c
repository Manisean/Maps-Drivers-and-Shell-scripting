#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "common.h"

#define MAX_LINE_LEN 1024

int main(int argc, char * argv[]) {
    
     // read data from buffer of /dev/asciimap
    if ((fd = open("/dev/asciimap", O_RDWR)) >= 0) {
        n = read(fd, first_buf, sizeof(first_buf));
        if (n == 0) {
            (void) fprintf(stderr, "READ 1 FAILED: got %d\n", n);
            if (n < 0)
                perror("read(asciimap) failed");
        } else {
            (void) printf("Original map received:  \n");
            for (int i = 0; i < n; i++) {
                printf("%c", first_buf[i]);
            }
            (void) printf("\n");
        }
		close(fd);

    return 0;
}
