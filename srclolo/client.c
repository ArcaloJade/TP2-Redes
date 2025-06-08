// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT_1 20251
#define SERVER_PORT_2 20252
#define BUF_SIZE 4096
#define T 20
#define MAX_CONNECTION_TIME (T + 3)

typedef struct {
    int id;
    long long bytes_received;
} download_thread_info;

typedef struct {
    int id;
    long long bytes_sent;
    uint32_t test_id;
    uint16_t conn_id;
} upload_thread_info;

// struct BW_result {
//     uint32_t id_measurement;
//     uint64_t *conn_bytes;
//     double *conn_duration;
// };

long long total_bytes = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *client_thread(void *arg) {
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
        pthread_create(&threads[i], NULL, client_thread, &infos[i]);
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
