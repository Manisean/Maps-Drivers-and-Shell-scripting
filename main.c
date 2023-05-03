#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include "common.h"

#define MAX_LINE_LEN 1024
#define MY_IOCTL_MAGIC 'm'
#define RESET_MAP       _IO(MY_IOCTL_MAGIC, 0)
#define ZERO_OUT_BUFFER _IO(MY_IOCTL_MAGIC, 1)
#define CHECK_MAP       _IO(MY_IOCTL_MAGIC, 2)

void test_ioctl(int fd){
	int ret, op, np;
	char buf[MAX_LINE_LEN];
	char nonbuf[MAX_LINE_LEN];

	ret = ioctl(fd, RESET_MAP, 0);
	if (ret != 0) {
		(void) fprintf(stderr, "RESET_MAP CAUSED ERROR, RETURNED: %d\n", ret);
	}
	op = read(fd, buf, sizeof(buf));
	
	printf("\nRESET PROOF: \n");
	for (int i = 0; i < op; i++)
		printf("%c", buf[i]);
	printf("\n");

	ret = ioctl(fd, CHECK_MAP, 0);
	if (ret != 0) {
		(void) fprintf(stderr, "CHECK MAP SUCC CAUSED ERROR, RETURNED: %d\n", ret);	
	}
	
	np = open("nonascii.txt", O_RDONLY);
	op = read(np, nonbuf, sizeof(nonbuf));
	op = write(fd, nonbuf, sizeof(nonbuf));

	ret = ioctl(fd, CHECK_MAP, 0);
	if (ret != 0) {
		(void) fprintf(stderr, "CHECK MAP FAil CAUSED ERROR, RETURNED: %d\n", ret);	
	}
	
	ret = ioctl(fd, ZERO_OUT_BUFFER, 0);
	if (ret != 0) {
		(void) fprintf(stderr, "ZERO_OUT_BUFFER CAUSED ERROR, RETURNED: %d\n", ret);
	}
	op = read(fd, buf, sizeof(buf));
	
	printf("\nZERO_OUT PROOF: \n");
	for (int i = 0; i < op; i++)
		printf("%c", buf[i]);
	printf("\n");

	ioctl(fd, RESET_MAP, 0);

	close(np);
}

void test_seek(int fd) {
	int fp;
	char buf[MAX_LINE_LEN];

	fp = read(fd, buf, 200);
	if (fp < 0)
    	perror("read failed");
	printf("\nBEFORE LSEEK BUF:\n");
	for (int i = 0; i < fp; i++)
		printf("%c", buf[i]);
	printf("\n");

	lseek(fd, 0, SEEK_SET);
	fp = read(fd, buf, 200);
	printf("\nAfter SEEK_SET (BEGIN):\n");
	for (int i = 0; i < fp; i++)
		printf("%c", buf[i]);
	printf("\n");

	lseek(fd, 200, SEEK_SET);
	fp = read(fd, buf, 200);
	printf("\nAfter SEEK_SET (OFF 200):\n");
	for (int i = 0; i < fp; i++)
		printf("%c", buf[i]);
	printf("\n");

	lseek(fd, 200, SEEK_CUR);
	fp = read(fd, buf, 200);
	printf("\nAfter SEEK_CUR (OFF 200):\n");
	for (int i = 0; i < fp; i++)
		printf("%c", buf[i]);
	printf("\n");

	lseek(fd, 0, SEEK_END);
	fp = read(fd, buf, 200);
	printf("\nAfter SEEK_END:\n");
	for (int i = 0; i < fp; i++)
		printf("%c", buf[i]);
	printf("\n");
}


void generate_map(char * filename, int line_num, int width, int height) {
    int fd, n;
    char first_buf[MAX_LINE_LEN];

    // read data from buffer of /dev/asciimap
    if ((fd = open("/dev/asciimap", O_RDWR)) >= 0) {
        n = read(fd, first_buf, sizeof(first_buf));
        if (n == 0) {
            (void) fprintf(stderr, "READ 1 FAILED: got %d\n", n);
            if (n < 0)
                perror("read(asciimap) failed");
        } else {
            (void) printf("Original map received:  \n");
            for (int i = 0; i < n; i++)
			{
				printf("%c", first_buf[i]);
			}
            (void) printf("\n");
        }
		close(fd);

        // read test file contents into second buffer
		int tst;
        if ((tst = open(filename, O_RDWR)) >= 0) {
         
			char map[height][width+1];
			int i = 0;
			while (i < height) {
			    char buf[MAX_LINE_LEN];
			    int m = read(tst, buf, MAX_LINE_LEN);
			    if (m < 0) {
		        perror("read() failed");
		        exit(1);
		    }
		    if (m == 0) {
		        memset(map[i], ' ', width);
		    } else {
		        int j = 0;
		        int k = 0;
		        while (j < m && k < width) {
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
		        while (j < m && buf[j] != '\n') {
		            j++;
		        }
		        if (j < m) {
		            j++;
		        }
		        lseek(fd, j-m, SEEK_CUR);
		    }
		    map[i][width] = '\0';
		    i++;
		}
		close(tst);
		printf("\nFILE CONTENTS: \n");
		for (int i = 0; i < height; i++) {
		    printf("%s\n", map[i]);
		}

		char fin_buf[MAX_LINE_LEN];

		fd = open("/dev/asciimap", O_RDWR);
		int w = write(fd, map, sizeof(map));
		if (w == 0)
			(void) fprintf(stderr, "WRITE FAILED: got %d\n", w);
		
		int l = read(fd, fin_buf, sizeof(fin_buf));
		if (l == 0) {
            (void) fprintf(stderr, "READ FINAL FAILED: got %d\n", l);
            if (l < 0)
                perror("read(asciimap) failed");
        } else {
            (void) printf("NEW MAP received:  \n");
            for (int i = 0; i < l; i++)
			{
				printf("%c", fin_buf[i]);
			}
            (void) printf("\n");

			(void) printf("\nioctl TESTS: \n");
			test_ioctl(fd);
			test_seek(fd);
        }
		close(fd);

        } else {
            (void) fprintf(stderr, "OPEN TEST FILE FAILED: %s\n", strerror(errno));
        }

    } else {
        (void) fprintf(stderr, "OPEN DEVICE FAILED: %s\n", strerror(errno));
    }

}

int main(int argc, char * argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s FILENAME [LINE_NUM] [WIDTH] [HEIGHT]\n", argv[0]);
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        char * filename = argv[i];
        int line_num = 1;
        int width = 1;
        int height = 1;

        line_num = atoi(argv[i + 1]);
        width = atoi(argv[i + 2]);
        height = atoi(argv[i + 3]);

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
            waitpid(pid, & status, 0);
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                fprintf(stderr, "Failed to generate map for %s\n", filename);
            }
        }
    }

    return 0;
}
