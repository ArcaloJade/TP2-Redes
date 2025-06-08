/* Cliente mínimo: se conecta, recibe N segundos, calcula Mb/s */
#include "common.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "uso: %s <ip-servidor>\n", argv[0]);
        return 1;
    }
    const char *ip = argv[1];

    int s = tcp_connect(ip, PORT_DOWNLOAD);   /* handshake TCP */
    if (s < 0) return 1;

    uint8_t  buf[1<<15];      /* 32 KiB de buffer temporal   */
    uint64_t bytes = 0;       /* contador acumulado          */
    double   t0 = now_sec();  /* hora de inicio              */

    while (now_sec() - t0 < TEST_DURATION) {  /* lee 10 s redondos */
        ssize_t n = read(s, buf, sizeof buf);
        if (n <= 0) break;    /* 0=FIN conexión, <0=error      */
        bytes += n;
    }
    close(s);

    double bps = (bytes * 8.0) / TEST_DURATION;      /* bits/seg */
    printf("Download: %.2f Mb/s\n", bps / 1e6);      /* a megabits*/

    return 0;
}
