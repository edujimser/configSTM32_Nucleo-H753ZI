/* web_err.c - ver web_err.h */
#include <stdio.h>
#include <string.h>
#include "Network/PageWeb/web_err.h"
#include "Network/PageWeb/web_err_cfg.h"

volatile uint32_t err_act, err_una, err_tot;
volatile uint8_t  err_n, err_cfg;
volatile uint16_t err_cnt[WEBERR_MAX];
volatile uint32_t err_last[WEBERR_MAX];
WebErrHist_t      err_hist[WEBERR_HIST];

typedef struct {
  uint16_t id; const char *name; uint8_t sev; uint8_t delay;
  const char *vars; const char *desc; const char *cond;
  uint8_t (*cond_fn)(void); float (*val_fn)(void);
} ErrCfg_t;

#define X(id, ident, sev, delay, cond, vars, desc, val) \
  static uint8_t ec_##ident(void) { return (cond) ? 1u : 0u; } \
  static float   ev_##ident(void) { return (float) (val); }
WEBERR_LIST(X)
#undef X
#define X(id, ident, sev, delay, cond, vars, desc, val) { id, #ident, sev, delay, vars, desc, #cond, ec_##ident, ev_##ident },
static const ErrCfg_t cfg[] = { WEBERR_LIST(X) };
#undef X
#define ERR_N  (sizeof(cfg) / sizeof(cfg[0]))
_Static_assert(sizeof(cfg) / sizeof(cfg[0]) <= WEBERR_MAX, "web_err_cfg.h: demasiados errores (maximo WEBERR_MAX)");

static uint8_t  on_s[WEBERR_MAX];        /* segundos seguidos con la condicion verdadera */
static uint8_t  test_s[WEBERR_MAX];      /* segundos de simulacion que quedan */
static uint32_t since[WEBERR_MAX];

static void hist_push(uint32_t t, uint16_t id, uint8_t ev, uint32_t dur, float val) {
  memmove(&err_hist[1], &err_hist[0], sizeof(err_hist[0]) * (WEBERR_HIST - 1));
  err_hist[0].t = t; err_hist[0].id = id; err_hist[0].ev = ev; err_hist[0].dur = dur; err_hist[0].val = val;
}

void WebErr_Tick1s(void) {
  uint32_t now = (uint32_t) WEBERR_NOW();
  err_cfg = (uint8_t) ERR_N;
  uint8_t n = 0;
  for (unsigned i = 0; i < ERR_N; i++) {
    uint32_t bit = 1u << i;
    uint8_t c = cfg[i].cond_fn();
    if (test_s[i]) { c = 1; test_s[i]--; }
    if (c) {
      if (!(err_act & bit)) {
        if (on_s[i] < 255) on_s[i]++;
        if (on_s[i] >= cfg[i].delay || cfg[i].delay == 0) {
          err_act |= bit; err_una |= bit; since[i] = now; err_last[i] = now; err_tot++;
          if (err_cnt[i] < 65535u) err_cnt[i]++;
          hist_push(now, cfg[i].id, 1, 0, cfg[i].val_fn());
        }
      }
    } else {
      on_s[i] = 0;
      if (err_act & bit) {
        err_act &= ~bit; err_una &= ~bit;
        hist_push(now, cfg[i].id, 0, now - since[i], 0.0f);
      }
    }
    if (err_act & bit) n++;
  }
  err_n = n;
}

void WebErr_Ack(int id) {
  for (unsigned i = 0; i < ERR_N; i++) if (id == 0 || cfg[i].id == id) err_una &= ~(1u << i);
}
void WebErr_ClearCounters(void) { for (int i = 0; i < WEBERR_MAX; i++) { err_cnt[i] = 0; err_last[i] = 0; } err_tot = 0; }
void WebErr_ClearHistory(void)  { memset(err_hist, 0, sizeof(err_hist)); }
void WebErr_Test(int id)        { for (unsigned i = 0; i < ERR_N; i++) if (cfg[i].id == id) test_s[i] = 8; }

static size_t esc(char *o, size_t cap, const char *s) {       /* copia escapando " \ y control */
  size_t n = 0;
  for (; *s && n + 7 < cap; s++) {
    unsigned char ch = (unsigned char) *s;
    if (ch == '"' || ch == '\\') { o[n++] = '\\'; o[n++] = (char) ch; }
    else if (ch < 0x20) { o[n++] = ' '; }
    else o[n++] = (char) ch;
  }
  o[n] = '\0'; return n;
}

int WebErr_Catalog(unsigned idx, char *buf, size_t size) {
  if (idx >= ERR_N) return 0;
  char n[48], d[160], v[80], c[160];
  esc(n, sizeof n, cfg[idx].name); esc(d, sizeof d, cfg[idx].desc); esc(v, sizeof v, cfg[idx].vars); esc(c, sizeof c, cfg[idx].cond);
  int k = snprintf(buf, size, "{\"i\":%u,\"n\":\"%s\",\"s\":%u,\"t\":%u,\"d\":\"%s\",\"v\":\"%s\",\"c\":\"%s\"}",
                   (unsigned) cfg[idx].id, n, (unsigned) cfg[idx].sev, (unsigned) cfg[idx].delay, d, v, c);
  return (k < 0 || (size_t) k >= size) ? 0 : k;
}
