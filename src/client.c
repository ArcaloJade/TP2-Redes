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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <N conexiones>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    pthread_t threads[N];
    download_thread_info infos[N];

    struct timeval t0, t1;
    gettimeofday(&t0, NULL);

    for (int i = 0; i < N; ++i) {
        infos[i].id = i;
        infos[i].bytes_received = 0;
        pthread_create(&threads[i], NULL, download_thread, &infos[i]);
    }

    for (int i = 0; i < N; ++i) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&t1, NULL);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_usec - t0.tv_usec) / 1e6;

    double throughput = 8.0 * total_bytes / elapsed / 1e6; // Mbps

    printf("Total bytes received: %lld bytes\n", total_bytes);
    printf("Elapsed time: %.3f seconds\n", elapsed);
    printf("Download throughput: %.3f Mbps\n", throughput);

    return 0;
}
