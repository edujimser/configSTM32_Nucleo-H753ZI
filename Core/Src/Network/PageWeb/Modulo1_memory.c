
#include <string.h>
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Network/PageWeb/Modulo1_memory.h"

/*******************************************************************************************/
/*                                  RAM MICROPROCESADOR MODULO 1                           */
/*******************************************************************************************/

  const RamRegion_t ram_regions[RAMINFO_NUM_REGIONS] = {
    { "ITCM",          0x00000000u,  64u * 1024u },   /* solo CPU, codigo/datos rapidos   */
    { "DTCM",          0x20000000u, 128u * 1024u },   /* solo CPU, muy rapida, sin DMA    */
    { "AXI SRAM (D1)", 0x24000000u, 512u * 1024u },   /* principal                        */
    { "SRAM1 (D2)",    0x30000000u, 128u * 1024u },   /* DMA, Ethernet                    */
    { "SRAM2 (D2)",    0x30020000u, 128u * 1024u },
    { "SRAM3 (D2)",    0x30040000u,  32u * 1024u },
    { "SRAM4 (D3)",    0x38000000u,  64u * 1024u },   /* dominio de bajo consumo          */
    { "Backup SRAM",   0x38800000u,   4u * 1024u },   /* se mantiene con bateria (VBAT)   */
  };




/* 1 = usar vPortGetHeapStats (heap_4 / heap_5, FreeRTOS >= 10.2.1: el de CubeMX vale)
 * 0 = solo xPortGetFreeHeapSize / xPortGetMinimumEverFreeHeapSize                  */
#ifndef RAMINFO_HEAPSTATS
#define RAMINFO_HEAPSTATS 1
#endif

/* ---- Simbolos que define el script del linker (.ld) de STM32CubeIDE ----
 * No son variables: lo que importa es su DIRECCION (&_sdata...).
 * _Min_Stack_Size es un numero (0x400...) guardado como "direccion". */
extern uint32_t _sdata, _edata;        /* .data  */
extern uint32_t _sbss,  _ebss;         /* .bss   */
extern uint8_t  _end;                  /* fin de .bss = inicio del heap de malloc */
extern uint8_t  _estack;               /* tope de la pila principal               */
extern uint8_t  _Min_Stack_Size;       /* pila reservada en el .ld                */


volatile RamInfo_t ram_info;

/* =============================================================================
 * Recalcular todo (cada segundo, por ejemplo)
 * ============================================================================= */
volatile RamInfo_t ram_info;

/* ---- utilidades ---- */
static inline uint32_t leer_msp(void) {               /* puntero de pila principal (MSP) */
  uint32_t r;
  __asm volatile ("MRS %0, msp" : "=r" (r));
  return r;
}

static const RamRegion_t *region_de(uint32_t addr) {  /* en que bloque esta una direccion */
  for (int i = 0; i < RAMINFO_NUM_REGIONS; i++)
    if (addr >= ram_regions[i].base && addr - ram_regions[i].base < ram_regions[i].size)
      return &ram_regions[i];
  return NULL;
}

static uint8_t pct(uint32_t usado, uint32_t total) {
  return total ? (uint8_t) (((uint64_t) usado * 100u) / total) : 0;
}

/* =============================================================================
 * Pintado de la pila principal: se rellena la zona libre con un patron.
 * Mas tarde, lo que ya no tenga el patron es lo que la pila ha llegado a usar.
 * Llamar UNA vez, lo primero de main() (antes de HAL_Init y del scheduler).
 * ============================================================================= */
#define PATRON  0xA5A5A5A5u
static uint32_t *pila_lo = NULL;                       /* inicio de la zona pintada */

void __attribute__((noinline)) RamInfo_PaintMainStack(void) {
  uint32_t lo = (uint32_t) &_estack - (uint32_t) &_Min_Stack_Size;   /* fondo de la pila reservada */
  uint32_t hi = leer_msp() - 64u;                                    /* margen bajo el SP actual   */
  lo = (lo + 3u) & ~3u;
  if (hi <= lo) return;
  for (volatile uint32_t *p = (uint32_t *) lo; p < (uint32_t *) hi; p++) *p = PATRON;
  pila_lo = (uint32_t *) lo;
}

static uint32_t pila_maxima(void) {
  if (pila_lo == NULL) return 0;                       /* no se llamo a RamInfo_PaintMainStack */
  uint32_t *p = pila_lo;
  uint32_t *tope = (uint32_t *) &_estack;
  while (p < tope && *p == PATRON) p++;                /* primer hueco "pisado" desde abajo    */
  return (uint32_t) &_estack - (uint32_t) p;           /* si p == pila_lo: llego al fondo (!)  */
}

/* =============================================================================
 * Recalcular todo (cada segundo, por ejemplo)
 * ============================================================================= */
void Modulo1_Memory(void) {
  RamInfo_t r;
  memset(&r, 0, sizeof(r));

  /* ---- total del micro (sin la Backup SRAM) */
  for (int i = 0; i < RAMINFO_NUM_REGIONS - 1; i++) r.ram_total += ram_regions[i].size;

  /* ---- 2. variables globales / static */
  r.data    = (uint32_t) ((uint8_t *) &_edata - (uint8_t *) &_sdata);
  r.bss     = (uint32_t) ((uint8_t *) &_ebss  - (uint8_t *) &_sbss);
  r.statics = r.data + r.bss;

  /* ---- 3. heap de FreeRTOS (es un array global: ya esta contado dentro de .bss) */
  r.rtos_total = configTOTAL_HEAP_SIZE;
#if RAMINFO_HEAPSTATS
  HeapStats_t hs;
  vPortGetHeapStats(&hs);
  r.rtos_free     = (uint32_t) hs.xAvailableHeapSpaceInBytes;
  r.rtos_min_free = (uint32_t) hs.xMinimumEverFreeBytesRemaining;
  r.rtos_largest  = (uint32_t) hs.xSizeOfLargestFreeBlockInBytes;
  r.rtos_allocs   = (uint32_t) hs.xNumberOfSuccessfulAllocations;
  r.rtos_frees    = (uint32_t) hs.xNumberOfSuccessfulFrees;
#else
  r.rtos_free     = (uint32_t) xPortGetFreeHeapSize();
  r.rtos_min_free = (uint32_t) xPortGetMinimumEverFreeHeapSize();
#endif
  r.rtos_pct = pct(r.rtos_total - r.rtos_free, r.rtos_total);

  /* ---- 4. heap de la libreria C (crece desde _end hacia arriba) */
  struct mallinfo mi = mallinfo();
  r.libc_arena = (uint32_t) mi.arena;
  r.libc_used  = (uint32_t) mi.uordblks;
  r.libc_free  = (uint32_t) mi.fordblks;

  /* ---- 5. pila principal (crece desde _estack hacia abajo) */
  r.stack_reserved = (uint32_t) &_Min_Stack_Size;
  r.stack_now      = (uint32_t) &_estack - leer_msp();
  r.stack_max      = pila_maxima();
  r.stack_pct      = pct(r.stack_max, r.stack_reserved);

  /* ---- 6. bloque principal: [ .data | .bss | heap malloc | ...libre... | pila ] */
  const RamRegion_t *rg = region_de((uint32_t) &_sdata);
  const RamRegion_t *rs = region_de((uint32_t) &_estack - 4u);
  if (rg != NULL) {
    uint32_t fin_heap = (uint32_t) &_end + r.libc_arena;          /* hasta donde llega malloc */
    uint32_t limite   = rg->base + rg->size;                      /* final del bloque         */
    if (rs == rg) limite = (uint32_t) &_estack - r.stack_reserved; /* la pila esta arriba     */
    r.main_region = rg->name;
    strncpy(r.region_txt, rg->name, sizeof(r.region_txt) - 1);   /* r ya esta a 0: queda terminado */
    r.main_size   = rg->size;
    r.main_free   = (limite > fin_heap) ? limite - fin_heap : 0;
    r.main_used   = rg->size - r.main_free;
    r.main_pct    = pct(r.main_used, rg->size);
  }

  /* copiar de una vez al resultado visible */
  memcpy((void *) &ram_info, &r, sizeof(r));
}

