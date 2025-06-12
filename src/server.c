# include "header.h"

// — Estructuras y globals —
BW_result results[MAX_RESULTS];
int result_count = 0;
pthread_mutex_t results_mutex = PTHREAD_MUTEX_INITIALIZER;

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

    uint32_t test_id = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
    uint16_t conn_id = (header[4] << 8) | header[5];

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

    double duration = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1e6;
    pthread_mutex_lock(&results_mutex);
    int found = 0;
    for (int i = 0; i < result_count; ++i) {
        if (results[i].id_measurement == test_id) {
            results[i].conn_bytes[conn_id-1]    = total_bytes;
            results[i].conn_duration[conn_id-1] = duration;
            found = 1;
            break;
        }
    }
    if (!found && result_count < MAX_RESULTS) {
        results[result_count].id_measurement             = test_id;
        results[result_count].conn_bytes[conn_id-1]      = total_bytes;
        results[result_count].conn_duration[conn_id-1]   = duration;
        result_count++;
    }
    pthread_mutex_unlock(&results_mutex);

    close(sock);
    pthread_exit(NULL);
    return NULL;
}

void *udp_result_server(void *arg) {

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket udp");
        pthread_exit(NULL);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #ifdef SO_REUSEPORT
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    #endif

    struct sockaddr_in addr, cliaddr;
    socklen_t cli_len = sizeof(cliaddr);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(SERVER_PORT_1);


    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind udp");
        close(sockfd);
        pthread_exit(NULL);
    }

    uint8_t  recv_buf[4];
    uint8_t  send_buf[4096];
    while (1) {
        ssize_t n = recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0,
                             (struct sockaddr *)&cliaddr, &cli_len);
        if (n != 4) continue;

        if (recv_buf[0] == 0xFF) {
            sendto(sockfd, recv_buf, 4, 0,
                   (struct sockaddr *)&cliaddr, cli_len);
            continue;
        }

        uint32_t net_id;
        memcpy(&net_id, recv_buf, 4);
        uint32_t test_id = ntohl(net_id);

        pthread_mutex_lock(&results_mutex);
        BW_result r = {0};
        int found=0;
        for(int i=0;i<result_count;i++){
            if(results[i].id_measurement==test_id){
                r = results[i]; 
                found=1; 
                break;
            }
        }
        pthread_mutex_unlock(&results_mutex);
        if(!found) continue;

        int plen = packResultPayload(r,send_buf,sizeof(send_buf));
        if(plen>0) {
        sendto(sockfd,send_buf,plen,0,(struct sockaddr*)&cliaddr,cli_len);
        }
    }
    close(sockfd);
    return NULL;
}

void *download_connections(void *arg){
    int server_fd, new_socket;
    struct sockaddr_in address, client_addr;
        socklen_t clilen = sizeof(client_addr);
    signal(SIGPIPE, SIG_IGN); 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #ifdef SO_REUSEPORT
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    #endif

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

    printf("Servidor escuchando DOWNLOAD en puerto %d...\n", SERVER_PORT_1);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &clilen);
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

        pthread_detach(thread_id); 
    }

    close(server_fd);
    return NULL;
}

void *upload_connections(void *arg) {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    signal(SIGPIPE, SIG_IGN); 

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket upload failed");
        pthread_exit(NULL);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT_2);  

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

        pthread_detach(thread_id); 
    }

    close(server_fd);
    pthread_exit(NULL);
    return NULL;
}


int main() {    
    pthread_t download_thread, upload_thread, udp_thread;

    pthread_create(&download_thread, NULL, download_connections, NULL);
    pthread_create(&upload_thread,   NULL, upload_connections,   NULL);
    pthread_create(&udp_thread,      NULL, udp_result_server,    NULL);

    pthread_join(download_thread, NULL);
    pthread_join(upload_thread,   NULL);
    pthread_join(udp_thread,      NULL);

    return 0;
}
