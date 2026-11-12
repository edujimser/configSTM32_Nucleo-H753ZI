/* web_sig.h - Señales de comunicacion (CAN/TRDP) con forzado individual. Ver web_sig_cfg.h.
 * No hace falta llamar a nada: web_bus.c ya llama a WebSig_OnRx() desde WebBus_Rx() y a
 * WebSig_TxPoll() desde WebBus_TxPoll(). El hilo de red llama a WebSig_Tick1s(). */
#ifndef WEB_SIG_H
#define WEB_SIG_H
#include <stddef.h>
#include <stdint.h>

#define WEBSIG_MAX  16          /* señales por bus (RX + TX) */

typedef struct {
  float    val;                 /* valor fisico actual (recibido, enviado o forzado) */
  float    fval;                /* valor forzado (ESCRIBIBLE desde la web) */
  uint16_t age;                /* RX: segundos desde la ultima trama (65535 = nunca) */
  uint8_t  fen;                /* 1 = esta señal esta forzada (ESCRIBIBLE desde la web) */
  uint8_t  pad;
} WebSigRt_t;

extern WebSigRt_t web_sig_rt[2][WEBSIG_MAX];

void WebSig_OnRx(int bus, uint32_t id, uint8_t len, const uint8_t *data);
int  WebSig_TxPoll(int bus, uint32_t now_ms, uint32_t *id, uint8_t *len, uint8_t data[8]);
void WebSig_Tick1s(void);
int  WebSig_Catalog(int bus, unsigned idx, char *buf, size_t size);   /* JSON de una señal; 0 = no hay mas */

#endif
