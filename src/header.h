#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
// #include <endian.h>
#include <stdint.h>
#include <time.h>
#include <arpa/inet.h>  /* htonl */
#include <string.h>     /* memcpy */
#include <ctype.h>     /* needed for isprint() */
#include <errno.h>
#include <signal.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT_1 20251
#define SERVER_PORT_2 20252
#define BUF_SIZE 4096
#define NUM_CONN 10
#define T 20
#define MAX_CONNECTION_TIME (T + 3)


// client.c
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

void *download_thread(void *arg);


// server.c
void *download_connections(void *arg);
void *upload_connections(void *arg);
void *connection_handler_download(void *arg);
void *connection_handler_upload(void *arg);
typedef struct {
    int sock;
    struct sockaddr_in addr;  // ← para identificar IP del cliente
} client_info;