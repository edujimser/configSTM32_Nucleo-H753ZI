#ifndef WEB_MODULO2_THREADS_H
#define WEB_MODULO2_THREADS_H

#include <stdint.h>
#include <stddef.h>

/*******************************************************************************************/
/*                               HILOS DE FREERTOS  -  MODULO 2                            */
/*******************************************************************************************
 *  Que mide, por cada hilo (tarea) de FreeRTOS:
 *    - nombre, estado, prioridad actual y base
 *    - % de CPU del ultimo segundo           (necesita configGENERATE_RUN_TIME_STATS = 1)
 *    - pila: tamano total, maximo usado, minimo libre y % usado
 *  Y en total: numero de hilos, CPU, pila reservada/usada, memoria de los TCB,
 *  y el hilo con la pila mas llena.
 *
 *  Necesita en FreeRTOSConfig.h:  configUSE_TRACE_FACILITY = 1   (ya lo tienes)
 *
 *  Uso:   cada segundo, desde UNA tarea (p.ej. el hilo de red):  RtosInfo_Update();
 *         NUNCA desde una interrupcion.
 *******************************************************************************************/

/* Hilos que se guardan (si hay mas, se cuentan en 'count' pero no se detallan).
 * Cada hilo son 7 variables en la tabla web: no lo subas sin mirar WEBVARS_JSON_MAX. */
#ifndef RTOSINFO_MAX_THREADS
#define RTOSINFO_MAX_THREADS  10
#endif

#define RTOSINFO_NAME_LEN     16          /* = configMAX_TASK_NAME_LEN de CubeMX */

/* Estado (mismo numero que eTaskState de FreeRTOS) */
#define HILO_EJECUTANDO   0               /* eRunning   : es el que usa la CPU ahora    */
#define HILO_LISTO        1               /* eReady     : quiere CPU, espera turno      */
#define HILO_BLOQUEADO    2               /* eBlocked   : esperando (delay, cola, ...)  */
#define HILO_SUSPENDIDO   3               /* eSuspended : parado con vTaskSuspend       */
#define HILO_BORRADO      4               /* eDeleted   : borrado, falta liberar memoria*/

typedef struct {
  char     name[RTOSINFO_NAME_LEN];       /* nombre ("" = hueco libre en la tabla)       */
  uint8_t  state;                         /* HILO_xxx                                     */
  uint8_t  prio;                          /* prioridad actual (puede subir por un mutex)  */
  uint8_t  prio_base;                     /* prioridad con la que se creo                 */
  uint8_t  cpu;                           /* % de CPU en el ultimo segundo                */
  uint32_t number;                        /* numero de tarea (orden de creacion)          */
  uint32_t stack_size;                    /* pila total en bytes (0 = no se pudo saber)   */
  uint32_t stack_free;                    /* minimo libre desde que se creo (bytes)       */
  uint32_t stack_used;                    /* maximo usado = size - free (0 si size = 0)   */
  uint8_t  stack_pct;                     /* % de pila usado como maximo                  */
  uint8_t  stack_src;                     /* de donde sale el tamano: 0 ?, 1 heap, 2 fijo */
} RtosThread_t;

typedef struct {
  uint8_t  count;                         /* hilos que existen ahora                      */
  uint8_t  shown;                         /* hilos guardados en th[] (<= MAX)             */
  uint8_t  stats_ok;                      /* 1 = hay estadisticas de CPU                  */
  uint8_t  cpu_total;                     /* % CPU total (100 - IDLE)                     */
  uint32_t uptime_s;                      /* segundos desde el arranque                   */

  uint32_t stack_total;                   /* suma de pilas conocidas (bytes)              */
  uint32_t stack_used;                    /* suma de maximos usados (bytes)               */
  uint8_t  stack_pct;                     /* stack_used / stack_total                     */
  uint32_t tcb_bytes;                     /* memoria de los bloques de control (TCB)      */
  uint32_t mem_total;                     /* pilas + TCB: lo que ocupan los hilos en RAM  */

  char     worst_name[RTOSINFO_NAME_LEN]; /* hilo con la pila mas llena                   */
  uint8_t  worst_pct;                     /* su % de pila usado                           */

  RtosThread_t th[RTOSINFO_MAX_THREADS];  /* ordenados por numero de tarea                */
} RtosInfo_t;

extern volatile RtosInfo_t rtos_info;

void Modulo2_Update(void);               /* recalcula rtos_info (1 vez por segundo)      */
void Modulo2_RunTimeInit(void);          /* inicializa el contador de ciclos del DWT     */
uint32_t Modulo2_RunTimeGet(void);       /* devuelve el contador de ciclos del DWT       */
#endif