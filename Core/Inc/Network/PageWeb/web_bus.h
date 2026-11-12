/* web_bus.h - Visor/forzado de tramas de bus (CAN y TRDP) para la web.
 *
 *  RECEPCION:  tu driver llama a WebBus_Rx() con cada trama recibida. La web muestra una
 *              tabla con las ultimas ids vistas (contador, frecuencia, antiguedad, datos).
 *  Tu aplicacion debe LEER las señales con WebBus_GetRx(), no del buffer del driver:
 *              si el "forzado de recepcion" esta activo, devuelve los datos que puso la web.
 *  TRANSMISION: tu hilo de comunicaciones llama a WebBus_TxPoll() en bucle. Si el
 *              "forzado de transmision" esta activo, devuelve las tramas que la web quiere
 *              enviar (periodicas). Tu aplicacion NO debe transmitir por su cuenta mientras
 *              WebBus_TxForced() devuelva 1 (la web toma el control del bus).
 *  SEGURIDAD:  el forzado se libera solo a los WEBBUS_FORCE_TIMEOUT_S segundos.
 *
 *  Los datos se guardan como 2 palabras de 32 bits (primeros 8 bytes, little-endian:
 *  byte0 = lo & 0xFF ... byte7 = hi >> 24). Para TRDP, "id" es el ComId y "len" el tamaño del
 *  dato (la web solo muestra y fuerza los primeros 8 bytes).
 */
#ifndef WEB_BUS_H
#define WEB_BUS_H
#include <stdint.h>

#define WEBBUS_CAN   0
#define WEBBUS_TRDP  1
#define WEBBUS_RX_N  6          /* ids distintas que se muestran por bus */
#define WEBBUS_TX_N  4          /* tramas que se pueden forzar por bus   */
#define WEBBUS_FORCE_TIMEOUT_S  600u

typedef struct {
  uint32_t id, lo, hi, cnt;     /* id/ComId, bytes 0-3, bytes 4-7, tramas recibidas */
  uint16_t hz;                  /* tramas por segundo (ultimo segundo) */
  uint16_t age;                 /* segundos desde la ultima trama (65535 = nunca) */
  uint8_t  len;                 /* DLC (CAN) o tamaño (TRDP, saturado a 255) */
  uint8_t  used;
} WebBusRx_t;

typedef struct {
  uint32_t id, lo, hi;          /* ESCRIBIBLES desde la web */
  uint32_t per;                 /* periodo en ms (0 = una sola vez al activar) */
  uint8_t  len;                 /* 0..8 */
  uint8_t  en;                  /* 1 = se transmite mientras el forzado TX este activo */
} WebBusTx_t;

typedef struct {
  uint8_t  frx, ftx;            /* forzado de recepcion / transmision de TRAMAS (solo lectura en la web) */
  uint8_t  fsg;                 /* forzado de SEÑALES individuales (web_sig.c) */
  uint8_t  estado;              /* 0 sin bus, 1 error, 2 correcto (lo pones tu con WebBus_SetState) */
  uint8_t  carga;               /* % de carga del bus (lo pones tu, opcional) */
  uint16_t left;                /* segundos que quedan de forzado */
  uint32_t rxn, txn, err, drop; /* totales: recibidas, enviadas, errores, ids sin hueco */
} WebBusSt_t;

extern WebBusRx_t web_bus_rx[2][WEBBUS_RX_N];
extern WebBusTx_t web_bus_tx[2][WEBBUS_TX_N];
extern WebBusSt_t web_bus_st[2];

/* ---- lo que llamas tu ---- */
void WebBus_Rx(int bus, uint32_t id, uint8_t len, const uint8_t *data);   /* en la recepcion (hilo o ISR) */
int  WebBus_GetRx(int bus, uint32_t id, uint8_t data[8], uint8_t *len);   /* 1 si existe; respeta el forzado */
int  WebBus_TxPoll(int bus, uint32_t now_ms, uint32_t *id, uint8_t *len, uint8_t data[8]);  /* 1 = envia esta trama */
int  WebBus_TxForced(int bus);                                            /* 1 = la web controla la transmision */
void WebBus_SetState(int bus, uint8_t estado, uint8_t carga_pct);
void WebBus_CountErr(int bus);

/* ---- lo llama el hilo de red ---- */
void WebBus_Tick1s(void);                                                 /* 1 vez por segundo */
void WebBus_Force(int bus, int tipo, int on);                             /* comando de la web: tipo 0 rx, 1 tx, 2 señales */

#endif
