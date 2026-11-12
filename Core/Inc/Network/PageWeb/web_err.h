/* web_err.h - Gestor de errores para la web: estado en vivo, historico y contadores.
 * La lista de errores esta en web_err_cfg.h. Llama a WebErr_Tick1s() una vez por segundo. */
#ifndef WEB_ERR_H
#define WEB_ERR_H
#include <stddef.h>
#include <stdint.h>

#define WEBERR_MAX   32           /* errores configurables */
#define WEBERR_HIST  20           /* eventos en el historico */

#define ERR_INFO     0
#define ERR_AVISO    1
#define ERR_ALARMA   2
#define ERR_CRITICO  3

typedef struct {
  uint32_t t;                     /* hora (segundos UNIX) del evento */
  uint32_t dur;                   /* solo RESUELTA: segundos que estuvo activo */
  float    val;                   /* valor guardado al activarse */
  uint16_t id;
  uint8_t  ev;                    /* 1 = activa, 0 = resuelta */
} WebErrHist_t;

extern volatile uint32_t err_act;           /* bit i = error i activo */
extern volatile uint32_t err_una;           /* bit i = activo y SIN RECONOCER */
extern volatile uint32_t err_tot;           /* eventos de activacion totales */
extern volatile uint8_t  err_n;             /* errores activos ahora */
extern volatile uint8_t  err_cfg;           /* errores configurados */
extern volatile uint16_t err_cnt[WEBERR_MAX];   /* veces que se ha activado cada uno */
extern volatile uint32_t err_last[WEBERR_MAX];  /* hora de la ultima activacion */
extern WebErrHist_t      err_hist[WEBERR_HIST]; /* [0] = el mas reciente */

void WebErr_Tick1s(void);                   /* evalua las condiciones (cada segundo) */
void WebErr_Ack(int id);                    /* reconoce el error con ese ID (0 = todos) */
void WebErr_ClearCounters(void);
void WebErr_ClearHistory(void);
void WebErr_Test(int id);                   /* simula el error 8 s (sin tocar la maquina) */
int  WebErr_Catalog(unsigned idx, char *buf, size_t size);   /* JSON de un error; 0 = no hay mas */

#endif
