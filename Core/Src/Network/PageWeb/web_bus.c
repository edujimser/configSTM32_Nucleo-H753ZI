/* web_bus.c - ver web_bus.h */
#include <string.h>
#include "Network/PageWeb/web_bus.h"
#include "Network/PageWeb/web_sig.h"

WebBusRx_t web_bus_rx[2][WEBBUS_RX_N];
WebBusTx_t web_bus_tx[2][WEBBUS_TX_N];
WebBusSt_t web_bus_st[2];

static uint32_t rx_prev[2][WEBBUS_RX_N];        /* contador del segundo anterior */
static uint32_t tx_next[2][WEBBUS_TX_N];        /* proximo instante de envio */
static uint8_t  tx_arm[2][WEBBUS_TX_N];         /* 1 = ya se fijo tx_next */
static uint8_t  tx_once[2][WEBBUS_TX_N];

static void pack(const uint8_t *d, uint8_t n, uint32_t *lo, uint32_t *hi) {
  uint8_t b[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  if (n > 8) n = 8;
  for (uint8_t i = 0; i < n; i++) b[i] = d[i];
  *lo = (uint32_t) b[0] | ((uint32_t) b[1] << 8) | ((uint32_t) b[2] << 16) | ((uint32_t) b[3] << 24);
  *hi = (uint32_t) b[4] | ((uint32_t) b[5] << 8) | ((uint32_t) b[6] << 16) | ((uint32_t) b[7] << 24);
}
static void unpack(uint32_t lo, uint32_t hi, uint8_t d[8]) {
  d[0] = (uint8_t) lo; d[1] = (uint8_t) (lo >> 8); d[2] = (uint8_t) (lo >> 16); d[3] = (uint8_t) (lo >> 24);
  d[4] = (uint8_t) hi; d[5] = (uint8_t) (hi >> 8); d[6] = (uint8_t) (hi >> 16); d[7] = (uint8_t) (hi >> 24);
}

void WebBus_Rx(int bus, uint32_t id, uint8_t len, const uint8_t *data) {
  if (bus < 0 || bus > 1) return;
  WebBusRx_t *s = 0;
  for (int i = 0; i < WEBBUS_RX_N; i++) if (web_bus_rx[bus][i].used && web_bus_rx[bus][i].id == id) { s = &web_bus_rx[bus][i]; break; }
  if (!s) for (int i = 0; i < WEBBUS_RX_N; i++) if (!web_bus_rx[bus][i].used) {
    s = &web_bus_rx[bus][i]; s->id = id; s->cnt = 0; s->hz = 0; s->used = 1; break;
  }
  web_bus_st[bus].rxn++;
  WebSig_OnRx(bus, id, len, data);                      /* señales definidas en web_sig_cfg.h */
  if (!s) { web_bus_st[bus].drop++; return; }
  s->cnt++; s->age = 0; s->len = len;
  if (!web_bus_st[bus].frx) pack(data, len, (uint32_t *) &s->lo, (uint32_t *) &s->hi);   /* forzado: no se pisa */
}

int WebBus_GetRx(int bus, uint32_t id, uint8_t data[8], uint8_t *len) {
  if (bus < 0 || bus > 1) return 0;
  for (int i = 0; i < WEBBUS_RX_N; i++) {
    const WebBusRx_t *s = &web_bus_rx[bus][i];
    if (s->used && s->id == id) { unpack(s->lo, s->hi, data); if (len) *len = s->len; return 1; }
  }
  return 0;
}

int WebBus_TxForced(int bus) { return (bus >= 0 && bus <= 1) ? web_bus_st[bus].ftx : 0; }

int WebBus_TxPoll(int bus, uint32_t now_ms, uint32_t *id, uint8_t *len, uint8_t data[8]) {
  if (bus < 0 || bus > 1) return 0;
  if (!web_bus_st[bus].ftx) return WebSig_TxPoll(bus, now_ms, id, len, data);   /* tramas de web_sig_cfg.h */
  for (int i = 0; i < WEBBUS_TX_N; i++) {
    WebBusTx_t *t = &web_bus_tx[bus][i];
    if (!t->en) { tx_arm[bus][i] = 0; tx_once[bus][i] = 0; continue; }
    if (t->per == 0) { if (tx_once[bus][i]) continue; tx_once[bus][i] = 1; }
    else {
      if (!tx_arm[bus][i]) { tx_arm[bus][i] = 1; tx_next[bus][i] = now_ms; }
      if ((int32_t) (now_ms - tx_next[bus][i]) < 0) continue;
      tx_next[bus][i] += t->per;
      if ((int32_t) (now_ms - tx_next[bus][i]) >= 0) tx_next[bus][i] = now_ms + t->per;   /* no acumular retraso */
    }
    *id = t->id; *len = t->len > 8 ? 8 : t->len; unpack(t->lo, t->hi, data);
    web_bus_st[bus].txn++;
    return 1;
  }
  return WebSig_TxPoll(bus, now_ms, id, len, data);
}

void WebBus_SetState(int bus, uint8_t estado, uint8_t carga) {
  if (bus < 0 || bus > 1) return;
  web_bus_st[bus].estado = estado; web_bus_st[bus].carga = carga;
}
void WebBus_CountErr(int bus) { if (bus >= 0 && bus <= 1) web_bus_st[bus].err++; }

void WebBus_Force(int bus, int tipo, int on) {
  if (bus < 0 || bus > 1) return;
  if (tipo == 2) web_bus_st[bus].fsg = on ? 1 : 0; else if (tipo == 1) web_bus_st[bus].ftx = on ? 1 : 0; else web_bus_st[bus].frx = on ? 1 : 0;
  if (on) web_bus_st[bus].left = WEBBUS_FORCE_TIMEOUT_S;
  if (!web_bus_st[bus].ftx && !web_bus_st[bus].frx && !web_bus_st[bus].fsg) web_bus_st[bus].left = 0;
  if (!on && tipo == 1) for (int i = 0; i < WEBBUS_TX_N; i++) { tx_arm[bus][i] = 0; tx_once[bus][i] = 0; }
}

void WebBus_Tick1s(void) {
  for (int b = 0; b < 2; b++) {
    for (int i = 0; i < WEBBUS_RX_N; i++) {
      WebBusRx_t *s = &web_bus_rx[b][i];
      if (!s->used) continue;
      s->hz = (uint16_t) (s->cnt - rx_prev[b][i]);
      rx_prev[b][i] = s->cnt;
      if (s->hz) s->age = 0; else if (s->age < 65535u) s->age++;
    }
    if ((web_bus_st[b].frx || web_bus_st[b].ftx || web_bus_st[b].fsg) && web_bus_st[b].left) {
      if (--web_bus_st[b].left == 0) { web_bus_st[b].frx = 0; web_bus_st[b].ftx = 0; web_bus_st[b].fsg = 0; }   /* se libera solo */
    }
  }
}
