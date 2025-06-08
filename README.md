## 1 · Enunciado resumido

La cátedra solicita implementar un **mini-Speedtest** que mida tres métricas entre un cliente y un servidor:

| Métrica | Quién genera el tráfico | Puerto / Protocolo | Resultado |
|---------|------------------------|--------------------|-----------|
| **Download** (bajada)   | Servidor → Cliente | TCP 20251 | Throughput bit/s |
| **Upload** (subida)     | Cliente → Servidor | TCP 20252 | Throughput bit/s |
| **Latencia + Jitter**   | Cliente ↔ Servidor | UDP 20251 | RTT medio y dispersión |

Al finalizar la prueba, el cliente debe empaquetar todo en un JSON y enviarlo por UDP a un colector (Logstash).

---

## 2 · Alcance de esta entrega

**ruta de Download**:

* El servidor escucha en **0.0.0.0:20251/TCP**.  
* Cada cliente aceptado se atiende en un `fork()` hijo que envía bloques de 1 MiB durante *T* = 10 s (configurable).  
* El cliente se conecta, lee durante esos 10 s, cuenta bytes y calcula  
  \[
  \text{Throughput} = \frac{\text{bytes} \times 8}{T} \quad[\text{bit/s}]
  \]
* Probado en loopback (≈800 Mb/s) y en red Wi-Fi (≈220 Mb/s), valores coherentes.

Pendiente para la versión final:

* **Latencia + Jitter** (UDP echo)  
* **Upload** (TCP 20252 + `handle_result.c`)  
* Envío del JSON final al colector

---