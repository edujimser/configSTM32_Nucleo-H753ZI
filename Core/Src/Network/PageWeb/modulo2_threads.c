#include <string.h>
#include "main.h"                 /* registros del Cortex-M7: CoreDebug, DWT */
#include "FreeRTOS.h"
#include "task.h"
#include "Network/PageWeb/Modulo2_threads.h"
#include "FreeRTOS.h"
#include "main.h"

/*******************************************************************************************/
/*                               HILOS DE FREERTOS  -  MODULO 2                            */
/*******************************************************************************************/

/* ---- Tamano de pila de cada hilo ------------------------------------------------------
 * FreeRTOS NO guarda el tamano de la pila de un hilo en ningun dato publico. Se obtiene asi:
 *   1. Tabla manual (abajo): para hilos con pila estatica (osThreadNew con stack_mem).
 *   2. "IDLE" y "Tmr Svc": CubeMX les da pila estatica de tamano conocido.
 *   3. Hilos creados normalmente (pila en el heap de FreeRTOS, heap_4): el tamano esta
 *      en la cabecera del bloque de heap, justo antes de la pila.
 *   Si nada funciona, stack_size = 0 y la web muestra solo el libre minimo.            */
/* Nombre del hilo de timers: FreeRTOS 10.3 solo lo define dentro de timers.c */
#ifndef configTIMER_SERVICE_TASK_NAME
#define configTIMER_SERVICE_TASK_NAME "Tmr Svc"
#endif

#ifndef RTOSINFO_HEAP4
#define RTOSINFO_HEAP4 1                  /* 0 si NO usas heap_4.c (o heap_5.c) */
#endif

static const struct { const char *name; uint32_t bytes; } pilas_manuales[] = {
  /* { "MiTarea", 2048 },     <- descomenta y anade los hilos con pila estatica */
  { NULL, 0 }
};

volatile RtosInfo_t rtos_info;

static TaskStatus_t ts[RTOSINFO_MAX_THREADS + 6];            /* estatico: no gasta pila */
#if (configGENERATE_RUN_TIME_STATS == 1)
static struct { TaskHandle_t h; uint32_t run; } prev[RTOSINFO_MAX_THREADS + 6];
static uint32_t prev_total = 0;
#endif

static uint8_t pct(uint32_t a, uint32_t b) {
  if (b == 0) return 0;
  uint32_t p = (uint32_t) (((uint64_t) a * 100u) / b);
  return (uint8_t) (p > 100u ? 100u : p);
}

static uint32_t tam_pila(const TaskStatus_t *t, uint8_t *src) {
  /* 1. tabla manual */
  for (int i = 0; pilas_manuales[i].name != NULL; i++)
    if (strcmp(pilas_manuales[i].name, t->pcTaskName) == 0) { *src = 2; return pilas_manuales[i].bytes; }

  /* 2. hilos de sistema con pila estatica (CubeMX: cmsis_os2.c) */
  if (strcmp(t->pcTaskName, "IDLE") == 0) { *src = 2; return configMINIMAL_STACK_SIZE * sizeof(StackType_t); }
#if (configUSE_TIMERS == 1)
  if (strcmp(t->pcTaskName, configTIMER_SERVICE_TASK_NAME) == 0) {
    *src = 2; return configTIMER_TASK_STACK_DEPTH * sizeof(StackType_t);
  }
#endif

#if RTOSINFO_HEAP4
  /* 3. cabecera del bloque de heap_4 justo antes de la pila:
   *      [ pxNextFreeBlock | xBlockSize (bit alto = ocupado) ][ ...pila... ]
   *    En un bloque ocupado pxNextFreeBlock es NULL. Si no cuadra, no se usa. */
  {
    const size_t hdr = (2u * sizeof(void *) + (portBYTE_ALIGNMENT - 1u)) & ~((size_t) portBYTE_ALIGNMENT - 1u);
    const size_t ocupado = (size_t) 1 << (sizeof(size_t) * 8u - 1u);
    const void * const *cab = (const void * const *) ((const uint8_t *) t->pxStackBase - hdr);
    size_t tam = *(const size_t *) &cab[1];
    if (cab[0] == NULL && (tam & ocupado)) {
      tam &= ~ocupado;
      if (tam > hdr + 64u && tam <= configTOTAL_HEAP_SIZE) { *src = 1; return (uint32_t) (tam - hdr); }
    }
  }
#endif
  *src = 0;
  return 0;
}

void Modulo2_Update(void) {
  volatile RtosInfo_t *r = &rtos_info;                  /* solo lo escribe esta funcion */
  uint32_t total = 0;
  UBaseType_t n = uxTaskGetSystemState(ts, RTOSINFO_MAX_THREADS + 6, &total);

  /* orden estable por numero de tarea (orden de creacion) */
  for (UBaseType_t i = 1; i < n; i++) {
    TaskStatus_t x = ts[i]; UBaseType_t j = i;
    while (j > 0 && ts[j - 1].xTaskNumber > x.xTaskNumber) { ts[j] = ts[j - 1]; j--; }
    ts[j] = x;
  }

#if (configGENERATE_RUN_TIME_STATS == 1)
  uint32_t dtotal = total - prev_total;                  /* sin signo: vale aunque de la vuelta */
#endif

  uint32_t st_tot = 0, st_uso = 0;
  uint8_t  peor = 0; int peor_i = -1;
  uint8_t  idle_pct = 0, idle_ok = 0;

  r->count = (uint8_t) uxTaskGetNumberOfTasks();
  r->shown = (uint8_t) (n < RTOSINFO_MAX_THREADS ? n : RTOSINFO_MAX_THREADS);

  for (UBaseType_t i = 0; i < n; i++) {
    const TaskStatus_t *t = &ts[i];
    uint8_t  cpu = 0, src = 0;

#if (configGENERATE_RUN_TIME_STATS == 1)
    uint32_t prun = t->ulRunTimeCounter;                 /* hilo nuevo: 0 % este segundo */
    for (UBaseType_t k = 0; k < RTOSINFO_MAX_THREADS + 6; k++)
      if (prev[k].h == t->xHandle) { prun = prev[k].run; break; }
    cpu = pct(t->ulRunTimeCounter - prun, dtotal);
    if (strcmp(t->pcTaskName, "IDLE") == 0) { idle_pct = cpu; idle_ok = 1; }
#endif

    uint32_t libre = (uint32_t) t->usStackHighWaterMark * sizeof(StackType_t);
    uint32_t tam   = tam_pila(t, &src);
    uint32_t uso   = (tam > libre) ? tam - libre : 0;
    uint8_t  p     = pct(uso, tam);

    if (tam) { st_tot += tam; st_uso += uso; if (p >= peor) { peor = p; peor_i = (int) i; } }

    if (i < RTOSINFO_MAX_THREADS) {
      volatile RtosThread_t *h = &r->th[i];
      strncpy((char *) h->name, t->pcTaskName, RTOSINFO_NAME_LEN - 1);
      h->name[RTOSINFO_NAME_LEN - 1] = '\0';
      h->state      = (uint8_t) t->eCurrentState;
      h->prio       = (uint8_t) t->uxCurrentPriority;
      h->prio_base  = (uint8_t) t->uxBasePriority;
      h->cpu        = cpu;
      h->number     = (uint32_t) t->xTaskNumber;
      h->stack_size = tam;
      h->stack_free = libre;
      h->stack_used = uso;
      h->stack_pct  = p;
      h->stack_src  = src;
    }
  }
  for (UBaseType_t i = n; i < RTOSINFO_MAX_THREADS; i++) {   /* huecos sobrantes a cero */
    memset((void *) &r->th[i], 0, sizeof(RtosThread_t));
  }

#if (configGENERATE_RUN_TIME_STATS == 1)
  for (UBaseType_t k = 0; k < RTOSINFO_MAX_THREADS + 6; k++) {
    prev[k].h   = (k < n) ? ts[k].xHandle : NULL;
    prev[k].run = (k < n) ? ts[k].ulRunTimeCounter : 0;
  }
  prev_total   = total;
  r->stats_ok  = (uint8_t) (idle_ok && dtotal);
  r->cpu_total = r->stats_ok ? (uint8_t) (100u - idle_pct) : 0;
#else
  (void) idle_pct; (void) idle_ok;
  r->stats_ok  = 0;
  r->cpu_total = 0;
#endif

  r->uptime_s    = (uint32_t) (xTaskGetTickCount() / configTICK_RATE_HZ);
  r->stack_total = st_tot;
  r->stack_used  = st_uso;
  r->stack_pct   = pct(st_uso, st_tot);
  r->tcb_bytes   = (uint32_t) r->count * (uint32_t) sizeof(StaticTask_t);   /* = tamano de un TCB */
  r->mem_total   = st_tot + r->tcb_bytes;
  r->worst_pct   = peor;
  if (peor_i >= 0) {
    strncpy((char *) r->worst_name, ts[peor_i].pcTaskName, RTOSINFO_NAME_LEN - 1);
    r->worst_name[RTOSINFO_NAME_LEN - 1] = '\0';
  } else {
    r->worst_name[0] = '\0';
  }
}

void Modulo2_RunTimeInit(void) {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  *(volatile uint32_t *) 0xE0001FB0u = 0xC5ACCE55u;     /* desbloqueo del DWT en Cortex-M7 */
  DWT->CYCCNT = 0;
  DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t Modulo2_RunTimeGet(void) { return DWT->CYCCNT >> 8; }
