#include "header.h"

long long total_bytes = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
const char *dst_ip;

double medir_rtt() {
    double avg_rtt = 0.0;
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
    inet_pton(AF_INET, dst_ip, &serv_addr.sin_addr);


    for (int i = 0; i < 3; ++i) {
        msg[0] = 0xFF;
        for (int j = 1; j < 4; ++j)
            msg[j] = rand() % 256;

        struct timeval t1, t2;
        gettimeofday(&t1, NULL);

        if (sendto(udp_sock, msg, 4, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 4) {
            perror("sendto failed");
            close(udp_sock);
            exit(EXIT_FAILURE);
        }

        // fd_set readfds;
        // FD_ZERO(&readfds);
        // FD_SET(udp_sock, &readfds);

        // struct timeval timeout;
        // timeout.tv_sec = 10;
        // timeout.tv_usec = 0;

        // int ready = select(udp_sock + 1, &readfds, NULL, NULL, &timeout);

        // if (ready == -1) {
        //     perror("select failed");
        //     close(udp_sock);
        //     exit(EXIT_FAILURE);
        // } else if (ready == 0) {
        //     fprintf(stderr, "Intento %d: Timeout esperando respuesta del servidor\n", i + 1);
        //     if (i < 2) {
        //         sleep(1);  // espera 1 segundo antes del siguiente intento
        //         continue;  // continúa con el siguiente intento
        //     } else {
        //         fprintf(stderr, "Error: Timeout definitivo tras 3 intentos.\n");
        //         close(udp_sock);
        //         exit(EXIT_FAILURE);
        //     }
        // }

        socklen_t addr_len = sizeof(serv_addr);
        ssize_t n = recvfrom(udp_sock, recv_buf, 4, 0, (struct sockaddr *)&serv_addr, &addr_len);

        gettimeofday(&t2, NULL);

        if (n != 4 || memcmp(msg, recv_buf, 4) != 0) {
            fprintf(stderr, "Error: respuesta inválida o no coincide\n");
            close(udp_sock);
            exit(EXIT_FAILURE);
        }

        double rtt = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
        avg_rtt += rtt;
        sleep(1);
    }
    avg_rtt /= 3.0;
    close(udp_sock);
    return avg_rtt;
}


/*
double medir_rtt() {
    double avg_rtt = 0.0;
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
    inet_pton(AF_INET, dst_ip, &serv_addr.sin_addr);

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
        avg_rtt += rtt;
        sleep(1);
    }
    avg_rtt /= 3.0;
    close(udp_sock);
    return avg_rtt;
}
*/



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

    if (inet_pton(AF_INET, dst_ip, &serv_addr.sin_addr) <= 0) {
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

    if (inet_pton(AF_INET, dst_ip, &serv_addr.sin_addr) <= 0) {
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

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket UDP");
        return;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(SERVER_PORT_1);
    inet_pton(AF_INET, dst_ip, &servaddr.sin_addr);

    uint32_t net_id = htonl(test_id);
    if (sendto(sockfd, &net_id, sizeof(net_id), 0,
               (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("sendto");
        close(sockfd);
        return;
    }

    ssize_t n = recvfrom(sockfd, recv_buf, sizeof(recv_buf)-1, 0, NULL, NULL);
    if (n < 0) {
        perror("recvfrom");
        close(sockfd);
        return;
    }
    recv_buf[n] = '\0';  

    uint32_t rnet; memcpy(&rnet,recv_buf,4);
    if (ntohl(rnet)!=test_id) 
        fprintf(stderr,"ID mismatch\n");
    else {
        BW_result res;
        int ret = unpackResultPayload(&res, recv_buf, n);
        if (ret<0) fprintf(stderr,"Error unpack: %d\n",ret);
    }
    close(sockfd);
}


void *rtt_thread(void *arg) {
    rtt_thread_info *info = (rtt_thread_info *)arg;
    info->rtt_result = medir_rtt() / 1000.0;
    return NULL;
}




int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Necesita como único argumento el IP destino.\n");
        return 1;
    }   

    dst_ip = argv[1];

    char timestamp[20]; 
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);


    srand(time(NULL));  

    // Etapa Idle
    printf("Etapa IDLE corriendo...\n");
    double idle_rtt = medir_rtt() / 1000.0;       

    //-----------download main-----------
    printf("Etapa DOWNLOAD corriendo...\n");
    pthread_t rtt_download_thread;
    rtt_thread_info dwld_rtt_info;
    pthread_create(&rtt_download_thread, NULL, rtt_thread, &dwld_rtt_info);


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

    pthread_join(rtt_download_thread, NULL);
    double dwld_rtt = dwld_rtt_info.rtt_result;


    gettimeofday(&t1, NULL);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_usec - t0.tv_usec) / 1e6;

    double throughput = 8.0 * total_bytes / elapsed / 1e6; 

    double dwld_throughput = throughput * 1000000.0;  

    //-----------upload main-----------
    printf("Etapa UPLOAD corriendo...\n");
    pthread_t rtt_upload_thread;
    rtt_thread_info upld_rtt_info;
    pthread_create(&rtt_upload_thread, NULL, rtt_thread, &upld_rtt_info);


    pthread_t upload_threads[NUM_CONN];
    upload_thread_info up_infos[NUM_CONN];


    srand(time(NULL));  
    uint32_t test_id;
    do {
        test_id = (uint32_t) rand();
    } while ((test_id >> 24) == 0xFF);

    for (int i = 0; i < NUM_CONN; ++i) {
        up_infos[i].id = i;
        up_infos[i].bytes_sent = 0;
        up_infos[i].test_id = test_id;    
        up_infos[i].conn_id = i + 1;      
        pthread_create(&upload_threads[i], NULL, upload_thread, &up_infos[i]);
    }

    for (int i = 0; i < NUM_CONN; ++i) {
        pthread_join(upload_threads[i], NULL);
    }
    pthread_join(rtt_upload_thread, NULL);
    double upld_rtt = upld_rtt_info.rtt_result;


    double total_bytes_sent = 0;
    for (int i = 0; i < NUM_CONN; ++i) {
        total_bytes_sent += up_infos[i].bytes_sent;
    }

    double upld_throughput = 8.0 * total_bytes_sent / elapsed / 1e6 * 1000000.0;

    sleep(1);
    query_upload_results(test_id);

    //-----------JSON main-----------

    char src_ip[INET_ADDRSTRLEN];
    {
        int s = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in tmp;
        socklen_t len = sizeof(tmp);
        memset(&tmp, 0, sizeof(tmp));
        tmp.sin_family = AF_INET;
        tmp.sin_addr.s_addr = inet_addr(dst_ip); 
        tmp.sin_port = htons(53);                   
        connect(s, (struct sockaddr*)&tmp, sizeof(tmp));
        getsockname(s, (struct sockaddr*)&tmp, &len);
        inet_ntop(AF_INET, &tmp.sin_addr, src_ip, sizeof(src_ip));
        close(s);
    }

    char json[512];
    snprintf(json, sizeof(json),
        "{"
        "\"src_ip\": \"%s\", "
        "\"dst_ip\": \"%s\", "
        "\"timestamp\": \"%s\", "
        "\"avg_bw_download_bps\": %.0f, "
        "\"avg_bw_upload_bps\": %.0f, "
        "\"num_conns\": %d, "
        "\"rtt_idle\": %.3f, "
        "\"rtt_download\": %.3f, "
        "\"rtt_upload\": %.3f"
        "}",
        src_ip, dst_ip, timestamp,
        dwld_throughput, upld_throughput,
        NUM_CONN,
        idle_rtt, dwld_rtt, upld_rtt
    );

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT_1);
    inet_pton(AF_INET, dst_ip, &dest_addr.sin_addr);

    ssize_t sent = sendto(sock, json, strlen(json), 0,
                          (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (sent < 0) {
        perror("sendto");
        close(sock);
        return 1;
    }

    printf("JSON enviado (%ld bytes):\n%s\n", sent, json);
    close(sock);

    return 0;
}
