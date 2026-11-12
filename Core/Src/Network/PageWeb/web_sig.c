/* web_sig.c - ver web_sig.h */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "Network/PageWeb/web_bus.h"
#include "Network/PageWeb/web_sig.h"
#include "Network/PageWeb/web_sig_cfg.h"

WebSigRt_t web_sig_rt[2][WEBSIG_MAX];

typedef struct {
  const char *name; uint32_t frame; uint8_t sb, len, sgn, tx;
  float factor, offset; const char *unit, *desc;
  void (*set)(float); float (*get)(void);
} SigCfg_t;
typedef struct { uint32_t id; uint8_t dlc; uint32_t period; uint32_t next; uint8_t armed; } FrameCfg_t;

/* ---- funciones generadas a partir de las listas ---- */
#define XS(n, id, sb, ln, sg, fa, of, un, de, set) static void set_##n(float v) { (void) v; set; }
#define XG(n, id, sb, ln, sg, fa, of, un, de, get) static float get_##n(void) { return (float) (get); }
WEBSIG_CAN_RX(XS)  WEBSIG_CAN_TX(XG)  WEBSIG_TRDP_RX(XS)  WEBSIG_TRDP_TX(XG)
#undef XS
#undef XG

#define XR(n, id, sb, ln, sg, fa, of, un, de, set) { #n, id, sb, ln, sg, 0, fa, of, un, de, set_##n, 0 },
#define XT(n, id, sb, ln, sg, fa, of, un, de, get) { #n, id, sb, ln, sg, 1, fa, of, un, de, 0, get_##n },
static const SigCfg_t cfg_can[]  = { {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, WEBSIG_CAN_RX(XR)  WEBSIG_CAN_TX(XT) };
static const SigCfg_t cfg_trdp[] = { {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, WEBSIG_TRDP_RX(XR) WEBSIG_TRDP_TX(XT) };
#undef XR
#undef XT
#define XF(id, dlc, per) { id, dlc, per, 0, 0 },
static FrameCfg_t fr_can[]  = { {0, 0, 0, 0, 0}, WEBSIG_CAN_FRAMES(XF) };
static FrameCfg_t fr_trdp[] = { {0, 0, 0, 0, 0}, WEBSIG_TRDP_FRAMES(XF) };
#undef XF

#define NCAN   (sizeof(cfg_can) / sizeof(cfg_can[0]) - 1)
#define NTRDP  (sizeof(cfg_trdp) / sizeof(cfg_trdp[0]) - 1)
_Static_assert(sizeof(cfg_can) / sizeof(cfg_can[0]) - 1 <= WEBSIG_MAX, "web_sig_cfg.h: demasiadas señales CAN (maximo 16)");
_Static_assert(sizeof(cfg_trdp) / sizeof(cfg_trdp[0]) - 1 <= WEBSIG_MAX, "web_sig_cfg.h: demasiadas señales TRDP (maximo 16)");

static const SigCfg_t *cfg_of(int bus) { return (bus ? cfg_trdp : cfg_can) + 1; }
static unsigned n_of(int bus) { return bus ? NTRDP : NCAN; }
static FrameCfg_t *fr_of(int bus, unsigned *n) {
  if (bus) { *n = sizeof(fr_trdp) / sizeof(fr_trdp[0]) - 1; return fr_trdp + 1; }
  *n = sizeof(fr_can) / sizeof(fr_can[0]) - 1; return fr_can + 1;
}

static uint32_t get_bits(const uint8_t *d, unsigned sb, unsigned len) {
  uint32_t r = 0;
  for (unsigned b = 0; b < len; b++) { unsigned p = sb + b; r |= (uint32_t) ((d[p >> 3] >> (p & 7)) & 1u) << b; }
  return r;
}
static void put_bits(uint8_t *d, unsigned sb, unsigned len, uint32_t v) {
  for (unsigned b = 0; b < len; b++) {
    unsigned p = sb + b; uint8_t m = (uint8_t) (1u << (p & 7));
    if ((v >> b) & 1u) d[p >> 3] |= m; else d[p >> 3] &= (uint8_t) ~m;
  }
}

void WebSig_OnRx(int bus, uint32_t id, uint8_t len, const uint8_t *data) {
  if (bus < 0 || bus > 1) return;
  const SigCfg_t *c = cfg_of(bus); unsigned n = n_of(bus);
  for (unsigned i = 0; i < n; i++) {
    if (c[i].tx || c[i].frame != id || (unsigned) (c[i].sb + c[i].len + 7u) / 8u > len) continue;
    uint32_t raw = get_bits(data, c[i].sb, c[i].len);
    float v;
    if (c[i].sgn && c[i].len < 32 && (raw & (1u << (c[i].len - 1)))) v = (float) (int32_t) (raw | (0xFFFFFFFFu << c[i].len));
    else if (c[i].sgn && c[i].len == 32) v = (float) (int32_t) raw;
    else v = (float) raw;
    v = v * c[i].factor + c[i].offset;
    WebSigRt_t *r = &web_sig_rt[bus][i];
    r->age = 0;
    if (web_bus_st[bus].fsg && r->fen) v = r->fval;
    r->val = v;
    c[i].set(v);
  }
}

int WebSig_TxPoll(int bus, uint32_t now_ms, uint32_t *id, uint8_t *len, uint8_t data[8]) {
  if (bus < 0 || bus > 1) return 0;
  unsigned nf; FrameCfg_t *f = fr_of(bus, &nf);
  const SigCfg_t *c = cfg_of(bus); unsigned n = n_of(bus);
  for (unsigned k = 0; k < nf; k++) {
    if (!f[k].armed) { f[k].armed = 1; f[k].next = now_ms; }
    if ((int32_t) (now_ms - f[k].next) < 0) continue;
    f[k].next += f[k].period;
    if ((int32_t) (now_ms - f[k].next) >= 0) f[k].next = now_ms + f[k].period;
    memset(data, 0, 8);
    for (unsigned i = 0; i < n; i++) {
      if (!c[i].tx || c[i].frame != f[k].id) continue;
      WebSigRt_t *r = &web_sig_rt[bus][i];
      float v = (web_bus_st[bus].fsg && r->fen) ? r->fval : c[i].get();
      r->val = v;
      float rawf = (v - c[i].offset) / c[i].factor;
      int64_t raw = (int64_t) lroundf(rawf);
      int64_t mx = c[i].sgn ? ((int64_t) 1 << (c[i].len - 1)) - 1 : ((int64_t) 1 << c[i].len) - 1;
      int64_t mn = c[i].sgn ? -((int64_t) 1 << (c[i].len - 1)) : 0;
      if (raw > mx) raw = mx;
      if (raw < mn) raw = mn;
      if ((c[i].sb + c[i].len + 7u) / 8u <= f[k].dlc) put_bits(data, c[i].sb, c[i].len, (uint32_t) raw);
    }
    *id = f[k].id; *len = f[k].dlc > 8 ? 8 : f[k].dlc;
    web_bus_st[bus].txn++;
    return 1;
  }
  return 0;
}

void WebSig_Tick1s(void) {
  for (int b = 0; b < 2; b++) {
    const SigCfg_t *c = cfg_of(b); unsigned n = n_of(b);
    for (unsigned i = 0; i < n; i++) {
      WebSigRt_t *r = &web_sig_rt[b][i];
      if (c[i].tx) continue;
      if (r->age < 65535u) r->age++;
      if (web_bus_st[b].fsg && r->fen) { r->val = r->fval; c[i].set(r->fval); }   /* el forzado se mantiene sin trafico */
    }
  }
}

static size_t esc(char *o, size_t cap, const char *s) {
  size_t n = 0;
  for (; *s && n + 7 < cap; s++) {
    unsigned char ch = (unsigned char) *s;
    if (ch == '"' || ch == '\\') { o[n++] = '\\'; o[n++] = (char) ch; } else if (ch < 0x20) o[n++] = ' '; else o[n++] = (char) ch;
  }
  o[n] = '\0'; return n;
}
int WebSig_Catalog(int bus, unsigned idx, char *buf, size_t size) {
  if (bus < 0 || bus > 1 || idx >= n_of(bus)) return 0;
  const SigCfg_t *c = &cfg_of(bus)[idx];
  char n[40], u[16], d[96];
  esc(n, sizeof n, c->name); esc(u, sizeof u, c->unit); esc(d, sizeof d, c->desc);
  int k = snprintf(buf, size, "{\"n\":\"%s\",\"d\":\"%s\",\"u\":\"%s\",\"i\":%lu,\"r\":%u,\"b\":%u,\"l\":%u}",
                   n, d, u, (unsigned long) c->frame, c->tx ? 0u : 1u, (unsigned) c->sb, (unsigned) c->len);
  return (k < 0 || (size_t) k >= size) ? 0 : k;
}
