#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "common.h"

#define LOG_FILE "mapclient.log"

void log_message(char *message) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    }
}

int main(int argc, char **argv) {

    if (argc != 3) {
        printf("Usage: %s width height\n", argv[0]);
        return 1;
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }
    log_message("successfully made client socket\n\n");
    
    // Connect to server
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(DEFAULT_IP);
    server_address.sin_port = htons(DEFAULT_PORT);

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("connect");
        return 1;
    }
    log_message("connected!\n\n");

    // Send request to server
    char request[REQUEST_SIZE];
    request[0] = REQUEST_MAP;
    *((int *)(request+1)) = width;
    *((int *)(request+1+sizeof(int))) = height;

    if (send(sock, request, REQUEST_SIZE, 0) < 0) {
        perror("send");
        return 1;
    }
    log_message("sent a message to the server\n\n");

    // Receive response from server
    char response[MAP_SIZE+2*sizeof(int)];
    if (recv(sock, response, MAP_SIZE+2*sizeof(int), 0) < 0) {
        perror("recv");
        return 1;
    }

    if (response[0] != RESPONSE_MAP) {
        printf("Received invalid response type from server.\n");
        return 1;
    }
    log_message("received a response from the server\n\n");

    int response_width = *((int *)(response+1));
    int response_height = *((int *)(response+1+sizeof(int)));

    printf("Received map with dimensions %d x %d\n", response_width, response_height);
    printf("%s\n", response+1+2*sizeof(int));
    log_message(response+1+2*sizeof(int));

    // Close socket
    close(sock);

    return 0;
}
