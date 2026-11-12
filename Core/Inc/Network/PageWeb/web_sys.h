/* =============================================================================
 * web_sys.h  -  Estado interno del STM32 para la web: hilos, RAM, CPU
 *
 * WebSys_Update() se llama una vez por segundo desde el hilo de red (network.c).
 * Rellena las variables sys_* que estan registradas en web_vars.c (pagina interno).
 *
 * La carga de CPU (total y por hilo) necesita en FreeRTOS:
 *     configUSE_TRACE_FACILITY             = 1
 *     configGENERATE_RUN_TIME_STATS        = 1   (y los dos macros del LEEME)
 *     INCLUDE_uxTaskGetStackHighWaterMark  = 1
 * Sin ellos funciona igual pero sin CPU (sys_stats_ok = 0) y sin lista de hilos.
 * ============================================================================= */
#ifndef WEB_SYS_H
#define WEB_SYS_H

#include <stdint.h>

#define WEBSYS_MAX_THREADS  12

extern volatile uint8_t  sys_cpu;           /* carga total de CPU, 0..100 % */
extern volatile uint8_t  sys_stats_ok;      /* 1 si hay estadisticas de CPU */
extern volatile uint8_t  sys_heap_pct;      /* % del heap de FreeRTOS en uso */
extern volatile uint32_t sys_heap_tot;      /* bytes */
extern volatile uint32_t sys_heap_free;
extern volatile uint32_t sys_heap_min;      /* minimo libre desde el arranque */
extern volatile uint32_t sys_ram_stat;      /* RAM estatica (.data + .bss), aproximada */
extern volatile uint32_t sys_up;            /* segundos desde el arranque */
extern volatile uint8_t  sys_threads;       /* numero de hilos */
extern volatile uint8_t  sys_ws;            /* conexiones WebSocket abiertas (la rellena network.c) */
extern char              sys_th_n[WEBSYS_MAX_THREADS][16];   /* nombre */
extern volatile uint8_t  sys_th_s[WEBSYS_MAX_THREADS];       /* estado: 0 ejecutando 1 listo 2 bloqueado 3 suspendido 4 eliminado */
extern volatile uint8_t  sys_th_p[WEBSYS_MAX_THREADS];       /* prioridad */
extern volatile uint8_t  sys_th_c[WEBSYS_MAX_THREADS];       /* % CPU */
extern volatile uint16_t sys_th_f[WEBSYS_MAX_THREADS];       /* stack libre minimo, bytes */

void WebSys_Update(void);                   /* 1 vez por segundo, desde el hilo de red */
void WebSys_RequestReset(void);             /* reinicia el microcontrolador */

/* Contador para las estadisticas de CPU de FreeRTOS (ver LEEME) */
void     WebSys_RunTimeInit(void);
uint32_t WebSys_RunTimeGet(void);

#endif
