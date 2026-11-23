#ifndef WEB_MODULO3_CLOCKS_H
#define WEB_MODULO3_CLOCKS_H

#include <stdint.h>

/*******************************************************************************************/
/*                         RELOJES Y TEMPERATURA DEL MICRO  -  MODULO 3                    */
/*******************************************************************************************
 *  Que mide:
 *    - Frecuencias reales: SYSCLK, CPU, AHB, APB1..APB4, PLL1/2/3 (salidas P, Q, R)
 *    - De donde sale el reloj del sistema y que osciladores estan funcionando
 *    - Escala de tension del nucleo (VOS), esperas de la flash, revision del chip
 *    - Temperatura interna del chip, Vref+ (alimentacion analogica) y bateria del RTC (VBAT)
 *      -> SOLO si el ADC3 esta activado en CubeMX (si no, adc_ok = 0 y lo demas funciona)
 *
 *  Uso:   cada segundo, desde UNA tarea (p.ej. el hilo de red):  Modulo3_Clocks();
 *******************************************************************************************/

/* Origen del reloj del sistema (RCC_CFGR.SWS) */
#define RELOJ_HSI    0
#define RELOJ_CSI    1
#define RELOJ_HSE    2
#define RELOJ_PLL1   3

/* Bits de osc_on: que osciladores / PLL estan funcionando */
#define OSC_HSI      (1u << 0)
#define OSC_CSI      (1u << 1)
#define OSC_HSE      (1u << 2)
#define OSC_HSI48    (1u << 3)
#define OSC_LSI      (1u << 4)
#define OSC_LSE      (1u << 5)
#define OSC_PLL1     (1u << 6)
#define OSC_PLL2     (1u << 7)
#define OSC_PLL3     (1u << 8)

typedef struct {
  /* --- frecuencias (Hz) --- */
  uint32_t sysclk;                 /* reloj del sistema (antes de divisores)     */
  uint32_t cpu;                    /* reloj del Cortex-M7 (SystemCoreClock)      */
  uint32_t hclk;                   /* bus AHB (memorias, DMA)                    */
  uint32_t apb1, apb2, apb3, apb4; /* buses de perifericos                       */
  uint32_t pll1_p, pll1_q, pll1_r;
  uint32_t pll2_p, pll2_q, pll2_r;
  uint32_t pll3_p, pll3_q, pll3_r;
  uint32_t hse_hz;                 /* cristal externo (HSE_VALUE)                */
  uint32_t hsi_hz;                 /* oscilador interno (64 MHz / divisor)       */

  /* --- estado --- */
  uint8_t  sys_src;                /* RELOJ_xxx                                  */
  uint8_t  pll_src;                /* entrada de los PLL: 0 HSI, 1 CSI, 2 HSE, 3 ninguna */
  uint16_t osc_on;                 /* bits OSC_xxx                               */
  uint8_t  vos;                    /* escala de tension: 0 (maxima) .. 3         */
  uint8_t  flash_ws;               /* esperas de la flash (wait states)          */
  uint16_t dev_id;                 /* 0x450 = STM32H74x/H75x                     */
  uint16_t rev_id;                 /* 0x1003 = rev Y, 0x2003 = rev V             */
  char     rev_txt[4];             /* "Y", "V", "?"                              */

  /* --- analogico (ADC3) --- */
  uint8_t  adc_ok;                 /* 1 = ADC3 configurado y midiendo            */
  float    temp_c;                 /* temperatura del chip (grados C)            */
  float    temp_min, temp_max;     /* minima y maxima desde el arranque          */
  uint16_t vref_mv;                /* Vref+ real, calculada con VREFINT (mV)     */
  uint16_t vbat_mv;                /* pila del RTC (mV)                          */
} ClockInfo_t;

extern volatile ClockInfo_t clock_info;

void Modulo3_Clocks(void);       /* recalcula clock_info (1 vez por segundo)   */

#endif