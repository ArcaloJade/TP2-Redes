#include "header.h"

long long total_bytes = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void medir_rtt() {
    int udp_sock;
    struct sockaddr_in serv_addr;
    uint8_t msg[4], recv_buf[4];

    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct timeval timeout = {10, 0};  // 10 segundos
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT_1);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    printf("Iniciando mediciones de latencia (RTT)...\n");

    for (int i = 0; i < 3; ++i) {
        // Generar payload aleatorio válido
        msg[0] = 0xFF;
        for (int j = 1; j < 4; ++j)
            msg[j] = rand() % 256;

        struct timeval t1, t2;

        if (sendto(udp_sock, msg, 4, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 4) {
            perror("sendto failed");
            close(udp_sock);
            exit(EXIT_FAILURE);
        }
        gettimeofday(&t1, NULL);

        socklen_t addr_len = sizeof(serv_addr);
        ssize_t n = recvfrom(udp_sock, recv_buf, 4, 0, (struct sockaddr *)&serv_addr, &addr_len);

        gettimeofday(&t2, NULL);

        if (n != 4 || memcmp(msg, recv_buf, 4) != 0) {
            fprintf(stderr, "Error: respuesta inválida o no coincide\n");
            close(udp_sock);
            exit(EXIT_FAILURE);
        }

        double rtt = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
        printf("RTT %d: %.3f ms\n", i + 1, rtt);

        sleep(1);
    }

    close(udp_sock);
}


void *download_thread(void *arg) {
    download_thread_info *info = (download_thread_info *)arg;
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        pthread_exit(NULL);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT_1);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        pthread_exit(NULL);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        pthread_exit(NULL);
    }

    struct timeval start, now;
    gettimeofday(&start, NULL);

    while (1) {
        ssize_t bytes = recv(sock, buffer, BUF_SIZE, 0);
        if (bytes <= 0)
            break;

        info->bytes_received += bytes;

        gettimeofday(&now, NULL);
        double elapsed = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1e6;
        if (elapsed >= T)
            break;
    }

    close(sock);

    pthread_mutex_lock(&lock);
    total_bytes += info->bytes_received;
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

void *upload_thread(void *arg) {
    upload_thread_info *info = (upload_thread_info *)arg;
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        pthread_exit(NULL);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT_2);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        pthread_exit(NULL);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        pthread_exit(NULL);
    }

    uint8_t header[6];
    header[0] = (info->test_id >> 24) & 0xFF;
    header[1] = (info->test_id >> 16) & 0xFF;
    header[2] = (info->test_id >> 8) & 0xFF;
    header[3] = info->test_id & 0xFF;
    header[4] = (info->conn_id >> 8) & 0xFF;
    header[5] = info->conn_id & 0xFF;

    send(sock, header, 6, 0);
    printf("upload_thread %d → test_id: %08X, conn_id: %04X\n", info->id, info->test_id, info->conn_id);
    struct timeval start, now;
    gettimeofday(&start, NULL);

    while (1) {
        ssize_t sent = send(sock, buffer, BUF_SIZE, 0);
        if (sent <= 0) break;
        info->bytes_sent += sent;

        gettimeofday(&now, NULL);
        double elapsed = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1e6;
        if (elapsed >= T) break;
    }
    close(sock);
    pthread_exit(NULL);
}

void query_upload_results(uint32_t test_id) {
    int sockfd;
    struct sockaddr_in servaddr;
    uint8_t recv_buf[4096];

    // 1) Crear socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket UDP");
        return;
    }

    // 2) Configurar dirección del servidor (mismo IP y puerto 20251)
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(SERVER_PORT_1);
    inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr);

    // 3) Enviar solo los 4 bytes del test_id en orden de red
    uint32_t net_id = htonl(test_id);
    if (sendto(sockfd, &net_id, sizeof(net_id), 0,
               (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("sendto");
        close(sockfd);
        return;
    }

    // 4) Recibir la respuesta completa (4 bytes + NUM_CONN líneas ASCII)
    ssize_t n = recvfrom(sockfd, recv_buf, sizeof(recv_buf)-1, 0, NULL, NULL);
    if (n < 0) {
        perror("recvfrom");
        close(sockfd);
        return;
    }
    recv_buf[n] = '\0';  // cerrar string

    // 5) Imprimir líneas que llegaron
    // Primero, verificar que los primeros 4 bytes coincidan con net_id
    uint32_t rnet; memcpy(&rnet,recv_buf,4);
    if (ntohl(rnet)!=test_id) 
        fprintf(stderr,"ID mismatch\n");
    else {
        BW_result res;
        int ret = unpackResultPayload(&res, recv_buf, n);
        if (ret<0) fprintf(stderr,"Error unpack: %d\n",ret);
        else {
            printf("Results for test_id 0x%08X:\n",res.id_measurement);
            printBwResult(res);
        }
    }
    close(sockfd);
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "no necesita argumentos\n");
        return 1;
    }   

    // int N = atoi(argv[1]);
    srand(time(NULL));  // semilla para rand()
    medir_rtt();        // hace 3 mediciones de RTT por UDP

    pthread_t download_threads[NUM_CONN];
    download_thread_info infos[NUM_CONN];

    struct timeval t0, t1;
    gettimeofday(&t0, NULL);

    for (int i = 0; i < NUM_CONN; ++i) {
        infos[i].id = i;
        infos[i].bytes_received = 0;
        pthread_create(&download_threads[i], NULL, download_thread, &infos[i]);
    }

    for (int i = 0; i < NUM_CONN; ++i) {
        pthread_join(download_threads[i], NULL);
    }

    gettimeofday(&t1, NULL);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_usec - t0.tv_usec) / 1e6;

    double throughput = 8.0 * total_bytes / elapsed / 1e6; // Mbps

    printf("Total bytes received: %lld bytes\n", total_bytes);
    printf("Elapsed time: %.3f seconds\n", elapsed);
    printf("Download throughput: %.3f Mbps\n", throughput);

    //-----------upload main-----------
    pthread_t upload_threads[NUM_CONN];
    upload_thread_info up_infos[NUM_CONN];


    srand(time(NULL));  // semilla
    uint32_t test_id;
    do {
        test_id = (uint32_t) rand();
    } while ((test_id >> 24) == 0xFF);

    for (int i = 0; i < NUM_CONN; ++i) {
        up_infos[i].id = i;
        up_infos[i].bytes_sent = 0;
        up_infos[i].test_id = test_id;    // todos los hilos comparten este test_id
        up_infos[i].conn_id = i + 1;      // conexión 1, 2, 3, ..., N
        pthread_create(&upload_threads[i], NULL, upload_thread, &up_infos[i]);
    }

    for (int i = 0; i < NUM_CONN; ++i) {
        pthread_join(upload_threads[i], NULL);
    }
    // Pequeño retardo para asegurarnos de que el servidor haya guardado todo
    sleep(1);
    query_upload_results(test_id);
    return 0;
}
