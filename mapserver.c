#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "common.h"

#define LOG_FILE "mapserver.log"
#define MAP_FILE "/dev/asciimap"
#define MAP_SIZE 80*24

void log_message(char *message) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    }
}

void send_error(int client_socket, char *error_message) {
    int message_size = sizeof(char) + sizeof(int) + strlen(error_message);
    char *message = malloc(message_size);
    message[0] = RESPONSE_ERROR;
    *((int *)(message+1)) = strlen(error_message);
    memcpy(message+sizeof(char)+sizeof(int), error_message, strlen(error_message));
    send(client_socket, message, message_size, 0);
    free(message);
}

void handle_client(int client_socket) {
    char request[REQUEST_SIZE];
    if (recv(client_socket, request, REQUEST_SIZE, 0) < 0) {
        perror("recv");
        log_message("Error: Failed to receive request from client.");
        return;
    }

    if (request[0] != REQUEST_MAP) {
        log_message("Error: Received invalid request type.");
        send_error(client_socket, "Invalid request type.");
        return;
    }

    int width, height;
    if (request[1] == 0) {
        width = height = -1;
    } else if (recv(client_socket, &width, sizeof(int), 0) < 0 || recv(client_socket, &height, sizeof(int), 0) < 0) {
        perror("recv");
        log_message("Error: Failed to receive request size from client.");
        return;
    }

    // Read map from file
    char map[MAP_SIZE];
    FILE *map_file = fopen(MAP_FILE, "r");
    if (!map_file) {
        char error_message[100];
        snprintf(error_message, sizeof(error_message), "Failed to open map file %s: %s", MAP_FILE, strerror(errno));
        send_error(client_socket, error_message);
        log_message(error_message);
        return;
    }

    if (fread(map, 1, MAP_SIZE, map_file) < MAP_SIZE) {
        char error_message[100];
        snprintf(error_message, sizeof(error_message), "Failed to read entire map from file %s: %s", MAP_FILE, strerror(errno));
        send_error(client_socket, error_message);
        log_message(error_message);
        fclose(map_file);
        return;
    }
    fclose(map_file);

    // Send map to client
    int response_size = sizeof(char) + 2*sizeof(int) + MAP_SIZE;
    char *response = malloc(response_size);
    response[0] = RESPONSE_MAP;
    *((int *)(response+sizeof(char))) = width;
    *((int *)(response+sizeof(char)+sizeof(int))) = height;
    memcpy(response+sizeof(char)+2*sizeof(int), map, MAP_SIZE);
    if (send(client_socket, response, response_size, 0) < 0) {
    perror("send");
    log_message("Error: Failed to send response to client.");
    free(response);
    return;
}
free(response);
}

int main(int argc, char **argv) {
    int server_socket, client_socket, opt = 1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    // Create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    // Set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        return 1;
    }

    // Bind socket to port and address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(DEFAULT_PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_socket, SOMAXCONN) < 0) {
        perror("listen");
        return 1;
    }

    // Accept incoming connections and handle them in separate processes
    while (1) {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) < 0) {
            perror("accept");
            continue;
        }

        char *client_ip = inet_ntoa(client_addr.sin_addr);
        char message[100];
        snprintf(message, sizeof(message), "Accepted connection from %s:%d", client_ip, ntohs(client_addr.sin_port));
        log_message(message);

        pid_t pid;
        if ((pid = fork()) < 0) {
            perror("fork");
            log_message("Error: Failed to fork new process to handle client connection.");
            close(client_socket);
            continue;
        } else if (pid == 0) {
            // Child process
            close(server_socket);
            handle_client(client_socket);
            close(client_socket);
            exit(0);
        } else {
            // Parent process
            close(client_socket);
        }
    }

    return 0;
}