#include "header.h"

long long total_bytes = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

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

int main(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "no necesita argumentos\n");
        return 1;
    }   

    // int N = atoi(argv[1]);
    
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

    return 0;
}
