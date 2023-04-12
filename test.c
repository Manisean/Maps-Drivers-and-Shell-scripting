#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define DEFAULT_WIDTH 80 // default width of map
#define DEFAULT_HEIGHT 24 // default height of map
#define DEFAULT_LINE 1 // default line number of file

int count_lines(FILE *file) {
    // function to count number of lines in a file
    int count = 0;
    char ch = fgetc(file);
    while (ch != EOF) {
        if (ch == '\n') {
            count++;
        }
        ch = fgetc(file);
    }
    return count;
}

int main(int argc, char *argv[]) {
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;
    int line = DEFAULT_LINE;

    // parse command line arguments to set width, height and line number
    // ...

    // iterate through filename arguments
    for (int i = 1; i < argc; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // child process

            // open file
            FILE *file = fopen(argv[i], "r");
            if (file == NULL) {
                perror("File open failed");
                exit(EXIT_FAILURE);
            }

            // count number of lines in file
            int num_lines = count_lines(file);

            // calculate starting line number and number of lines to include in map
            int start_line = (line > num_lines) ? num_lines : line;
            int end_line = (line + height - 1 > num_lines) ? num_lines : line + height - 1;
            int num_map_lines = end_line - start_line + 1;

            // allocate memory for map
            char **map = (char **)malloc(num_map_lines * sizeof(char *));
            for (int j = 0; j < num_map_lines; j++) {
                map[j] = (char *)malloc((width + 1) * sizeof(char));
            }

            // read file and store lines in map
            fseek(file, 0, SEEK_SET);
            for (int j = 0; j < start_line - 1; j++) {
                char buffer[256];
                fgets(buffer, 256, file);
            }
            for (int j = 0; j < num_map_lines; j++) {
                char buffer[256];
                if (fgets(buffer, 256, file) == NULL) {
                    for (int k = 0; k < width; k++) {
                        map[j][k] = ' ';
                    }
                } else {
                    int len = strlen(buffer);
                    if (len < width) {
                        buffer[len-1] = '\0'; // remove newline character
                        for (int k = len-1; k < width; k++) {
                            buffer[k] = ' ';
                        }
                    } else {
                        buffer[width-1] = '\0'; // truncate line to fit width
                    }
                    strcpy(map[j], buffer);
                }
            }

            // close file
            fclose(file);

            // print map
            for (int j = 0; j < num_map_lines; j++) {
                printf("%s\n", map[j]);
            }

            // free memory
            for (int j = 0; j < num_map_lines; j++) {
                free(map[j]);
            }
            free(map);
            exit(EXIT_SUCCESS);
        } else {
            // parent process
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                int exit_status = WEXITSTATUS(status);
                if (exit_status != 0) {
                    printf("Map generation failed for %s\n", argv[i]);
                }
            }
        }
    }
    return 0;
}