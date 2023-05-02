#include "common.h"


struct Message {
    char code;
    int data[BSIZE];
};

int main(int argc, char **argv) {

    //set up client log
    int logid = open(CLIENT_LOG, O_WRONLY);
    if (logid == -1)
    {
        char err_msg[256];
        //use snprintf)_ to print to the buffer instead of to stdout
        snprintf(err_msg, 256, "Error opening file '%s'", CLIENT_LOG);
        //use perror() to output the specific error
        perror(err_msg);
        return 1;
    }

    int opt;
    char *ip = DEFAULT_IP;
    int port = DEFAULT_PORT;

    while ((opt = getopt(argc, argv, "i:p:")) != -1) {
        switch (opt) {
        case 'i':
            ip = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        default:
            printf("Usage: %s [-i IP] [-p port]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

  // Create a socket
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Specify the server address and port
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip);

  // Connect to the server
  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("connection failed");
    exit(EXIT_FAILURE);
  }

  // Tests
    // Read and print the maps 10 times
    for (int i = 0; i < 10; i++) {
        // Send request to server
        char requestTest[2 + 2 * sizeof(int)];
        memset(requestTest, 0, sizeof(requestTest));
        requestTest[0] = 'M';
        requestTest[1] = 0;
        if (write(sock, requestTest, sizeof(requestTest)) < 0) {
            perror("write() failed");
            exit(1);
        }

        // Receive map from server
        char header[sizeof(char) + 2 * sizeof(int)];
        if (read(sock, header, sizeof(header)) < 0) {
            perror("read() failed");
            exit(1);
        }

        if (header[0] == 'M') {
            int width = *(int *) &header[1];
            int height = *(int *) &header[1 + sizeof(int)];
            int map_size = width * height;

            char map[map_size];
            if (read(sock, map, map_size) < 0) {
                perror("read() failed");
                exit(1);
            }

            // Print map to STDOUT
            printf("Map %d:\n", i + 1);
            print_map(map, width, height);
        } else if (header[0] == 'E') {
            int error_len = *(int *) &header[1];

            char error[error_len + 1];
            if (read(sock, error, error_len) < 0) {
                perror("read() failed");
                exit(1);
            }
            error[error_len] = '\0';

            fprintf(stderr, "Error: %s\n", error);
        } else {
            fprintf(stderr, "Unrecognized protocol message\n");
        }
    }


  // Send a request for a map
  struct Message request;
  request.code = 'M';
  request.data[0] = 0;  // Default map size

  // Uncomment the following lines if want to specify map size
  //request.data[0] = 1;
  //request.data[1] = <width>;
  //request.data[2] = <height>;

  int request_size = sizeof(request.code) + sizeof(request.data);
  int bytes_sent = send(sock, (void *)&request, request_size, 0);
  if (bytes_sent != request_size) {
    perror("failed to send request");
    exit(EXIT_FAILURE);
  }

  // Read the server's response
  char response_type;
  recv(client_sock, &response_type, sizeof(char), 0);
  if (response_type == 'M') {
    // Read the map size
    int map_size;
    recv(client_sock, &map_size, sizeof(int), 0);

    // Read the map data
    char *map = (char *)malloc(map_size);
    recv(client_sock, map, map_size, 0);

    // Output the map to STDOUT
    fwrite(map, 1, map_size, stdout);
    fflush(stdout);

    free(map);
  } else if (response_type == 'E') {
    // Read the error message length
    int error_length;
    recv(client_sock, &error_length, sizeof(int), 0);

    // Read the error message data
    char *error = (char *)malloc(error_length + 1);
    recv(client_sock, error, error_length, 0);
    error[error_length] = '\0';

    // Output the error to STDERR
    fprintf(stderr, "%s\n", error);
    fflush(stderr);

    free(error);
  } else {
    // Unrecognized message type
    fprintf(stderr, "Unrecognized protocol message from server\n");
    exit(EXIT_FAILURE);
  }

  // Close the socket
  close(sock);
  close(logid);

  return 0;
}
