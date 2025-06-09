# include "header.h"

void *connection_handler_download(void *arg) {
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

void *connection_handler_upload(void *arg) {
    int sock = *(int *)arg;
    free(arg);

    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    getpeername(sock, (struct sockaddr *)&cli_addr, &cli_len);

    uint8_t header[6];
    ssize_t n = recv(sock, header, 6, MSG_WAITALL);
    if (n != 6) {
        perror("Error al recibir encabezado");
        close(sock);
        pthread_exit(NULL);
    }

    // Extraer test_id y conn_id
    uint32_t test_id = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
    uint16_t conn_id = (header[4] << 8) | header[5];

    printf("Conexión desde %s - test_id: %08X, conn_id: %d\n",
           inet_ntoa(cli_addr.sin_addr), test_id, conn_id);

    // Contar bytes recibidos durante T segundos
    char buffer[BUF_SIZE];
    long long total_bytes = 0;

    struct timeval start, now;
    gettimeofday(&start, NULL);

    while (1) {
        ssize_t received = recv(sock, buffer, BUF_SIZE, 0);
        if (received <= 0) break;

        total_bytes += received;

        gettimeofday(&now, NULL);
        double elapsed = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1e6;
        if (elapsed >= T) break;
    }

    printf("UPLOAD conn_id=%d bytes=%lld\n", conn_id, total_bytes);

    // Aquí podrías guardar `test_id`, `conn_id`, `total_bytes` y `elapsed` en una estructura global protegida por mutex

    close(sock);
    pthread_exit(NULL);
}

void *download_connections(void *arg){
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
    address.sin_port = htons(SERVER_PORT_1);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 128) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", SERVER_PORT_1);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            continue;
        }

        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, connection_handler_download, client_sock) != 0) {
            perror("pthread_create");
            close(new_socket);
            free(client_sock);
        }

        pthread_detach(thread_id); // liberar recursos automáticamente
    }

    close(server_fd);
}

void *upload_connections(void *arg) {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    signal(SIGPIPE, SIG_IGN); // Evita que el servidor muera por write() a socket cerrado

    // Crear socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket upload failed");
        pthread_exit(NULL);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Configurar dirección del servidor
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT_2);  // puerto 20252

    // Asociar socket a puerto
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind upload failed");
        close(server_fd);
        pthread_exit(NULL);
    }

    if (listen(server_fd, 128) < 0) {
        perror("listen upload failed");
        close(server_fd);
        pthread_exit(NULL);
    }

    printf("Servidor escuchando UPLOAD en puerto %d...\n", SERVER_PORT_2);

    // Aceptar conexiones entrantes en bucle
    while (1) {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) {
            perror("accept upload");
            continue;
        }

        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, connection_handler_upload, client_sock) != 0) {
            perror("pthread_create upload");
            close(new_socket);
            free(client_sock);
            continue;
        }

        pthread_detach(thread_id); // Liberar recursos del hilo automáticamente
    }

    close(server_fd);
    pthread_exit(NULL);
}

int main() {
    pthread_t download_thread, upload_thread;

    if (pthread_create(&download_thread, NULL, download_connections, NULL) != 0) {
        perror("pthread_create download");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&upload_thread, NULL, upload_connections, NULL) != 0) {
        perror("pthread_create upload");
        exit(EXIT_FAILURE);
    }

    pthread_join(download_thread, NULL);
    pthread_join(upload_thread, NULL);

    return 0;
}
