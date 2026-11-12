/* =============================================================================
 * web_sys.c  -  Estado interno del STM32: hilos, RAM libre y carga de CPU.
 * NO probado con tu proyecto (usa FreeRTOS y CMSIS): si da error de compilacion, pegalo.
 * ============================================================================= */
#include <string.h>
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Network/PageWeb/web_sys.h"

volatile uint8_t  sys_cpu = 0;
volatile uint8_t  sys_stats_ok = 0;
volatile uint8_t  sys_heap_pct = 0;
volatile uint32_t sys_heap_tot = 0;
volatile uint32_t sys_heap_free = 0;
volatile uint32_t sys_heap_min = 0;
volatile uint32_t sys_ram_stat = 0;
volatile uint32_t sys_up = 0;
volatile uint8_t  sys_threads = 0;
volatile uint8_t  sys_ws = 0;
char              sys_th_n[WEBSYS_MAX_THREADS][16];
volatile uint8_t  sys_th_s[WEBSYS_MAX_THREADS];
volatile uint8_t  sys_th_p[WEBSYS_MAX_THREADS];
volatile uint8_t  sys_th_c[WEBSYS_MAX_THREADS];
volatile uint16_t sys_th_f[WEBSYS_MAX_THREADS];

/* Simbolos del linker script de CubeIDE (inicio/fin de .data y .bss) */
extern uint32_t _sdata, _edata, _sbss, _ebss;

#if (configGENERATE_RUN_TIME_STATS == 1) && (configUSE_TRACE_FACILITY == 1)
#define WEBSYS_HAVE_STATS 1
#endif

/* ---- contador de alta resolucion para las estadisticas (DWT, ciclos / 256) ---- */
void WebSys_RunTimeInit(void) {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  *(volatile uint32_t *) 0xE0001FB0u = 0xC5ACCE55u;     /* desbloqueo del DWT en Cortex-M7 */
  DWT->CYCCNT = 0;
  DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}
uint32_t WebSys_RunTimeGet(void) { return DWT->CYCCNT >> 8; }

void WebSys_RequestReset(void) { NVIC_SystemReset(); }

#ifdef WEBSYS_HAVE_STATS
#define MAXT WEBSYS_MAX_THREADS
static TaskStatus_t ts[MAXT + 4];
static struct { TaskHandle_t h; uint32_t run; } prev[MAXT + 4];
static uint32_t prev_total = 0;

static uint8_t estado_web(eTaskState e) {
  switch (e) {
    case eRunning:   return 0;
    case eReady:     return 1;
    case eBlocked:   return 2;
    case eSuspended: return 3;
    default:         return 4;
  }
}
#endif

void WebSys_Update(void) {
  sys_up = (uint32_t) (xTaskGetTickCount() / configTICK_RATE_HZ);

  /* ---- heap de FreeRTOS */
  sys_heap_tot  = configTOTAL_HEAP_SIZE;
  sys_heap_free = (uint32_t) xPortGetFreeHeapSize();
  sys_heap_min  = (uint32_t) xPortGetMinimumEverFreeHeapSize();
  sys_heap_pct  = sys_heap_tot ? (uint8_t) (((sys_heap_tot - sys_heap_free) * 100u) / sys_heap_tot) : 0;

  /* ---- RAM estatica aproximada */
  sys_ram_stat = (uint32_t) ((&_edata - &_sdata) + (&_ebss - &_sbss)) * 4u;

  sys_threads = (uint8_t) uxTaskGetNumberOfTasks();

#ifdef WEBSYS_HAVE_STATS
  uint32_t total = 0;
  UBaseType_t n = uxTaskGetSystemState(ts, MAXT + 4, &total);
  uint32_t dtotal = total - prev_total;                 /* aritmetica sin signo: vale aunque el contador de la vuelta */

  /* orden estable por numero de tarea */
  for (UBaseType_t i = 1; i < n; i++) {
    TaskStatus_t x = ts[i]; UBaseType_t j = i;
    while (j > 0 && ts[j - 1].xTaskNumber > x.xTaskNumber) { ts[j] = ts[j - 1]; j--; }
    ts[j] = x;
  }

  uint32_t idle_pct = 0; int idle_found = 0;
  for (UBaseType_t i = 0; i < n; i++) {
    uint32_t prun = 0;
    for (UBaseType_t k = 0; k < MAXT + 4; k++) if (prev[k].h == ts[i].xHandle) { prun = prev[k].run; break; }
    uint32_t pct = dtotal ? (uint32_t) (((uint64_t) (ts[i].ulRunTimeCounter - prun) * 100u) / dtotal) : 0;
    if (pct > 100) pct = 100;
    if (strcmp(ts[i].pcTaskName, "IDLE") == 0) { idle_pct = pct; idle_found = 1; }

    if (i < MAXT) {
      strncpy(sys_th_n[i], ts[i].pcTaskName, sizeof(sys_th_n[i]) - 1);
      sys_th_n[i][sizeof(sys_th_n[i]) - 1] = '\0';
      sys_th_s[i] = estado_web(ts[i].eCurrentState);
      sys_th_p[i] = (uint8_t) ts[i].uxCurrentPriority;
      sys_th_c[i] = (uint8_t) pct;
      uint32_t bytes = (uint32_t) ts[i].usStackHighWaterMark * sizeof(StackType_t);
      sys_th_f[i] = bytes > 65535u ? 65535u : (uint16_t) bytes;
    }
  }
  for (UBaseType_t i = n; i < MAXT; i++) { sys_th_n[i][0] = '\0'; sys_th_s[i] = 0; sys_th_p[i] = 0; sys_th_c[i] = 0; sys_th_f[i] = 0; }

  for (UBaseType_t k = 0; k < MAXT + 4; k++) { prev[k].h = (k < n) ? ts[k].xHandle : NULL; prev[k].run = (k < n) ? ts[k].ulRunTimeCounter : 0; }
  prev_total = total;

  if (idle_found && dtotal) { sys_cpu = (uint8_t) (100u - idle_pct); sys_stats_ok = 1; }
#else
  sys_stats_ok = 0;
  sys_cpu = 0;
#endif
}
