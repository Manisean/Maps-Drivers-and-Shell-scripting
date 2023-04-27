#include "common.h"

struct Message {
  char code;
  int data[2];
};

void handle_request(int client_sock) {
    // Read the request from the client
    struct Message request;
    int bytes_received = recv(client_sock, (void *)&request, sizeof(request), 0);
    if (bytes_received < 0) {
        perror("failed to receive request");
        exit(EXIT_FAILURE);
    }

    // Validate the request
    if (request.code != 'M' || (request.data[0] != 0 && request.data[0] != 1)) {
        // Send an error response
        int error_len = strlen("invalid request");
        int response_size = sizeof(char) + sizeof(int) + error_len;
        char *response = (char *)malloc(response_size);
        response[0] = 'E';
        memcpy(response + 1, &error_len, sizeof(int));
        memcpy(response + sizeof(char) + sizeof(int), "invalid request", error_len);
        send(client_sock, response, response_size, 0);
        free(response);
        return;
    }

    // Handle the request
    if (request.data[0] == 0) {
        // Default map size
        int map_size = 10;
        char *map = (char *)malloc(map_size);
        // Fill the map with some data
        // ...
        // Send the map to the client
        int response_size = sizeof(char) + sizeof(int)*2 + map_size;
        char *response = (char *)malloc(response_size);
        response[0] = 'M';
        memcpy(response + 1, &map_size, sizeof(int)*2);
        memcpy(response + 1 + sizeof(int)*2, map, map_size);
        send(client_sock, response, response_size, 0);
        free(map);
        free(response);
    } else {
        // Custom map size
        int width = request.data[1];
        int height = request.data[2];
        if (width <= 0 || height <= 0) {
        // Send an error response
        int error_len = strlen("invalid map size");
        int response_size = sizeof(char) + sizeof(int) + error_len;
        char *response = (char *)malloc(response_size);
        response[0] = 'E';
        memcpy(response + 1, &error_len, sizeof(int));
        memcpy(response + sizeof(char) + sizeof(int), "invalid map size", error_len);
        send(client_sock, response, response_size, 0);
        free(response);
        return;
        }
        int map_size = width * height;
        char *map = (char *)malloc(map_size);
        // Fill the map with some data
        // ...
        // Send the map to the client
        int response_size = sizeof(char) + sizeof(int)*2 + map_size;
        char *response = (char *)malloc(response_size);
        response[0] = 'M';
        memcpy(response + 1, &width, sizeof(int));
        memcpy(response + 1 + sizeof(int), &height, sizeof(int));
        memcpy(response + 1 + sizeof(int)*2, map, map_size);
        send(client_sock, response, response_size, 0);
        free(map);
        free(response);
    }
}

int main(int argc, char **argv) {
    // Parse command line arguments
    int port = DEFAULT_PORT;
    char *ip = DEFAULT_IP;
    if (argc >= 2) {
        ip = argv[1];
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
    }

    // Create a socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Bind to the address
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("failed to bind to address");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_sock, 1) < 0) {
        perror("failed to listen for connections");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connections and handle them one by one
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
        perror("failed to accept incoming connection");
        continue;
        }

        // Spawn a child process to handle the client request
        pid_t pid = fork();
        if (pid < 0) {
        perror("failed to spawn child process");
        close(client_sock);
        continue;
        } else if (pid == 0) {
        // Child process

        // Read the client's request
        char request_type;
        recv(client_sock, &request_type, sizeof(char), 0);
        if (request_type != 'M') {
            // Unrecognized request type
            int error_length = strlen("Unrecognized request type");
            char *error = (char *)malloc(error_length + 1);
            strcpy(error, "Unrecognized request type");
            send_error(client_sock, error, error_length);
            free(error);
            close(client_sock);
            exit(EXIT_FAILURE);
        }

        int request_size;
        recv(client_sock, &request_size, sizeof(int), 0);
        if (request_size != sizeof(int) && request_size != sizeof(int) * 2) {
            // Invalid request size
            int error_length = strlen("Invalid request size");
            char *error = (char *)malloc(error_length + 1);
            strcpy(error, "Invalid request size");
            send_error(client_sock, error, error_length);
            free(error);
            close(client_sock);
            exit(EXIT_FAILURE);
        }

        int map_width = 0;
        int map_height = 0;
        if (request_size == sizeof(int) * 2) {
            // Read the requested map dimensions
            recv(client_sock, &map_width, sizeof(int), 0);
            recv(client_sock, &map_height, sizeof(int), 0);
        }

        // Open the map file
        int map_fd = open("/dev/asciimap", O_RDONLY);
        if (map_fd < 0) {
            perror("Failed to open /dev/asciimap");
            exit(EXIT_FAILURE);
        }
        char map_buffer[MAP_SIZE];
        int map_bytes_read = read(map_fd, map_buffer, MAP_SIZE);
        if (map_bytes_read < 0) {
            perror("Failed to read from /dev/asciimap");
            exit(EXIT_FAILURE);
        }

    // Close the connection
    close(client_sock);

    return 0;
}