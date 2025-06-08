#include "common.h"

/* ───────── ahora/segundos (double) ─────────
 * Usamos CLOCK_MONOTONIC porque nunca retrocede (no sufre cambios de
 * hora del sistema) y da resolución de nanosegundos. */
double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ───────── tcp_connect() ─────────
 * Crea un socket IPv4 TCP y lo conecta a ip:port.
 * Devuelve el file descriptor o -1 con perror(). */
int tcp_connect(const char *ip, uint16_t port)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);            /* 1. socket()   */
    if (s < 0) { perror("socket"); return -1; }

    struct sockaddr_in sa = {0};                        /* 2. destino    */
    sa.sin_family = AF_INET;                            /*   familia     */
    sa.sin_port   = htons(port);                        /*   puerto 20251*/
    inet_pton(AF_INET, ip, &sa.sin_addr);               /*   IP texto→bin*/

    if (connect(s, (struct sockaddr*)&sa, sizeof sa) < 0)/* 3. handshake  */
    {
        perror("connect");
        close(s);
        return -1;
    }
    return s;   /* listo para read()/write() */
}

/* ───────── tcp_listen() ─────────
 * Crea un socket servidor que queda en estado LISTEN en 0.0.0.0:port.
 * backlog es la cola máxima de conexiones “handshakeadas” pendientes. */
int tcp_listen(uint16_t port, int backlog)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return -1; }

    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes); /* reuse */

    struct sockaddr_in sa = {0};
    sa.sin_family      = AF_INET;
    sa.sin_port        = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;                /* 0.0.0.0 */

    if (bind(s, (struct sockaddr*)&sa, sizeof sa) < 0)
    { perror("bind"); close(s); return -1; }

    if (listen(s, backlog) < 0)
    { perror("listen"); close(s); return -1; }

    return s;   /* descriptor en LISTEN */
}