/* Servidor extremadamente simple:
 * 1. Abre listener 0.0.0.0:20251
 * 2. Por cada cliente que llega, hace fork()
 * 3. El hijo envía bloques de 1 MiB durante 10 s y termina */
#include "common.h"

/* ---------- hijo: genera tráfico de bajada ---------- */
void download_worker(int cli_fd)
{
    static uint8_t chunk[1<<20] = {0};   /* 1 MiB inicializado en cero   */
    double t0 = now_sec();               /* hora de arranque             */

    while (now_sec() - t0 < TEST_DURATION + 2) /* +2 s de colchón         */
        write(cli_fd, chunk, sizeof chunk);     /* bombardeo de bytes      */

    close(cli_fd);                               /* cierra conexión TCP     */
    exit(0);                                     /* termina el proceso hijo */
}

/* ---------- proceso principal ---------- */
int main(void)
{
    int lfd = tcp_listen(PORT_DOWNLOAD, 128);   /* abre puerto 20251 */
    if (lfd < 0) exit(1);

    for (;;)                                    /* loop infinito     */
    {
        int cfd = accept(lfd, NULL, NULL);      /* espera cliente */
        if (cfd < 0) continue;                  /* error espurio  */

        if (!fork()) {                          /* hijo = 0 → lógica */
            close(lfd);                         /* hijo no usa listener */
            download_worker(cfd);
        }
        close(cfd);                             /* padre libera fd y    */
    }                                           /* vuelve a aceptar     */
}
