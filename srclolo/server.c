// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>

#define PORT_1 20251
#define PORT_2 20252
#define BUF_SIZE 4096
#define T 20
#define MAX_CONNECTION_TIME (T + 3)

void *connection_handler(void *arg) {
    int sock = *(int *)arg;
    free(arg);

    char buffer[BUF_SIZE];
    memset(buffer, 'A', BUF_SIZE);

    time_t start_time = time(NULL);

    while (1) {
        ssize_t sent = send(sock, buffer, BUF_SIZE, 0);
        if (sent <= 0) break;

        if (time(NULL) - start_time >= MAX_CONNECTION_TIME)
            break;
    }

    close(sock);
    pthread_exit(NULL);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    signal(SIGPIPE, SIG_IGN); // Evita que el servidor termine con un write a socket cerrado

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT_1);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 128) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT_1);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            continue;
        }

        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, connection_handler, client_sock) != 0) {
            perror("pthread_create");
            close(new_socket);
            free(client_sock);
        }

        pthread_detach(thread_id); // liberar recursos automáticamente
    }

    close(server_fd);
    return 0;
}
