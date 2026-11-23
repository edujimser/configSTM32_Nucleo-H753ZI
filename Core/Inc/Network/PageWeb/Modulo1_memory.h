
#ifndef WEB_MODULO1_MEMORY_H
#define WEB_MODULO1_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>           
#include "FreeRTOS.h"         

/*******************************************************************************************/
/*                                  RAM MICROPROCESADOR MODULO 1                           */
/*******************************************************************************************/

/* ---- Bloque de RAM fisico (nombre, direccion, tamano) ----
 * El typedef tiene que ir ANTES de usarlo en el extern de abajo */
typedef struct {
  const char *name;
  uint32_t    base;
  uint32_t    size;
} RamRegion_t;

#define RAMINFO_NUM_REGIONS  8
extern const RamRegion_t ram_regions[RAMINFO_NUM_REGIONS];

/* ---- Resultados (se actualizan en Modulo1_Memory) ----
 * volatile: los escribe el hilo de red y los puede leer otra tarea / la web */
typedef struct {
  /* bloque de RAM donde el linker puso .data/.bss */
  const char *main_region;       /* nombre del bloque (ej. "AXI SRAM (D1)") */
  char        region_txt[16];    /* el mismo nombre copiado en un array: para la web (WV_STR) */
  uint32_t    main_size;         /* tamano del bloque                        */

  /* 2. variables globales / static */
  uint32_t data;                 /* .data: globales con valor inicial         */
  uint32_t bss;                  /* .bss : globales a 0 (incluye heap FreeRTOS)*/
  uint32_t statics;              /* data + bss                                */

  /* 3. heap de FreeRTOS */
  uint32_t rtos_total;           /* configTOTAL_HEAP_SIZE                     */
  uint32_t rtos_free;            /* libre ahora                               */
  uint32_t rtos_min_free;        /* minimo libre desde el arranque (peor caso)*/
  uint32_t rtos_largest;         /* bloque libre mas grande (fragmentacion)   */
  uint32_t rtos_allocs;          /* pvPortMalloc correctos                    */
  uint32_t rtos_frees;           /* vPortFree correctos                       */
  uint8_t  rtos_pct;             /* % usado                                   */

  /* 4. heap de la libreria C (malloc) */
  uint32_t libc_arena;           /* RAM pedida al sistema (sbrk)              */
  uint32_t libc_used;            /* en uso por malloc                         */
  uint32_t libc_free;            /* libre dentro de la arena                  */

  /* 5. pila principal (MSP) */
  uint32_t stack_reserved;       /* _Min_Stack_Size del .ld                   */
  uint32_t stack_now;            /* uso en este momento                       */
  uint32_t stack_max;            /* uso maximo historico (0 = no pintada)     */
  uint8_t  stack_pct;            /* stack_max / reservado en %                */

  /* 6. resumen del bloque principal */
  uint32_t main_used;            /* statics + heap libc + pila reservada      */
  uint32_t main_free;            /* lo que queda sin usar en ese bloque       */
  uint8_t  main_pct;             /* % usado del bloque                        */

  /* total del micro */
  uint32_t ram_total;            /* suma de todos los bloques (sin ITCM/backup)*/
} RamInfo_t;

extern volatile RamInfo_t ram_info;

void   RamInfo_PaintMainStack(void);                /* llamar al principio de main() */
void   Modulo1_Memory(void);                        /* recalcula ram_info            */

#endif