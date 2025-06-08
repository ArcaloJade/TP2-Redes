#ifndef COMMON_H
#define COMMON_H
#define _POSIX_C_SOURCE 200112L   /* habilita CLOCK_MONOTONIC, nanosleep… */
#include <time.h>
#include <stdint.h>       /* tipos fijos, ej. uint16_t            */
#include <time.h>         /* clock_gettime() y struct timespec    */
#include <arpa/inet.h>    /* htons(), inet_pton()                 */
#include <sys/socket.h>   /* socket(), bind(), connect()…         */
#include <netinet/in.h>   /* struct sockaddr_in, AF_INET          */
#include <unistd.h>       /* read(), write(), close()             */
#include <string.h>       /* memset(), memcpy(), strlen() …       */
#include <stdio.h>        /* printf(), perror()                   */
#include <stdlib.h>       /* exit(), perror()                     */

/* ====== parámetros de la prueba (solo download) ====== */
#define PORT_DOWNLOAD 20251   /* puerto TCP fijo para bajar datos */
#define TEST_DURATION 10      /* segundos; ponelo 20 para “serio” */

/* ====== prototipos de helpers definidos en util.c ====== */
int    tcp_connect(const char *ip, uint16_t port);   /* cliente   */
int    tcp_listen(uint16_t port, int backlog);       /* servidor  */
double now_sec(void);                                /* timestamp */

#endif /* COMMON_H */