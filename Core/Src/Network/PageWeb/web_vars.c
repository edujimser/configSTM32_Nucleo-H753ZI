/* =============================================================================
 * web_vars.c  -  AQUI ES DONDE EDITAS (secciones 1 a 6). Lo demas no se toca.
 * ============================================================================= */
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "Network/PageWeb/web_vars.h"
#include "Network/PageWeb/web_demo.h"     /* variables de DEMOSTRACION (borrar al usar las tuyas) */
#include "Network/PageWeb/web_sys.h"      /* estado interno del STM32: hilos, RAM, CPU */
#include "Network/PageWeb/web_bus.h"      /* visor/forzado de tramas CAN y TRDP */
#include "Network/PageWeb/web_sig.h"      /* señales de comunicacion con forzado individual */
#include "Network/PageWeb/web_err.h"      /* errores: estado, historico, contadores */
#include "Network/PageWeb/web_stm32.h"    /* pagina de datos de control de stm32 */
#include "Network/PageWeb/Modulo1_memory.h"
#include "Network/PageWeb/modulo2_threads.h"
#include "Network/PageWeb/modulo3_clocks.h"
#include "Network/PageWeb/modulo4_gpio.h"

void WebTerm_OnLine(const char *line);   /* la defines tu en la seccion 5 */

#ifndef WEBVARS_HOST_TEST               /* en el PC (pruebas) no hay RTOS */
#include "cmsis_os2.h"
#define WEBVARS_USE_RTOS 1
#endif


/* ===========================================================================
 * 1) PAGINAS: nombre de la ruta WebSocket -> bit de pagina.
 *    "sistema" significa que sistema.html abre  ws://placa/ws/sistema
 * =========================================================================== */
static const struct { const char *name; uint16_t pg; } web_pages[] = {
  { "datos",     PG_DATOS     },
  { "config",    PG_CONFIG    },
  { "sistema",   PG_SISTEMA   },
  { "widgets",   PG_WIDGETS   },
  { "industria", PG_INDUSTRIA },
  { "interno",   PG_INTERNO   },
  { "hvac",      PG_HVAC      },
  { "compon",    PG_COMPON    },
  { "objeto",    PG_OBJETO    },
  { "wdatos",    PG_WDATOS    },
  { "can",       PG_CAN       },
  { "trdp",      PG_TRDP      },
  { "errores",   PG_ERR       },
  { "stm32",     PG_STM32     },
};

/* ===========================================================================
 * 2) VARIABLES EXTERN
 * =========================================================================== */

/* ===========================================================================
 * 3) TABLA DE VARIABLES.
 * =========================================================================== */


#define R_WEB    0U
#define RW_WEB   1U
#define NO_VALUE 0


#define DATAEGETWEB(key, pages, typeVar, decimals, ptrVar) \
		  { (key), (pages), (typeVar), (decimals), (const volatile void *) &(ptrVar), R_WEB, NO_VALUE, NO_VALUE }

#define DATASETWEB(key, pages, typeVar, decimals, ptrVar, min, max) \
		  { (key), (pages), (typeVar), (decimals), (const volatile void *) &(ptrVar), RW_WEB, (min), (max) }

/* =============================================================================================
 * TABLA DE VARIABLES WEB
 *
 * COLUMNAS
 *   MACRO     DATAEGETWEB = GET, la web SOLO LEE la variable (rw = 0, sin min/max)
 *             DATASETWEB  = SET, la web LEE y ESCRIBE la variable (rw = 1, limitada a MIN..MAX)
 *   ID        nombre de la variable en el JSON y en el HTML  (data-v="ID", data-set="ID", ...)
 *             no se debe repetir en una misma pagina
 *   PAGINAS   paginas que reciben la variable (PG_xxx de web_vars.h, varias unidas con | )
 *   TIPO      tipo C de la variable:  WV_U8 uint8_t | WV_U16 uint16_t | WV_U32 uint32_t | WV_I32 int32_t
 *                                     WV_FLOAT float | WV_BOOL uint8_t 0/1 | WV_STR char[N]
 *   DEC       decimales con los que se envia un WV_FLOAT (0 a 6); 0 en enteros y booleanos
 *             en WV_STR es el TAMANO del array de char
 *   VARIABLE  la variable global del firmware (se guarda su direccion, se lee en cada envio)
 *   MIN, MAX  (solo SET) rango permitido al escribir desde la web; si MIN == MAX no se limita
 *             (aunque siempre se respeta el rango del tipo: un WV_U8 nunca pasa de 0..255)
 *
 * ORDEN
 *   1. GET  1.1 una sola pagina (por pagina)   1.2 varias paginas (por la primera pagina)
 *   2. SET  2.1 una sola pagina (por pagina)   2.2 varias paginas (por la primera pagina)
 * ============================================================================================= */
static const WebVar_t web_vars[] = {

  /* #############################################################################################
   * MODULO 1  - RAM DEL MICROPROCESADOR
   * ############################################################################################# */
   //MACRO     ID              PAGINAS     TIPO      DEC VARIABLE
 
    /* --- bloque principal --- */
    DATAEGETWEB("ram_reg",      PG_STM32, WV_STR,   16, ram_info.region_txt),
    DATAEGETWEB("ram_regsz",    PG_STM32, WV_U32,   0,  ram_info.main_size),
    DATAEGETWEB("ram_usado",    PG_STM32, WV_U32,   0,  ram_info.main_used),
    DATAEGETWEB("ram_libre",    PG_STM32, WV_U32,   0,  ram_info.main_free),
    DATAEGETWEB("ram_pct",      PG_STM32, WV_U8,    0,  ram_info.main_pct),
    DATAEGETWEB("ram_total",    PG_STM32, WV_U32,   0,  ram_info.ram_total),
  
    /* --- variables globales --- */
    DATAEGETWEB("ram_data",     PG_STM32, WV_U32,   0,  ram_info.data),
    DATAEGETWEB("ram_bss",      PG_STM32, WV_U32,   0,  ram_info.bss),
    DATAEGETWEB("ram_stat",     PG_STM32, WV_U32,   0,  ram_info.statics),
  
    /* --- heap de FreeRTOS --- */
    DATAEGETWEB("ram_rt_tot",   PG_STM32, WV_U32,   0,  ram_info.rtos_total),
    DATAEGETWEB("ram_rt_lib",   PG_STM32, WV_U32,   0,  ram_info.rtos_free),
    DATAEGETWEB("ram_rt_min",   PG_STM32, WV_U32,   0,  ram_info.rtos_min_free),
    DATAEGETWEB("ram_rt_may",   PG_STM32, WV_U32,   0,  ram_info.rtos_largest),
    DATAEGETWEB("ram_rt_alloc", PG_STM32, WV_U32,   0,  ram_info.rtos_allocs),
    DATAEGETWEB("ram_rt_free",  PG_STM32, WV_U32,   0,  ram_info.rtos_frees),
    DATAEGETWEB("ram_rt_pct",   PG_STM32, WV_U8,    0,  ram_info.rtos_pct),
  
    /* --- heap de malloc (libc) --- */
    DATAEGETWEB("ram_lc_arena", PG_STM32, WV_U32,   0,  ram_info.libc_arena),
    DATAEGETWEB("ram_lc_uso",   PG_STM32, WV_U32,   0,  ram_info.libc_used),
    DATAEGETWEB("ram_lc_lib",   PG_STM32, WV_U32,   0,  ram_info.libc_free),
  
    /* --- pila principal (MSP) --- */
    DATAEGETWEB("ram_pi_res",   PG_STM32, WV_U32,   0,  ram_info.stack_reserved),
    DATAEGETWEB("ram_pi_act",   PG_STM32, WV_U32,   0,  ram_info.stack_now),
    DATAEGETWEB("ram_pi_max",   PG_STM32, WV_U32,   0,  ram_info.stack_max),
    DATAEGETWEB("ram_pi_pct",   PG_STM32, WV_U8,    0,  ram_info.stack_pct),

  /* #############################################################################################
   * MODULO 2  - TREHADS DE FREERTOS
   * ############################################################################################# */
	//MACRO     ID           PAGINAS     TIPO      DEC VARIABLE

	/* --- resumen --- */
	DATAEGETWEB("hl_n",     PG_STM32, WV_U8,   0,  rtos_info.count),                /* hilos que existen */
	DATAEGETWEB("hl_vis",   PG_STM32, WV_U8,   0,  rtos_info.shown),                /* hilos detallados (<= MAX) */
	DATAEGETWEB("hl_ok",    PG_STM32, WV_BOOL, 0,  rtos_info.stats_ok),             /* 1 = hay estadisticas de CPU */
	DATAEGETWEB("hl_cpu",   PG_STM32, WV_U8,   0,  rtos_info.cpu_total),            /* % CPU total */
	DATAEGETWEB("hl_up",    PG_STM32, WV_U32,  0,  rtos_info.uptime_s),             /* segundos desde el arranque */
	DATAEGETWEB("hl_stot",  PG_STM32, WV_U32,  0,  rtos_info.stack_total),          /* pila reservada (suma) */
	DATAEGETWEB("hl_suso",  PG_STM32, WV_U32,  0,  rtos_info.stack_used),           /* pila usada (suma de maximos) */
	DATAEGETWEB("hl_spct",  PG_STM32, WV_U8,   0,  rtos_info.stack_pct),            /* % de pila usado en total */
	DATAEGETWEB("hl_tcb",   PG_STM32, WV_U32,  0,  rtos_info.tcb_bytes),            /* memoria de los TCB */
	DATAEGETWEB("hl_mem",   PG_STM32, WV_U32,  0,  rtos_info.mem_total),            /* pilas + TCB */
	DATAEGETWEB("hl_peor",  PG_STM32, WV_STR,  16, rtos_info.worst_name),           /* hilo con la pila mas llena */
	DATAEGETWEB("hl_peorp", PG_STM32, WV_U8,   0,  rtos_info.worst_pct),            /* su % de pila */

	/* --- hilo 0 --- */
	DATAEGETWEB("h0_n",     PG_STM32, WV_STR,  16, rtos_info.th[0].name),
	DATAEGETWEB("h0_s",     PG_STM32, WV_U8,   0,  rtos_info.th[0].state),
	DATAEGETWEB("h0_p",     PG_STM32, WV_U8,   0,  rtos_info.th[0].prio),
	DATAEGETWEB("h0_c",     PG_STM32, WV_U8,   0,  rtos_info.th[0].cpu),
	DATAEGETWEB("h0_sz",    PG_STM32, WV_U32,  0,  rtos_info.th[0].stack_size),
	DATAEGETWEB("h0_lib",   PG_STM32, WV_U32,  0,  rtos_info.th[0].stack_free),
	DATAEGETWEB("h0_pct",   PG_STM32, WV_U8,   0,  rtos_info.th[0].stack_pct),

	/* --- hilo 1 --- */
	DATAEGETWEB("h1_n",     PG_STM32, WV_STR,  16, rtos_info.th[1].name),
	DATAEGETWEB("h1_s",     PG_STM32, WV_U8,   0,  rtos_info.th[1].state),
	DATAEGETWEB("h1_p",     PG_STM32, WV_U8,   0,  rtos_info.th[1].prio),
	DATAEGETWEB("h1_c",     PG_STM32, WV_U8,   0,  rtos_info.th[1].cpu),
	DATAEGETWEB("h1_sz",    PG_STM32, WV_U32,  0,  rtos_info.th[1].stack_size),
	DATAEGETWEB("h1_lib",   PG_STM32, WV_U32,  0,  rtos_info.th[1].stack_free),
	DATAEGETWEB("h1_pct",   PG_STM32, WV_U8,   0,  rtos_info.th[1].stack_pct),

	/* --- hilo 2 --- */
	DATAEGETWEB("h2_n",     PG_STM32, WV_STR,  16, rtos_info.th[2].name),
	DATAEGETWEB("h2_s",     PG_STM32, WV_U8,   0,  rtos_info.th[2].state),
	DATAEGETWEB("h2_p",     PG_STM32, WV_U8,   0,  rtos_info.th[2].prio),
	DATAEGETWEB("h2_c",     PG_STM32, WV_U8,   0,  rtos_info.th[2].cpu),
	DATAEGETWEB("h2_sz",    PG_STM32, WV_U32,  0,  rtos_info.th[2].stack_size),
	DATAEGETWEB("h2_lib",   PG_STM32, WV_U32,  0,  rtos_info.th[2].stack_free),
	DATAEGETWEB("h2_pct",   PG_STM32, WV_U8,   0,  rtos_info.th[2].stack_pct),

	/* --- hilo 3 --- */
	DATAEGETWEB("h3_n",     PG_STM32, WV_STR,  16, rtos_info.th[3].name),
	DATAEGETWEB("h3_s",     PG_STM32, WV_U8,   0,  rtos_info.th[3].state),
	DATAEGETWEB("h3_p",     PG_STM32, WV_U8,   0,  rtos_info.th[3].prio),
	DATAEGETWEB("h3_c",     PG_STM32, WV_U8,   0,  rtos_info.th[3].cpu),
	DATAEGETWEB("h3_sz",    PG_STM32, WV_U32,  0,  rtos_info.th[3].stack_size),
	DATAEGETWEB("h3_lib",   PG_STM32, WV_U32,  0,  rtos_info.th[3].stack_free),
	DATAEGETWEB("h3_pct",   PG_STM32, WV_U8,   0,  rtos_info.th[3].stack_pct),

	/* --- hilo 4 --- */
	DATAEGETWEB("h4_n",     PG_STM32, WV_STR,  16, rtos_info.th[4].name),
	DATAEGETWEB("h4_s",     PG_STM32, WV_U8,   0,  rtos_info.th[4].state),
	DATAEGETWEB("h4_p",     PG_STM32, WV_U8,   0,  rtos_info.th[4].prio),
	DATAEGETWEB("h4_c",     PG_STM32, WV_U8,   0,  rtos_info.th[4].cpu),
	DATAEGETWEB("h4_sz",    PG_STM32, WV_U32,  0,  rtos_info.th[4].stack_size),
	DATAEGETWEB("h4_lib",   PG_STM32, WV_U32,  0,  rtos_info.th[4].stack_free),
	DATAEGETWEB("h4_pct",   PG_STM32, WV_U8,   0,  rtos_info.th[4].stack_pct),

	/* --- hilo 5 --- */
	DATAEGETWEB("h5_n",     PG_STM32, WV_STR,  16, rtos_info.th[5].name),
	DATAEGETWEB("h5_s",     PG_STM32, WV_U8,   0,  rtos_info.th[5].state),
	DATAEGETWEB("h5_p",     PG_STM32, WV_U8,   0,  rtos_info.th[5].prio),
	DATAEGETWEB("h5_c",     PG_STM32, WV_U8,   0,  rtos_info.th[5].cpu),
	DATAEGETWEB("h5_sz",    PG_STM32, WV_U32,  0,  rtos_info.th[5].stack_size),
	DATAEGETWEB("h5_lib",   PG_STM32, WV_U32,  0,  rtos_info.th[5].stack_free),
	DATAEGETWEB("h5_pct",   PG_STM32, WV_U8,   0,  rtos_info.th[5].stack_pct),

	/* --- hilo 6 --- */
	DATAEGETWEB("h6_n",     PG_STM32, WV_STR,  16, rtos_info.th[6].name),
	DATAEGETWEB("h6_s",     PG_STM32, WV_U8,   0,  rtos_info.th[6].state),
	DATAEGETWEB("h6_p",     PG_STM32, WV_U8,   0,  rtos_info.th[6].prio),
	DATAEGETWEB("h6_c",     PG_STM32, WV_U8,   0,  rtos_info.th[6].cpu),
	DATAEGETWEB("h6_sz",    PG_STM32, WV_U32,  0,  rtos_info.th[6].stack_size),
	DATAEGETWEB("h6_lib",   PG_STM32, WV_U32,  0,  rtos_info.th[6].stack_free),
	DATAEGETWEB("h6_pct",   PG_STM32, WV_U8,   0,  rtos_info.th[6].stack_pct),

	/* --- hilo 7 --- */
	DATAEGETWEB("h7_n",     PG_STM32, WV_STR,  16, rtos_info.th[7].name),
	DATAEGETWEB("h7_s",     PG_STM32, WV_U8,   0,  rtos_info.th[7].state),
	DATAEGETWEB("h7_p",     PG_STM32, WV_U8,   0,  rtos_info.th[7].prio),
	DATAEGETWEB("h7_c",     PG_STM32, WV_U8,   0,  rtos_info.th[7].cpu),
	DATAEGETWEB("h7_sz",    PG_STM32, WV_U32,  0,  rtos_info.th[7].stack_size),
	DATAEGETWEB("h7_lib",   PG_STM32, WV_U32,  0,  rtos_info.th[7].stack_free),
	DATAEGETWEB("h7_pct",   PG_STM32, WV_U8,   0,  rtos_info.th[7].stack_pct),

	/* --- hilo 8 --- */
	DATAEGETWEB("h8_n",     PG_STM32, WV_STR,  16, rtos_info.th[8].name),
	DATAEGETWEB("h8_s",     PG_STM32, WV_U8,   0,  rtos_info.th[8].state),
	DATAEGETWEB("h8_p",     PG_STM32, WV_U8,   0,  rtos_info.th[8].prio),
	DATAEGETWEB("h8_c",     PG_STM32, WV_U8,   0,  rtos_info.th[8].cpu),
	DATAEGETWEB("h8_sz",    PG_STM32, WV_U32,  0,  rtos_info.th[8].stack_size),
	DATAEGETWEB("h8_lib",   PG_STM32, WV_U32,  0,  rtos_info.th[8].stack_free),
	DATAEGETWEB("h8_pct",   PG_STM32, WV_U8,   0,  rtos_info.th[8].stack_pct),

	/* --- hilo 9 --- */
	DATAEGETWEB("h9_n",     PG_STM32, WV_STR,  16, rtos_info.th[9].name),
	DATAEGETWEB("h9_s",     PG_STM32, WV_U8,   0,  rtos_info.th[9].state),
	DATAEGETWEB("h9_p",     PG_STM32, WV_U8,   0,  rtos_info.th[9].prio),
	DATAEGETWEB("h9_c",     PG_STM32, WV_U8,   0,  rtos_info.th[9].cpu),
	DATAEGETWEB("h9_sz",    PG_STM32, WV_U32,  0,  rtos_info.th[9].stack_size),
	DATAEGETWEB("h9_lib",   PG_STM32, WV_U32,  0,  rtos_info.th[9].stack_free),
	DATAEGETWEB("h9_pct",   PG_STM32, WV_U8,   0,  rtos_info.th[9].stack_pct),

  /* #############################################################################################
  * MODULO 3  - CLOCKS
  * ############################################################################################# */
    //MACRO     ID           PAGINAS     TIPO      DEC VARIABLE

	/* --- Frecuencias (Hz) --- */
	DATAEGETWEB("ck_sys",  PG_STM32, WV_U32,   0,  clock_info.sysclk),            /* reloj del sistema */
	DATAEGETWEB("ck_cpu",  PG_STM32, WV_U32,   0,  clock_info.cpu),               /* Cortex-M7 (SystemCoreClock) */
	DATAEGETWEB("ck_hclk", PG_STM32, WV_U32,   0,  clock_info.hclk),              /* bus AHB */
	DATAEGETWEB("ck_apb1", PG_STM32, WV_U32,   0,  clock_info.apb1),              /* APB1 */
	DATAEGETWEB("ck_apb2", PG_STM32, WV_U32,   0,  clock_info.apb2),              /* APB2 */
	DATAEGETWEB("ck_apb3", PG_STM32, WV_U32,   0,  clock_info.apb3),              /* APB3 */
	DATAEGETWEB("ck_apb4", PG_STM32, WV_U32,   0,  clock_info.apb4),              /* APB4 */
	DATAEGETWEB("ck_p1p",  PG_STM32, WV_U32,   0,  clock_info.pll1_p),            /* PLL1 P */
	DATAEGETWEB("ck_p1q",  PG_STM32, WV_U32,   0,  clock_info.pll1_q),            /* PLL1 Q */
	DATAEGETWEB("ck_p1r",  PG_STM32, WV_U32,   0,  clock_info.pll1_r),            /* PLL1 R */
	DATAEGETWEB("ck_p2p",  PG_STM32, WV_U32,   0,  clock_info.pll2_p),            /* PLL2 P */
	DATAEGETWEB("ck_p2q",  PG_STM32, WV_U32,   0,  clock_info.pll2_q),            /* PLL2 Q */
	DATAEGETWEB("ck_p2r",  PG_STM32, WV_U32,   0,  clock_info.pll2_r),            /* PLL2 R */
	DATAEGETWEB("ck_p3p",  PG_STM32, WV_U32,   0,  clock_info.pll3_p),            /* PLL3 P */
	DATAEGETWEB("ck_p3q",  PG_STM32, WV_U32,   0,  clock_info.pll3_q),            /* PLL3 Q */
	DATAEGETWEB("ck_p3r",  PG_STM32, WV_U32,   0,  clock_info.pll3_r),            /* PLL3 R */
	DATAEGETWEB("ck_hse",  PG_STM32, WV_U32,   0,  clock_info.hse_hz),            /* cristal externo */
	DATAEGETWEB("ck_hsi",  PG_STM32, WV_U32,   0,  clock_info.hsi_hz),            /* oscilador interno */

	/* --- Estado --- */
	DATAEGETWEB("ck_src",  PG_STM32, WV_U8,    0,  clock_info.sys_src),           /* origen SYSCLK: 0 HSI, 1 CSI, 2 HSE, 3 PLL1 */
	DATAEGETWEB("ck_psrc", PG_STM32, WV_U8,    0,  clock_info.pll_src),           /* entrada PLL: 0 HSI, 1 CSI, 2 HSE, 3 ninguna */
	DATAEGETWEB("ck_osc",  PG_STM32, WV_U16,   0,  clock_info.osc_on),            /* bits OSC_xxx (que osciladores van) */
	DATAEGETWEB("ck_vos",  PG_STM32, WV_U8,    0,  clock_info.vos),               /* escala de tension 0..3 */
	DATAEGETWEB("ck_ws",   PG_STM32, WV_U8,    0,  clock_info.flash_ws),          /* esperas de la flash */
	DATAEGETWEB("ck_dev",  PG_STM32, WV_U16,   0,  clock_info.dev_id),            /* 0x450 = H74x/H75x */
	DATAEGETWEB("ck_rev",  PG_STM32, WV_U16,   0,  clock_info.rev_id),            /* 0x1003 = Y, 0x2003 = V */
	DATAEGETWEB("ck_revt", PG_STM32, WV_STR,   4,  clock_info.rev_txt),           /* revision en texto */

	/* --- Analogico (ADC3) --- */
	DATAEGETWEB("ck_adc",  PG_STM32, WV_BOOL,  0,  clock_info.adc_ok),            /* 1 = ADC3 midiendo */
	DATAEGETWEB("ck_t",    PG_STM32, WV_FLOAT, 1,  clock_info.temp_c),            /* temperatura del chip (C) */
	DATAEGETWEB("ck_tmin", PG_STM32, WV_FLOAT, 1,  clock_info.temp_min),          /* minima desde el arranque */
	DATAEGETWEB("ck_tmax", PG_STM32, WV_FLOAT, 1,  clock_info.temp_max),          /* maxima desde el arranque */
	DATAEGETWEB("ck_vref", PG_STM32, WV_U16,   0,  clock_info.vref_mv),           /* Vref+ (mV) */
	DATAEGETWEB("ck_vbat", PG_STM32, WV_U16,   0,  clock_info.vbat_mv),           /* pila RTC (mV) */



  /* #############################################################################################
  * MODULO 4  - GPIO
  * ############################################################################################# */
  //MACRO     ID          PAGINAS   TIPO      DEC VARIABLE

  DATAEGETWEB("gp_on",    PG_STM32, WV_U8,    0,  gpio_info.ports_on),   /* puertos con reloj */

  /* --- GPIOA --- */
  DATAEGETWEB("ga_en",   PG_STM32, WV_BOOL,  0,  gpio_info.p[0].clk),     /* reloj activado */
  DATAEGETWEB("ga_i",    PG_STM32, WV_U16,   0,  gpio_info.p[0].idr),     /* nivel real de los pines */
  DATAEGETWEB("ga_o",    PG_STM32, WV_U16,   0,  gpio_info.p[0].odr),     /* salidas programadas */
  DATAEGETWEB("ga_t",    PG_STM32, WV_U16,   0,  gpio_info.p[0].otyper),  /* 1 = open-drain */
  DATAEGETWEB("ga_m",    PG_STM32, WV_U32,   0,  gpio_info.p[0].moder),   /* modo, 2 bits por pin */
  DATAEGETWEB("ga_p",    PG_STM32, WV_U32,   0,  gpio_info.p[0].pupdr),   /* pull, 2 bits por pin */
  DATAEGETWEB("ga_al",   PG_STM32, WV_U32,   0,  gpio_info.p[0].afrl),    /* AF pines 0-7 */
  DATAEGETWEB("ga_ah",   PG_STM32, WV_U32,   0,  gpio_info.p[0].afrh),    /* AF pines 8-15 */

  /* --- GPIOB --- */
  DATAEGETWEB("gb_en",   PG_STM32, WV_BOOL,  0,  gpio_info.p[1].clk),
  DATAEGETWEB("gb_i",    PG_STM32, WV_U16,   0,  gpio_info.p[1].idr),
  DATAEGETWEB("gb_o",    PG_STM32, WV_U16,   0,  gpio_info.p[1].odr),
  DATAEGETWEB("gb_t",    PG_STM32, WV_U16,   0,  gpio_info.p[1].otyper),
  DATAEGETWEB("gb_m",    PG_STM32, WV_U32,   0,  gpio_info.p[1].moder),
  DATAEGETWEB("gb_p",    PG_STM32, WV_U32,   0,  gpio_info.p[1].pupdr),
  DATAEGETWEB("gb_al",   PG_STM32, WV_U32,   0,  gpio_info.p[1].afrl),
  DATAEGETWEB("gb_ah",   PG_STM32, WV_U32,   0,  gpio_info.p[1].afrh),

  /* --- GPIOC --- */
  DATAEGETWEB("gc_en",   PG_STM32, WV_BOOL,  0,  gpio_info.p[2].clk),
  DATAEGETWEB("gc_i",    PG_STM32, WV_U16,   0,  gpio_info.p[2].idr),
  DATAEGETWEB("gc_o",    PG_STM32, WV_U16,   0,  gpio_info.p[2].odr),
  DATAEGETWEB("gc_t",    PG_STM32, WV_U16,   0,  gpio_info.p[2].otyper),
  DATAEGETWEB("gc_m",    PG_STM32, WV_U32,   0,  gpio_info.p[2].moder),
  DATAEGETWEB("gc_p",    PG_STM32, WV_U32,   0,  gpio_info.p[2].pupdr),
  DATAEGETWEB("gc_al",   PG_STM32, WV_U32,   0,  gpio_info.p[2].afrl),
  DATAEGETWEB("gc_ah",   PG_STM32, WV_U32,   0,  gpio_info.p[2].afrh),

  /* --- GPIOD --- */
  DATAEGETWEB("gd_en",   PG_STM32, WV_BOOL,  0,  gpio_info.p[3].clk),
  DATAEGETWEB("gd_i",    PG_STM32, WV_U16,   0,  gpio_info.p[3].idr),
  DATAEGETWEB("gd_o",    PG_STM32, WV_U16,   0,  gpio_info.p[3].odr),
  DATAEGETWEB("gd_t",    PG_STM32, WV_U16,   0,  gpio_info.p[3].otyper),
  DATAEGETWEB("gd_m",    PG_STM32, WV_U32,   0,  gpio_info.p[3].moder),
  DATAEGETWEB("gd_p",    PG_STM32, WV_U32,   0,  gpio_info.p[3].pupdr),
  DATAEGETWEB("gd_al",   PG_STM32, WV_U32,   0,  gpio_info.p[3].afrl),
  DATAEGETWEB("gd_ah",   PG_STM32, WV_U32,   0,  gpio_info.p[3].afrh),

  /* --- GPIOE --- */
  DATAEGETWEB("ge_en",   PG_STM32, WV_BOOL,  0,  gpio_info.p[4].clk),
  DATAEGETWEB("ge_i",    PG_STM32, WV_U16,   0,  gpio_info.p[4].idr),
  DATAEGETWEB("ge_o",    PG_STM32, WV_U16,   0,  gpio_info.p[4].odr),
  DATAEGETWEB("ge_t",    PG_STM32, WV_U16,   0,  gpio_info.p[4].otyper),
  DATAEGETWEB("ge_m",    PG_STM32, WV_U32,   0,  gpio_info.p[4].moder),
  DATAEGETWEB("ge_p",    PG_STM32, WV_U32,   0,  gpio_info.p[4].pupdr),
  DATAEGETWEB("ge_al",   PG_STM32, WV_U32,   0,  gpio_info.p[4].afrl),
  DATAEGETWEB("ge_ah",   PG_STM32, WV_U32,   0,  gpio_info.p[4].afrh),

  /* --- GPIOF --- */
  DATAEGETWEB("gf_en",   PG_STM32, WV_BOOL,  0,  gpio_info.p[5].clk),
  DATAEGETWEB("gf_i",    PG_STM32, WV_U16,   0,  gpio_info.p[5].idr),
  DATAEGETWEB("gf_o",    PG_STM32, WV_U16,   0,  gpio_info.p[5].odr),
  DATAEGETWEB("gf_t",    PG_STM32, WV_U16,   0,  gpio_info.p[5].otyper),
  DATAEGETWEB("gf_m",    PG_STM32, WV_U32,   0,  gpio_info.p[5].moder),
  DATAEGETWEB("gf_p",    PG_STM32, WV_U32,   0,  gpio_info.p[5].pupdr),
  DATAEGETWEB("gf_al",   PG_STM32, WV_U32,   0,  gpio_info.p[5].afrl),
  DATAEGETWEB("gf_ah",   PG_STM32, WV_U32,   0,  gpio_info.p[5].afrh),

  /* --- GPIOG --- */
  DATAEGETWEB("gg_en",   PG_STM32, WV_BOOL,  0,  gpio_info.p[6].clk),
  DATAEGETWEB("gg_i",    PG_STM32, WV_U16,   0,  gpio_info.p[6].idr),
  DATAEGETWEB("gg_o",    PG_STM32, WV_U16,   0,  gpio_info.p[6].odr),
  DATAEGETWEB("gg_t",    PG_STM32, WV_U16,   0,  gpio_info.p[6].otyper),
  DATAEGETWEB("gg_m",    PG_STM32, WV_U32,   0,  gpio_info.p[6].moder),
  DATAEGETWEB("gg_p",    PG_STM32, WV_U32,   0,  gpio_info.p[6].pupdr),
  DATAEGETWEB("gg_al",   PG_STM32, WV_U32,   0,  gpio_info.p[6].afrl),
  DATAEGETWEB("gg_ah",   PG_STM32, WV_U32,   0,  gpio_info.p[6].afrh),

  /* --- GPIOH --- */
  DATAEGETWEB("gh_en",   PG_STM32, WV_BOOL,  0,  gpio_info.p[7].clk),
  DATAEGETWEB("gh_i",    PG_STM32, WV_U16,   0,  gpio_info.p[7].idr),
  DATAEGETWEB("gh_o",    PG_STM32, WV_U16,   0,  gpio_info.p[7].odr),
  DATAEGETWEB("gh_t",    PG_STM32, WV_U16,   0,  gpio_info.p[7].otyper),
  DATAEGETWEB("gh_m",    PG_STM32, WV_U32,   0,  gpio_info.p[7].moder),
  DATAEGETWEB("gh_p",    PG_STM32, WV_U32,   0,  gpio_info.p[7].pupdr),
  DATAEGETWEB("gh_al",   PG_STM32, WV_U32,   0,  gpio_info.p[7].afrl),
  DATAEGETWEB("gh_ah",   PG_STM32, WV_U32,   0,  gpio_info.p[7].afrh),

  /* #############################################################################################
   * 1.1 GET  -  SOLO LECTURA (placa -> web)
   * ############################################################################################# */


  /* == PG_CONFIG  -  config.html - configuracion  (1) == */
  //MACRO      ID           PAGINAS    TIPO    DEC VARIABLE
  DATAEGETWEB( "contador2", PG_CONFIG, WV_U32, 0,  demo_contador2),

  /* == PG_WIDGETS  -  widgets.html - catalogo de widgets  (2) == */
  //MACRO      ID       PAGINAS     TIPO      DEC VARIABLE
  DATAEGETWEB( "roll",  PG_WIDGETS, WV_FLOAT, 1,  demo_roll),
  DATAEGETWEB( "pitch", PG_WIDGETS, WV_FLOAT, 1,  demo_pitch),

  /* == PG_INDUSTRIA  -  industria.html - maquinas / motores  (19) == */
  //MACRO      ID          PAGINAS       TIPO      DEC VARIABLE
  DATAEGETWEB( "m1_run",   PG_INDUSTRIA, WV_BOOL,  0,  ind_run[0]),
  DATAEGETWEB( "m2_run",   PG_INDUSTRIA, WV_BOOL,  0,  ind_run[1]),
  DATAEGETWEB( "m3_run",   PG_INDUSTRIA, WV_BOOL,  0,  ind_run[2]),
  DATAEGETWEB( "m1_rpm",   PG_INDUSTRIA, WV_U16,   0,  ind_rpm[0]),
  DATAEGETWEB( "m2_rpm",   PG_INDUSTRIA, WV_U16,   0,  ind_rpm[1]),
  DATAEGETWEB( "m3_rpm",   PG_INDUSTRIA, WV_U16,   0,  ind_rpm[2]),
  DATAEGETWEB( "m1_t",     PG_INDUSTRIA, WV_FLOAT, 1,  ind_temp[0]),
  DATAEGETWEB( "m2_t",     PG_INDUSTRIA, WV_FLOAT, 1,  ind_temp[1]),
  DATAEGETWEB( "m3_t",     PG_INDUSTRIA, WV_FLOAT, 1,  ind_temp[2]),
  DATAEGETWEB( "m1_a",     PG_INDUSTRIA, WV_FLOAT, 1,  ind_amp[0]),
  DATAEGETWEB( "m2_a",     PG_INDUSTRIA, WV_FLOAT, 1,  ind_amp[1]),
  DATAEGETWEB( "m3_a",     PG_INDUSTRIA, WV_FLOAT, 1,  ind_amp[2]),
  DATAEGETWEB( "estado",   PG_INDUSTRIA, WV_U8,    0,  ind_estado),
  DATAEGETWEB( "entradas", PG_INDUSTRIA, WV_U8,    0,  ind_entradas),
  DATAEGETWEB( "salidas",  PG_INDUSTRIA, WV_U8,    0,  ind_salidas),
  DATAEGETWEB( "presion",  PG_INDUSTRIA, WV_FLOAT, 1,  ind_presion),
  DATAEGETWEB( "piezas",   PG_INDUSTRIA, WV_U32,   0,  ind_piezas),
  DATAEGETWEB( "uptime",   PG_INDUSTRIA, WV_U32,   0,  ind_uptime),
  DATAEGETWEB( "valvula",  PG_INDUSTRIA, WV_BOOL,  0,  ind_valvula),

  /* == PG_INTERNO  -  interno.html - STM32: hilos, RAM, CPU  (67) == */
  //MACRO      ID         PAGINAS     TIPO     DEC VARIABLE
  DATAEGETWEB( "s_stats", PG_INTERNO, WV_BOOL, 0,  sys_stats_ok),
  DATAEGETWEB( "s_htot",  PG_INTERNO, WV_U32,  0,  sys_heap_tot),
  DATAEGETWEB( "s_hfree", PG_INTERNO, WV_U32,  0,  sys_heap_free),
  DATAEGETWEB( "s_hmin",  PG_INTERNO, WV_U32,  0,  sys_heap_min),
  DATAEGETWEB( "s_ram",   PG_INTERNO, WV_U32,  0,  sys_ram_stat),
  DATAEGETWEB( "s_nth",   PG_INTERNO, WV_U8,   0,  sys_threads),
  DATAEGETWEB( "s_ws",    PG_INTERNO, WV_U8,   0,  sys_ws),
  DATAEGETWEB( "t0_n",    PG_INTERNO, WV_STR,  15, sys_th_n[0]),
  DATAEGETWEB( "t0_s",    PG_INTERNO, WV_U8,   0,  sys_th_s[0]),
  DATAEGETWEB( "t0_p",    PG_INTERNO, WV_U8,   0,  sys_th_p[0]),
  DATAEGETWEB( "t0_c",    PG_INTERNO, WV_U8,   0,  sys_th_c[0]),
  DATAEGETWEB( "t0_f",    PG_INTERNO, WV_U16,  0,  sys_th_f[0]),
  DATAEGETWEB( "t1_n",    PG_INTERNO, WV_STR,  15, sys_th_n[1]),
  DATAEGETWEB( "t1_s",    PG_INTERNO, WV_U8,   0,  sys_th_s[1]),
  DATAEGETWEB( "t1_p",    PG_INTERNO, WV_U8,   0,  sys_th_p[1]),
  DATAEGETWEB( "t1_c",    PG_INTERNO, WV_U8,   0,  sys_th_c[1]),
  DATAEGETWEB( "t1_f",    PG_INTERNO, WV_U16,  0,  sys_th_f[1]),
  DATAEGETWEB( "t2_n",    PG_INTERNO, WV_STR,  15, sys_th_n[2]),
  DATAEGETWEB( "t2_s",    PG_INTERNO, WV_U8,   0,  sys_th_s[2]),
  DATAEGETWEB( "t2_p",    PG_INTERNO, WV_U8,   0,  sys_th_p[2]),
  DATAEGETWEB( "t2_c",    PG_INTERNO, WV_U8,   0,  sys_th_c[2]),
  DATAEGETWEB( "t2_f",    PG_INTERNO, WV_U16,  0,  sys_th_f[2]),
  DATAEGETWEB( "t3_n",    PG_INTERNO, WV_STR,  15, sys_th_n[3]),
  DATAEGETWEB( "t3_s",    PG_INTERNO, WV_U8,   0,  sys_th_s[3]),
  DATAEGETWEB( "t3_p",    PG_INTERNO, WV_U8,   0,  sys_th_p[3]),
  DATAEGETWEB( "t3_c",    PG_INTERNO, WV_U8,   0,  sys_th_c[3]),
  DATAEGETWEB( "t3_f",    PG_INTERNO, WV_U16,  0,  sys_th_f[3]),
  DATAEGETWEB( "t4_n",    PG_INTERNO, WV_STR,  15, sys_th_n[4]),
  DATAEGETWEB( "t4_s",    PG_INTERNO, WV_U8,   0,  sys_th_s[4]),
  DATAEGETWEB( "t4_p",    PG_INTERNO, WV_U8,   0,  sys_th_p[4]),
  DATAEGETWEB( "t4_c",    PG_INTERNO, WV_U8,   0,  sys_th_c[4]),
  DATAEGETWEB( "t4_f",    PG_INTERNO, WV_U16,  0,  sys_th_f[4]),
  DATAEGETWEB( "t5_n",    PG_INTERNO, WV_STR,  15, sys_th_n[5]),
  DATAEGETWEB( "t5_s",    PG_INTERNO, WV_U8,   0,  sys_th_s[5]),
  DATAEGETWEB( "t5_p",    PG_INTERNO, WV_U8,   0,  sys_th_p[5]),
  DATAEGETWEB( "t5_c",    PG_INTERNO, WV_U8,   0,  sys_th_c[5]),
  DATAEGETWEB( "t5_f",    PG_INTERNO, WV_U16,  0,  sys_th_f[5]),
  DATAEGETWEB( "t6_n",    PG_INTERNO, WV_STR,  15, sys_th_n[6]),
  DATAEGETWEB( "t6_s",    PG_INTERNO, WV_U8,   0,  sys_th_s[6]),
  DATAEGETWEB( "t6_p",    PG_INTERNO, WV_U8,   0,  sys_th_p[6]),
  DATAEGETWEB( "t6_c",    PG_INTERNO, WV_U8,   0,  sys_th_c[6]),
  DATAEGETWEB( "t6_f",    PG_INTERNO, WV_U16,  0,  sys_th_f[6]),
  DATAEGETWEB( "t7_n",    PG_INTERNO, WV_STR,  15, sys_th_n[7]),
  DATAEGETWEB( "t7_s",    PG_INTERNO, WV_U8,   0,  sys_th_s[7]),
  DATAEGETWEB( "t7_p",    PG_INTERNO, WV_U8,   0,  sys_th_p[7]),
  DATAEGETWEB( "t7_c",    PG_INTERNO, WV_U8,   0,  sys_th_c[7]),
  DATAEGETWEB( "t7_f",    PG_INTERNO, WV_U16,  0,  sys_th_f[7]),
  DATAEGETWEB( "t8_n",    PG_INTERNO, WV_STR,  15, sys_th_n[8]),
  DATAEGETWEB( "t8_s",    PG_INTERNO, WV_U8,   0,  sys_th_s[8]),
  DATAEGETWEB( "t8_p",    PG_INTERNO, WV_U8,   0,  sys_th_p[8]),
  DATAEGETWEB( "t8_c",    PG_INTERNO, WV_U8,   0,  sys_th_c[8]),
  DATAEGETWEB( "t8_f",    PG_INTERNO, WV_U16,  0,  sys_th_f[8]),
  DATAEGETWEB( "t9_n",    PG_INTERNO, WV_STR,  15, sys_th_n[9]),
  DATAEGETWEB( "t9_s",    PG_INTERNO, WV_U8,   0,  sys_th_s[9]),
  DATAEGETWEB( "t9_p",    PG_INTERNO, WV_U8,   0,  sys_th_p[9]),
  DATAEGETWEB( "t9_c",    PG_INTERNO, WV_U8,   0,  sys_th_c[9]),
  DATAEGETWEB( "t9_f",    PG_INTERNO, WV_U16,  0,  sys_th_f[9]),
  DATAEGETWEB( "t10_n",   PG_INTERNO, WV_STR,  15, sys_th_n[10]),
  DATAEGETWEB( "t10_s",   PG_INTERNO, WV_U8,   0,  sys_th_s[10]),
  DATAEGETWEB( "t10_p",   PG_INTERNO, WV_U8,   0,  sys_th_p[10]),
  DATAEGETWEB( "t10_c",   PG_INTERNO, WV_U8,   0,  sys_th_c[10]),
  DATAEGETWEB( "t10_f",   PG_INTERNO, WV_U16,  0,  sys_th_f[10]),
  DATAEGETWEB( "t11_n",   PG_INTERNO, WV_STR,  15, sys_th_n[11]),
  DATAEGETWEB( "t11_s",   PG_INTERNO, WV_U8,   0,  sys_th_s[11]),
  DATAEGETWEB( "t11_p",   PG_INTERNO, WV_U8,   0,  sys_th_p[11]),
  DATAEGETWEB( "t11_c",   PG_INTERNO, WV_U8,   0,  sys_th_c[11]),
  DATAEGETWEB( "t11_f",   PG_INTERNO, WV_U16,  0,  sys_th_f[11]),

  /* == PG_HVAC  -  hvac.html - climatizacion, circuito frigorifico  (9) == */
  //MACRO      ID        PAGINAS  TIPO      DEC VARIABLE
  DATAEGETWEB( "c2_td",  PG_HVAC, WV_FLOAT, 1,  c_td[1]),
  DATAEGETWEB( "c2_ts",  PG_HVAC, WV_FLOAT, 1,  c_ts[1]),
  DATAEGETWEB( "c2_eev", PG_HVAC, WV_U8,    0,  c_eev[1]),
  DATAEGETWEB( "c2_sh",  PG_HVAC, WV_FLOAT, 1,  c_sh[1]),
  DATAEGETWEB( "c2_sc",  PG_HVAC, WV_FLOAT, 1,  c_sc[1]),
  DATAEGETWEB( "c2_fc",  PG_HVAC, WV_U8,    0,  c_fc[1]),
  DATAEGETWEB( "c2_4v",  PG_HVAC, WV_BOOL,  0,  c_4v[1]),
  DATAEGETWEB( "c2_hg",  PG_HVAC, WV_BOOL,  0,  c_hg[1]),
  DATAEGETWEB( "c2_tyd", PG_HVAC, WV_U8,    0,  c2_tyd),

  /* == PG_COMPON  -  compon.html - componentes, mando manual, electrico  (7) == */
  //MACRO      ID      PAGINAS    TIPO      DEC VARIABLE
  DATAEGETWEB( "i_l1", PG_COMPON, WV_FLOAT, 1,  i_l[0]),
  DATAEGETWEB( "i_l2", PG_COMPON, WV_FLOAT, 1,  i_l[1]),
  DATAEGETWEB( "i_l3", PG_COMPON, WV_FLOAT, 1,  i_l[2]),
  DATAEGETWEB( "v_ll", PG_COMPON, WV_U16,   0,  v_ll),
  DATAEGETWEB( "kwh",  PG_COMPON, WV_U32,   0,  wh),
  DATAEGETWEB( "cv_a", PG_COMPON, WV_FLOAT, 1,  cv_a),
  DATAEGETWEB( "ev_a", PG_COMPON, WV_FLOAT, 1,  ev_a),

  /* == PG_OBJETO  -  objeto.html - identificacion, versiones, red, soporte  (10) == */
  //MACRO      ID           PAGINAS    TIPO    DEC VARIABLE
  DATAEGETWEB( "dev_name",  PG_OBJETO, WV_STR, 23, dev_name),
  DATAEGETWEB( "dev_hw",    PG_OBJETO, WV_STR, 11, dev_hw),
  DATAEGETWEB( "dev_mac",   PG_OBJETO, WV_STR, 17, dev_mac),
  DATAEGETWEB( "net_cmask", PG_OBJETO, WV_U32, 0,  net_cmask),
  DATAEGETWEB( "net_cgw",   PG_OBJETO, WV_U32, 0,  net_cgw),
  DATAEGETWEB( "net_cdns",  PG_OBJETO, WV_U32, 0,  net_cdns),
  DATAEGETWEB( "net_left",  PG_OBJETO, WV_U32, 0,  net_left),
  DATAEGETWEB( "net_t1",    PG_OBJETO, WV_U32, 0,  net_t1),
  DATAEGETWEB( "net_t2",    PG_OBJETO, WV_U32, 0,  net_t2),
  DATAEGETWEB( "net_srv",   PG_OBJETO, WV_U32, 0,  net_srv),

  /* == PG_WDATOS  -  wdatos.html - widgets de datos  (3) == */
  //MACRO      ID         PAGINAS    TIPO     DEC VARIABLE
  DATAEGETWEB( "bat",     PG_WDATOS, WV_U8,   0,  dem_bat),
  DATAEGETWEB( "rssi",    PG_WDATOS, WV_U8,   0,  dem_rssi),
  DATAEGETWEB( "bat_chg", PG_WDATOS, WV_BOOL, 0,  dem_chg),

  /* == PG_CAN  -  can.html - tramas y senales CAN  (70) == */
  //MACRO      ID         PAGINAS TIPO      DEC VARIABLE
  DATAEGETWEB( "cr0i",    PG_CAN, WV_U32,   0,  web_bus_rx[0][0].id),
  DATAEGETWEB( "cr0n",    PG_CAN, WV_U32,   0,  web_bus_rx[0][0].cnt),
  DATAEGETWEB( "cr0h",    PG_CAN, WV_U16,   0,  web_bus_rx[0][0].hz),
  DATAEGETWEB( "cr0t",    PG_CAN, WV_U16,   0,  web_bus_rx[0][0].age),
  DATAEGETWEB( "cr1i",    PG_CAN, WV_U32,   0,  web_bus_rx[0][1].id),
  DATAEGETWEB( "cr1l",    PG_CAN, WV_U8,    0,  web_bus_rx[0][1].len),
  DATAEGETWEB( "cr1n",    PG_CAN, WV_U32,   0,  web_bus_rx[0][1].cnt),
  DATAEGETWEB( "cr1h",    PG_CAN, WV_U16,   0,  web_bus_rx[0][1].hz),
  DATAEGETWEB( "cr1t",    PG_CAN, WV_U16,   0,  web_bus_rx[0][1].age),
  DATAEGETWEB( "cr2i",    PG_CAN, WV_U32,   0,  web_bus_rx[0][2].id),
  DATAEGETWEB( "cr2l",    PG_CAN, WV_U8,    0,  web_bus_rx[0][2].len),
  DATAEGETWEB( "cr2n",    PG_CAN, WV_U32,   0,  web_bus_rx[0][2].cnt),
  DATAEGETWEB( "cr2h",    PG_CAN, WV_U16,   0,  web_bus_rx[0][2].hz),
  DATAEGETWEB( "cr2t",    PG_CAN, WV_U16,   0,  web_bus_rx[0][2].age),
  DATAEGETWEB( "cr3i",    PG_CAN, WV_U32,   0,  web_bus_rx[0][3].id),
  DATAEGETWEB( "cr3l",    PG_CAN, WV_U8,    0,  web_bus_rx[0][3].len),
  DATAEGETWEB( "cr3n",    PG_CAN, WV_U32,   0,  web_bus_rx[0][3].cnt),
  DATAEGETWEB( "cr3h",    PG_CAN, WV_U16,   0,  web_bus_rx[0][3].hz),
  DATAEGETWEB( "cr3t",    PG_CAN, WV_U16,   0,  web_bus_rx[0][3].age),
  DATAEGETWEB( "cr4i",    PG_CAN, WV_U32,   0,  web_bus_rx[0][4].id),
  DATAEGETWEB( "cr4l",    PG_CAN, WV_U8,    0,  web_bus_rx[0][4].len),
  DATAEGETWEB( "cr4n",    PG_CAN, WV_U32,   0,  web_bus_rx[0][4].cnt),
  DATAEGETWEB( "cr4h",    PG_CAN, WV_U16,   0,  web_bus_rx[0][4].hz),
  DATAEGETWEB( "cr4t",    PG_CAN, WV_U16,   0,  web_bus_rx[0][4].age),
  DATAEGETWEB( "cr5i",    PG_CAN, WV_U32,   0,  web_bus_rx[0][5].id),
  DATAEGETWEB( "cr5l",    PG_CAN, WV_U8,    0,  web_bus_rx[0][5].len),
  DATAEGETWEB( "cr5n",    PG_CAN, WV_U32,   0,  web_bus_rx[0][5].cnt),
  DATAEGETWEB( "cr5h",    PG_CAN, WV_U16,   0,  web_bus_rx[0][5].hz),
  DATAEGETWEB( "cr5t",    PG_CAN, WV_U16,   0,  web_bus_rx[0][5].age),
  DATAEGETWEB( "cb_frx",  PG_CAN, WV_BOOL,  0,  web_bus_st[0].frx),
  DATAEGETWEB( "cb_ftx",  PG_CAN, WV_BOOL,  0,  web_bus_st[0].ftx),
  DATAEGETWEB( "cb_load", PG_CAN, WV_U8,    0,  web_bus_st[0].carga),
  DATAEGETWEB( "cb_left", PG_CAN, WV_U16,   0,  web_bus_st[0].left),
  DATAEGETWEB( "cb_rxn",  PG_CAN, WV_U32,   0,  web_bus_st[0].rxn),
  DATAEGETWEB( "cb_txn",  PG_CAN, WV_U32,   0,  web_bus_st[0].txn),
  DATAEGETWEB( "cb_err",  PG_CAN, WV_U32,   0,  web_bus_st[0].err),
  DATAEGETWEB( "cb_drop", PG_CAN, WV_U32,   0,  web_bus_st[0].drop),
  DATAEGETWEB( "gv0",     PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][0].val),
  DATAEGETWEB( "ga0",     PG_CAN, WV_U16,   0,  web_sig_rt[0][0].age),
  DATAEGETWEB( "gv1",     PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][1].val),
  DATAEGETWEB( "ga1",     PG_CAN, WV_U16,   0,  web_sig_rt[0][1].age),
  DATAEGETWEB( "gv2",     PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][2].val),
  DATAEGETWEB( "ga2",     PG_CAN, WV_U16,   0,  web_sig_rt[0][2].age),
  DATAEGETWEB( "gv3",     PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][3].val),
  DATAEGETWEB( "ga3",     PG_CAN, WV_U16,   0,  web_sig_rt[0][3].age),
  DATAEGETWEB( "gv4",     PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][4].val),
  DATAEGETWEB( "ga4",     PG_CAN, WV_U16,   0,  web_sig_rt[0][4].age),
  DATAEGETWEB( "gv5",     PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][5].val),
  DATAEGETWEB( "ga5",     PG_CAN, WV_U16,   0,  web_sig_rt[0][5].age),
  DATAEGETWEB( "gv6",     PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][6].val),
  DATAEGETWEB( "ga6",     PG_CAN, WV_U16,   0,  web_sig_rt[0][6].age),
  DATAEGETWEB( "gv7",     PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][7].val),
  DATAEGETWEB( "ga7",     PG_CAN, WV_U16,   0,  web_sig_rt[0][7].age),
  DATAEGETWEB( "gv8",     PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][8].val),
  DATAEGETWEB( "ga8",     PG_CAN, WV_U16,   0,  web_sig_rt[0][8].age),
  DATAEGETWEB( "gv9",     PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][9].val),
  DATAEGETWEB( "ga9",     PG_CAN, WV_U16,   0,  web_sig_rt[0][9].age),
  DATAEGETWEB( "gv10",    PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][10].val),
  DATAEGETWEB( "ga10",    PG_CAN, WV_U16,   0,  web_sig_rt[0][10].age),
  DATAEGETWEB( "gv11",    PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][11].val),
  DATAEGETWEB( "ga11",    PG_CAN, WV_U16,   0,  web_sig_rt[0][11].age),
  DATAEGETWEB( "gv12",    PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][12].val),
  DATAEGETWEB( "ga12",    PG_CAN, WV_U16,   0,  web_sig_rt[0][12].age),
  DATAEGETWEB( "gv13",    PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][13].val),
  DATAEGETWEB( "ga13",    PG_CAN, WV_U16,   0,  web_sig_rt[0][13].age),
  DATAEGETWEB( "gv14",    PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][14].val),
  DATAEGETWEB( "ga14",    PG_CAN, WV_U16,   0,  web_sig_rt[0][14].age),
  DATAEGETWEB( "gv15",    PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][15].val),
  DATAEGETWEB( "ga15",    PG_CAN, WV_U16,   0,  web_sig_rt[0][15].age),
  DATAEGETWEB( "cb_fsg",  PG_CAN, WV_BOOL,  0,  web_bus_st[0].fsg),

  /* == PG_TRDP  -  trdp.html - tramas y senales TRDP  (71) == */
  //MACRO      ID         PAGINAS  TIPO      DEC VARIABLE
  DATAEGETWEB( "dr0i",    PG_TRDP, WV_U32,   0,  web_bus_rx[1][0].id),
  DATAEGETWEB( "dr0l",    PG_TRDP, WV_U8,    0,  web_bus_rx[1][0].len),
  DATAEGETWEB( "dr0n",    PG_TRDP, WV_U32,   0,  web_bus_rx[1][0].cnt),
  DATAEGETWEB( "dr0h",    PG_TRDP, WV_U16,   0,  web_bus_rx[1][0].hz),
  DATAEGETWEB( "dr0t",    PG_TRDP, WV_U16,   0,  web_bus_rx[1][0].age),
  DATAEGETWEB( "dr1i",    PG_TRDP, WV_U32,   0,  web_bus_rx[1][1].id),
  DATAEGETWEB( "dr1l",    PG_TRDP, WV_U8,    0,  web_bus_rx[1][1].len),
  DATAEGETWEB( "dr1n",    PG_TRDP, WV_U32,   0,  web_bus_rx[1][1].cnt),
  DATAEGETWEB( "dr1h",    PG_TRDP, WV_U16,   0,  web_bus_rx[1][1].hz),
  DATAEGETWEB( "dr1t",    PG_TRDP, WV_U16,   0,  web_bus_rx[1][1].age),
  DATAEGETWEB( "dr2i",    PG_TRDP, WV_U32,   0,  web_bus_rx[1][2].id),
  DATAEGETWEB( "dr2l",    PG_TRDP, WV_U8,    0,  web_bus_rx[1][2].len),
  DATAEGETWEB( "dr2n",    PG_TRDP, WV_U32,   0,  web_bus_rx[1][2].cnt),
  DATAEGETWEB( "dr2h",    PG_TRDP, WV_U16,   0,  web_bus_rx[1][2].hz),
  DATAEGETWEB( "dr2t",    PG_TRDP, WV_U16,   0,  web_bus_rx[1][2].age),
  DATAEGETWEB( "dr3i",    PG_TRDP, WV_U32,   0,  web_bus_rx[1][3].id),
  DATAEGETWEB( "dr3l",    PG_TRDP, WV_U8,    0,  web_bus_rx[1][3].len),
  DATAEGETWEB( "dr3n",    PG_TRDP, WV_U32,   0,  web_bus_rx[1][3].cnt),
  DATAEGETWEB( "dr3h",    PG_TRDP, WV_U16,   0,  web_bus_rx[1][3].hz),
  DATAEGETWEB( "dr3t",    PG_TRDP, WV_U16,   0,  web_bus_rx[1][3].age),
  DATAEGETWEB( "dr4i",    PG_TRDP, WV_U32,   0,  web_bus_rx[1][4].id),
  DATAEGETWEB( "dr4l",    PG_TRDP, WV_U8,    0,  web_bus_rx[1][4].len),
  DATAEGETWEB( "dr4n",    PG_TRDP, WV_U32,   0,  web_bus_rx[1][4].cnt),
  DATAEGETWEB( "dr4h",    PG_TRDP, WV_U16,   0,  web_bus_rx[1][4].hz),
  DATAEGETWEB( "dr4t",    PG_TRDP, WV_U16,   0,  web_bus_rx[1][4].age),
  DATAEGETWEB( "dr5i",    PG_TRDP, WV_U32,   0,  web_bus_rx[1][5].id),
  DATAEGETWEB( "dr5l",    PG_TRDP, WV_U8,    0,  web_bus_rx[1][5].len),
  DATAEGETWEB( "dr5n",    PG_TRDP, WV_U32,   0,  web_bus_rx[1][5].cnt),
  DATAEGETWEB( "dr5h",    PG_TRDP, WV_U16,   0,  web_bus_rx[1][5].hz),
  DATAEGETWEB( "dr5t",    PG_TRDP, WV_U16,   0,  web_bus_rx[1][5].age),
  DATAEGETWEB( "db_frx",  PG_TRDP, WV_BOOL,  0,  web_bus_st[1].frx),
  DATAEGETWEB( "db_ftx",  PG_TRDP, WV_BOOL,  0,  web_bus_st[1].ftx),
  DATAEGETWEB( "db_load", PG_TRDP, WV_U8,    0,  web_bus_st[1].carga),
  DATAEGETWEB( "db_left", PG_TRDP, WV_U16,   0,  web_bus_st[1].left),
  DATAEGETWEB( "db_rxn",  PG_TRDP, WV_U32,   0,  web_bus_st[1].rxn),
  DATAEGETWEB( "db_txn",  PG_TRDP, WV_U32,   0,  web_bus_st[1].txn),
  DATAEGETWEB( "db_err",  PG_TRDP, WV_U32,   0,  web_bus_st[1].err),
  DATAEGETWEB( "db_drop", PG_TRDP, WV_U32,   0,  web_bus_st[1].drop),
  DATAEGETWEB( "tv0",     PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][0].val),
  DATAEGETWEB( "ta0",     PG_TRDP, WV_U16,   0,  web_sig_rt[1][0].age),
  DATAEGETWEB( "tv1",     PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][1].val),
  DATAEGETWEB( "ta1",     PG_TRDP, WV_U16,   0,  web_sig_rt[1][1].age),
  DATAEGETWEB( "tv2",     PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][2].val),
  DATAEGETWEB( "ta2",     PG_TRDP, WV_U16,   0,  web_sig_rt[1][2].age),
  DATAEGETWEB( "tv3",     PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][3].val),
  DATAEGETWEB( "ta3",     PG_TRDP, WV_U16,   0,  web_sig_rt[1][3].age),
  DATAEGETWEB( "tv4",     PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][4].val),
  DATAEGETWEB( "ta4",     PG_TRDP, WV_U16,   0,  web_sig_rt[1][4].age),
  DATAEGETWEB( "tv5",     PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][5].val),
  DATAEGETWEB( "ta5",     PG_TRDP, WV_U16,   0,  web_sig_rt[1][5].age),
  DATAEGETWEB( "tv6",     PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][6].val),
  DATAEGETWEB( "ta6",     PG_TRDP, WV_U16,   0,  web_sig_rt[1][6].age),
  DATAEGETWEB( "tv7",     PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][7].val),
  DATAEGETWEB( "ta7",     PG_TRDP, WV_U16,   0,  web_sig_rt[1][7].age),
  DATAEGETWEB( "tv8",     PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][8].val),
  DATAEGETWEB( "ta8",     PG_TRDP, WV_U16,   0,  web_sig_rt[1][8].age),
  DATAEGETWEB( "tv9",     PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][9].val),
  DATAEGETWEB( "ta9",     PG_TRDP, WV_U16,   0,  web_sig_rt[1][9].age),
  DATAEGETWEB( "tv10",    PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][10].val),
  DATAEGETWEB( "ta10",    PG_TRDP, WV_U16,   0,  web_sig_rt[1][10].age),
  DATAEGETWEB( "tv11",    PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][11].val),
  DATAEGETWEB( "ta11",    PG_TRDP, WV_U16,   0,  web_sig_rt[1][11].age),
  DATAEGETWEB( "tv12",    PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][12].val),
  DATAEGETWEB( "ta12",    PG_TRDP, WV_U16,   0,  web_sig_rt[1][12].age),
  DATAEGETWEB( "tv13",    PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][13].val),
  DATAEGETWEB( "ta13",    PG_TRDP, WV_U16,   0,  web_sig_rt[1][13].age),
  DATAEGETWEB( "tv14",    PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][14].val),
  DATAEGETWEB( "ta14",    PG_TRDP, WV_U16,   0,  web_sig_rt[1][14].age),
  DATAEGETWEB( "tv15",    PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][15].val),
  DATAEGETWEB( "ta15",    PG_TRDP, WV_U16,   0,  web_sig_rt[1][15].age),
  DATAEGETWEB( "db_fsg",  PG_TRDP, WV_BOOL,  0,  web_bus_st[1].fsg),

  /* == PG_ERR  -  errores.html - errores, historico, contadores  (170) == */
  //MACRO      ID        PAGINAS TIPO      DEC VARIABLE
  DATAEGETWEB( "er_act", PG_ERR, WV_U32,   0,  err_act),
  DATAEGETWEB( "er_una", PG_ERR, WV_U32,   0,  err_una),
  DATAEGETWEB( "er_n",   PG_ERR, WV_U8,    0,  err_n),
  DATAEGETWEB( "er_cfg", PG_ERR, WV_U8,    0,  err_cfg),
  DATAEGETWEB( "er_tot", PG_ERR, WV_U32,   0,  err_tot),
  DATAEGETWEB( "ec0",    PG_ERR, WV_U16,   0,  err_cnt[0]),
  DATAEGETWEB( "el0",    PG_ERR, WV_U32,   0,  err_last[0]),
  DATAEGETWEB( "ec1",    PG_ERR, WV_U16,   0,  err_cnt[1]),
  DATAEGETWEB( "el1",    PG_ERR, WV_U32,   0,  err_last[1]),
  DATAEGETWEB( "ec2",    PG_ERR, WV_U16,   0,  err_cnt[2]),
  DATAEGETWEB( "el2",    PG_ERR, WV_U32,   0,  err_last[2]),
  DATAEGETWEB( "ec3",    PG_ERR, WV_U16,   0,  err_cnt[3]),
  DATAEGETWEB( "el3",    PG_ERR, WV_U32,   0,  err_last[3]),
  DATAEGETWEB( "ec4",    PG_ERR, WV_U16,   0,  err_cnt[4]),
  DATAEGETWEB( "el4",    PG_ERR, WV_U32,   0,  err_last[4]),
  DATAEGETWEB( "ec5",    PG_ERR, WV_U16,   0,  err_cnt[5]),
  DATAEGETWEB( "el5",    PG_ERR, WV_U32,   0,  err_last[5]),
  DATAEGETWEB( "ec6",    PG_ERR, WV_U16,   0,  err_cnt[6]),
  DATAEGETWEB( "el6",    PG_ERR, WV_U32,   0,  err_last[6]),
  DATAEGETWEB( "ec7",    PG_ERR, WV_U16,   0,  err_cnt[7]),
  DATAEGETWEB( "el7",    PG_ERR, WV_U32,   0,  err_last[7]),
  DATAEGETWEB( "ec8",    PG_ERR, WV_U16,   0,  err_cnt[8]),
  DATAEGETWEB( "el8",    PG_ERR, WV_U32,   0,  err_last[8]),
  DATAEGETWEB( "ec9",    PG_ERR, WV_U16,   0,  err_cnt[9]),
  DATAEGETWEB( "el9",    PG_ERR, WV_U32,   0,  err_last[9]),
  DATAEGETWEB( "ec10",   PG_ERR, WV_U16,   0,  err_cnt[10]),
  DATAEGETWEB( "el10",   PG_ERR, WV_U32,   0,  err_last[10]),
  DATAEGETWEB( "ec11",   PG_ERR, WV_U16,   0,  err_cnt[11]),
  DATAEGETWEB( "el11",   PG_ERR, WV_U32,   0,  err_last[11]),
  DATAEGETWEB( "ec12",   PG_ERR, WV_U16,   0,  err_cnt[12]),
  DATAEGETWEB( "el12",   PG_ERR, WV_U32,   0,  err_last[12]),
  DATAEGETWEB( "ec13",   PG_ERR, WV_U16,   0,  err_cnt[13]),
  DATAEGETWEB( "el13",   PG_ERR, WV_U32,   0,  err_last[13]),
  DATAEGETWEB( "ec14",   PG_ERR, WV_U16,   0,  err_cnt[14]),
  DATAEGETWEB( "el14",   PG_ERR, WV_U32,   0,  err_last[14]),
  DATAEGETWEB( "ec15",   PG_ERR, WV_U16,   0,  err_cnt[15]),
  DATAEGETWEB( "el15",   PG_ERR, WV_U32,   0,  err_last[15]),
  DATAEGETWEB( "ec16",   PG_ERR, WV_U16,   0,  err_cnt[16]),
  DATAEGETWEB( "el16",   PG_ERR, WV_U32,   0,  err_last[16]),
  DATAEGETWEB( "ec17",   PG_ERR, WV_U16,   0,  err_cnt[17]),
  DATAEGETWEB( "el17",   PG_ERR, WV_U32,   0,  err_last[17]),
  DATAEGETWEB( "ec18",   PG_ERR, WV_U16,   0,  err_cnt[18]),
  DATAEGETWEB( "el18",   PG_ERR, WV_U32,   0,  err_last[18]),
  DATAEGETWEB( "ec19",   PG_ERR, WV_U16,   0,  err_cnt[19]),
  DATAEGETWEB( "el19",   PG_ERR, WV_U32,   0,  err_last[19]),
  DATAEGETWEB( "ec20",   PG_ERR, WV_U16,   0,  err_cnt[20]),
  DATAEGETWEB( "el20",   PG_ERR, WV_U32,   0,  err_last[20]),
  DATAEGETWEB( "ec21",   PG_ERR, WV_U16,   0,  err_cnt[21]),
  DATAEGETWEB( "el21",   PG_ERR, WV_U32,   0,  err_last[21]),
  DATAEGETWEB( "ec22",   PG_ERR, WV_U16,   0,  err_cnt[22]),
  DATAEGETWEB( "el22",   PG_ERR, WV_U32,   0,  err_last[22]),
  DATAEGETWEB( "ec23",   PG_ERR, WV_U16,   0,  err_cnt[23]),
  DATAEGETWEB( "el23",   PG_ERR, WV_U32,   0,  err_last[23]),
  DATAEGETWEB( "ec24",   PG_ERR, WV_U16,   0,  err_cnt[24]),
  DATAEGETWEB( "el24",   PG_ERR, WV_U32,   0,  err_last[24]),
  DATAEGETWEB( "ec25",   PG_ERR, WV_U16,   0,  err_cnt[25]),
  DATAEGETWEB( "el25",   PG_ERR, WV_U32,   0,  err_last[25]),
  DATAEGETWEB( "ec26",   PG_ERR, WV_U16,   0,  err_cnt[26]),
  DATAEGETWEB( "el26",   PG_ERR, WV_U32,   0,  err_last[26]),
  DATAEGETWEB( "ec27",   PG_ERR, WV_U16,   0,  err_cnt[27]),
  DATAEGETWEB( "el27",   PG_ERR, WV_U32,   0,  err_last[27]),
  DATAEGETWEB( "ec28",   PG_ERR, WV_U16,   0,  err_cnt[28]),
  DATAEGETWEB( "el28",   PG_ERR, WV_U32,   0,  err_last[28]),
  DATAEGETWEB( "ec29",   PG_ERR, WV_U16,   0,  err_cnt[29]),
  DATAEGETWEB( "el29",   PG_ERR, WV_U32,   0,  err_last[29]),
  DATAEGETWEB( "ec30",   PG_ERR, WV_U16,   0,  err_cnt[30]),
  DATAEGETWEB( "el30",   PG_ERR, WV_U32,   0,  err_last[30]),
  DATAEGETWEB( "ec31",   PG_ERR, WV_U16,   0,  err_cnt[31]),
  DATAEGETWEB( "el31",   PG_ERR, WV_U32,   0,  err_last[31]),
  DATAEGETWEB( "h0t",    PG_ERR, WV_U32,   0,  err_hist[0].t),
  DATAEGETWEB( "h0i",    PG_ERR, WV_U16,   0,  err_hist[0].id),
  DATAEGETWEB( "h0e",    PG_ERR, WV_U8,    0,  err_hist[0].ev),
  DATAEGETWEB( "h0d",    PG_ERR, WV_U32,   0,  err_hist[0].dur),
  DATAEGETWEB( "h0v",    PG_ERR, WV_FLOAT, 1,  err_hist[0].val),
  DATAEGETWEB( "h1t",    PG_ERR, WV_U32,   0,  err_hist[1].t),
  DATAEGETWEB( "h1i",    PG_ERR, WV_U16,   0,  err_hist[1].id),
  DATAEGETWEB( "h1e",    PG_ERR, WV_U8,    0,  err_hist[1].ev),
  DATAEGETWEB( "h1d",    PG_ERR, WV_U32,   0,  err_hist[1].dur),
  DATAEGETWEB( "h1v",    PG_ERR, WV_FLOAT, 1,  err_hist[1].val),
  DATAEGETWEB( "h2t",    PG_ERR, WV_U32,   0,  err_hist[2].t),
  DATAEGETWEB( "h2i",    PG_ERR, WV_U16,   0,  err_hist[2].id),
  DATAEGETWEB( "h2e",    PG_ERR, WV_U8,    0,  err_hist[2].ev),
  DATAEGETWEB( "h2d",    PG_ERR, WV_U32,   0,  err_hist[2].dur),
  DATAEGETWEB( "h2v",    PG_ERR, WV_FLOAT, 1,  err_hist[2].val),
  DATAEGETWEB( "h3t",    PG_ERR, WV_U32,   0,  err_hist[3].t),
  DATAEGETWEB( "h3i",    PG_ERR, WV_U16,   0,  err_hist[3].id),
  DATAEGETWEB( "h3e",    PG_ERR, WV_U8,    0,  err_hist[3].ev),
  DATAEGETWEB( "h3d",    PG_ERR, WV_U32,   0,  err_hist[3].dur),
  DATAEGETWEB( "h3v",    PG_ERR, WV_FLOAT, 1,  err_hist[3].val),
  DATAEGETWEB( "h4t",    PG_ERR, WV_U32,   0,  err_hist[4].t),
  DATAEGETWEB( "h4i",    PG_ERR, WV_U16,   0,  err_hist[4].id),
  DATAEGETWEB( "h4e",    PG_ERR, WV_U8,    0,  err_hist[4].ev),
  DATAEGETWEB( "h4d",    PG_ERR, WV_U32,   0,  err_hist[4].dur),
  DATAEGETWEB( "h4v",    PG_ERR, WV_FLOAT, 1,  err_hist[4].val),
  DATAEGETWEB( "h5t",    PG_ERR, WV_U32,   0,  err_hist[5].t),
  DATAEGETWEB( "h5i",    PG_ERR, WV_U16,   0,  err_hist[5].id),
  DATAEGETWEB( "h5e",    PG_ERR, WV_U8,    0,  err_hist[5].ev),
  DATAEGETWEB( "h5d",    PG_ERR, WV_U32,   0,  err_hist[5].dur),
  DATAEGETWEB( "h5v",    PG_ERR, WV_FLOAT, 1,  err_hist[5].val),
  DATAEGETWEB( "h6t",    PG_ERR, WV_U32,   0,  err_hist[6].t),
  DATAEGETWEB( "h6i",    PG_ERR, WV_U16,   0,  err_hist[6].id),
  DATAEGETWEB( "h6e",    PG_ERR, WV_U8,    0,  err_hist[6].ev),
  DATAEGETWEB( "h6d",    PG_ERR, WV_U32,   0,  err_hist[6].dur),
  DATAEGETWEB( "h6v",    PG_ERR, WV_FLOAT, 1,  err_hist[6].val),
  DATAEGETWEB( "h7t",    PG_ERR, WV_U32,   0,  err_hist[7].t),
  DATAEGETWEB( "h7i",    PG_ERR, WV_U16,   0,  err_hist[7].id),
  DATAEGETWEB( "h7e",    PG_ERR, WV_U8,    0,  err_hist[7].ev),
  DATAEGETWEB( "h7d",    PG_ERR, WV_U32,   0,  err_hist[7].dur),
  DATAEGETWEB( "h7v",    PG_ERR, WV_FLOAT, 1,  err_hist[7].val),
  DATAEGETWEB( "h8t",    PG_ERR, WV_U32,   0,  err_hist[8].t),
  DATAEGETWEB( "h8i",    PG_ERR, WV_U16,   0,  err_hist[8].id),
  DATAEGETWEB( "h8e",    PG_ERR, WV_U8,    0,  err_hist[8].ev),
  DATAEGETWEB( "h8d",    PG_ERR, WV_U32,   0,  err_hist[8].dur),
  DATAEGETWEB( "h8v",    PG_ERR, WV_FLOAT, 1,  err_hist[8].val),
  DATAEGETWEB( "h9t",    PG_ERR, WV_U32,   0,  err_hist[9].t),
  DATAEGETWEB( "h9i",    PG_ERR, WV_U16,   0,  err_hist[9].id),
  DATAEGETWEB( "h9e",    PG_ERR, WV_U8,    0,  err_hist[9].ev),
  DATAEGETWEB( "h9d",    PG_ERR, WV_U32,   0,  err_hist[9].dur),
  DATAEGETWEB( "h9v",    PG_ERR, WV_FLOAT, 1,  err_hist[9].val),
  DATAEGETWEB( "h10t",   PG_ERR, WV_U32,   0,  err_hist[10].t),
  DATAEGETWEB( "h10i",   PG_ERR, WV_U16,   0,  err_hist[10].id),
  DATAEGETWEB( "h10e",   PG_ERR, WV_U8,    0,  err_hist[10].ev),
  DATAEGETWEB( "h10d",   PG_ERR, WV_U32,   0,  err_hist[10].dur),
  DATAEGETWEB( "h10v",   PG_ERR, WV_FLOAT, 1,  err_hist[10].val),
  DATAEGETWEB( "h11t",   PG_ERR, WV_U32,   0,  err_hist[11].t),
  DATAEGETWEB( "h11i",   PG_ERR, WV_U16,   0,  err_hist[11].id),
  DATAEGETWEB( "h11e",   PG_ERR, WV_U8,    0,  err_hist[11].ev),
  DATAEGETWEB( "h11d",   PG_ERR, WV_U32,   0,  err_hist[11].dur),
  DATAEGETWEB( "h11v",   PG_ERR, WV_FLOAT, 1,  err_hist[11].val),
  DATAEGETWEB( "h12t",   PG_ERR, WV_U32,   0,  err_hist[12].t),
  DATAEGETWEB( "h12i",   PG_ERR, WV_U16,   0,  err_hist[12].id),
  DATAEGETWEB( "h12e",   PG_ERR, WV_U8,    0,  err_hist[12].ev),
  DATAEGETWEB( "h12d",   PG_ERR, WV_U32,   0,  err_hist[12].dur),
  DATAEGETWEB( "h12v",   PG_ERR, WV_FLOAT, 1,  err_hist[12].val),
  DATAEGETWEB( "h13t",   PG_ERR, WV_U32,   0,  err_hist[13].t),
  DATAEGETWEB( "h13i",   PG_ERR, WV_U16,   0,  err_hist[13].id),
  DATAEGETWEB( "h13e",   PG_ERR, WV_U8,    0,  err_hist[13].ev),
  DATAEGETWEB( "h13d",   PG_ERR, WV_U32,   0,  err_hist[13].dur),
  DATAEGETWEB( "h13v",   PG_ERR, WV_FLOAT, 1,  err_hist[13].val),
  DATAEGETWEB( "h14t",   PG_ERR, WV_U32,   0,  err_hist[14].t),
  DATAEGETWEB( "h14i",   PG_ERR, WV_U16,   0,  err_hist[14].id),
  DATAEGETWEB( "h14e",   PG_ERR, WV_U8,    0,  err_hist[14].ev),
  DATAEGETWEB( "h14d",   PG_ERR, WV_U32,   0,  err_hist[14].dur),
  DATAEGETWEB( "h14v",   PG_ERR, WV_FLOAT, 1,  err_hist[14].val),
  DATAEGETWEB( "h15t",   PG_ERR, WV_U32,   0,  err_hist[15].t),
  DATAEGETWEB( "h15i",   PG_ERR, WV_U16,   0,  err_hist[15].id),
  DATAEGETWEB( "h15e",   PG_ERR, WV_U8,    0,  err_hist[15].ev),
  DATAEGETWEB( "h15d",   PG_ERR, WV_U32,   0,  err_hist[15].dur),
  DATAEGETWEB( "h15v",   PG_ERR, WV_FLOAT, 1,  err_hist[15].val),
  DATAEGETWEB( "h16t",   PG_ERR, WV_U32,   0,  err_hist[16].t),
  DATAEGETWEB( "h16i",   PG_ERR, WV_U16,   0,  err_hist[16].id),
  DATAEGETWEB( "h16e",   PG_ERR, WV_U8,    0,  err_hist[16].ev),
  DATAEGETWEB( "h16d",   PG_ERR, WV_U32,   0,  err_hist[16].dur),
  DATAEGETWEB( "h16v",   PG_ERR, WV_FLOAT, 1,  err_hist[16].val),
  DATAEGETWEB( "h17t",   PG_ERR, WV_U32,   0,  err_hist[17].t),
  DATAEGETWEB( "h17i",   PG_ERR, WV_U16,   0,  err_hist[17].id),
  DATAEGETWEB( "h17e",   PG_ERR, WV_U8,    0,  err_hist[17].ev),
  DATAEGETWEB( "h17d",   PG_ERR, WV_U32,   0,  err_hist[17].dur),
  DATAEGETWEB( "h17v",   PG_ERR, WV_FLOAT, 1,  err_hist[17].val),
  DATAEGETWEB( "h18t",   PG_ERR, WV_U32,   0,  err_hist[18].t),
  DATAEGETWEB( "h18i",   PG_ERR, WV_U16,   0,  err_hist[18].id),
  DATAEGETWEB( "h18e",   PG_ERR, WV_U8,    0,  err_hist[18].ev),
  DATAEGETWEB( "h18d",   PG_ERR, WV_U32,   0,  err_hist[18].dur),
  DATAEGETWEB( "h18v",   PG_ERR, WV_FLOAT, 1,  err_hist[18].val),
  DATAEGETWEB( "h19t",   PG_ERR, WV_U32,   0,  err_hist[19].t),
  DATAEGETWEB( "h19i",   PG_ERR, WV_U16,   0,  err_hist[19].id),
  DATAEGETWEB( "h19e",   PG_ERR, WV_U8,    0,  err_hist[19].ev),
  DATAEGETWEB( "h19d",   PG_ERR, WV_U32,   0,  err_hist[19].dur),
  DATAEGETWEB( "h19v",   PG_ERR, WV_FLOAT, 1,  err_hist[19].val),
  DATAEGETWEB( "rtc_e",  PG_ERR, WV_U32,   0,  rtc_epoch),

  /* == PG_STM32  -  stm32.html) == */
  //MACRO           ID       PAGINAS   TIPO      DEC   VARIABLE
  DATAEGETWEB( "stm32_data", PG_STM32, WV_U32,   0,    err_act),

  /* #############################################################################################
   * 1.2 GET  -  SOLO LECTURA VARIAS PAGINA
   * ############################################################################################# */
  /* == empiezan en PG_DATOS  (3) == */
   //MACRO      ID      PAGINAS                                         TIPO      DEC VARIABLE
   DATAEGETWEB( "cont", PG_DATOS | PG_SISTEMA,                          WV_U32,   0,  demo_contador),
   DATAEGETWEB( "temp", PG_DATOS | PG_SISTEMA | PG_WIDGETS | PG_WDATOS, WV_FLOAT, 1,  demo_temp),
   DATAEGETWEB( "led",  PG_DATOS | PG_SISTEMA | PG_WIDGETS | PG_WDATOS, WV_BOOL,  0,  demo_led),

   /* == empiezan en PG_SISTEMA  (1) == */
   //MACRO      ID       PAGINAS                                             TIPO   DEC VARIABLE
   DATAEGETWEB( "nivel", PG_SISTEMA | PG_WIDGETS | PG_INDUSTRIA | PG_WDATOS, WV_U8, 0,  demo_nivel),

   /* == empiezan en PG_WIDGETS  (2) == */
   //MACRO      ID     PAGINAS                 TIPO      DEC VARIABLE
   DATAEGETWEB( "yaw", PG_WIDGETS | PG_WDATOS, WV_FLOAT, 1,  demo_yaw),
   DATAEGETWEB( "rpm", PG_WIDGETS | PG_WDATOS, WV_U16,   0,  demo_rpm),

   /* == empiezan en PG_INDUSTRIA  (1) == */
   //MACRO      ID        PAGINAS                   TIPO      DEC VARIABLE
   DATAEGETWEB( "caudal", PG_INDUSTRIA | PG_OBJETO, WV_FLOAT, 1,  ind_caudal),

   /* == empiezan en PG_INTERNO  (3) == */
   //MACRO      ID        PAGINAS                             TIPO    DEC VARIABLE
   DATAEGETWEB( "s_cpu",  PG_INTERNO | PG_WDATOS | PG_ERR,    WV_U8,  0,  sys_cpu),
   DATAEGETWEB( "s_hpct", PG_INTERNO | PG_WDATOS | PG_ERR,    WV_U8,  0,  sys_heap_pct),
   DATAEGETWEB( "s_up",   PG_INTERNO | PG_OBJETO | PG_WDATOS, WV_U32, 0,  sys_up),

   /* == empiezan en PG_HVAC  (32) == */
   //MACRO      ID         PAGINAS                                               TIPO      DEC VARIABLE
   DATAEGETWEB( "hreal",   PG_HVAC | PG_OBJETO,                                  WV_U8,    0,  hv_real),
   DATAEGETWEB( "t_int",   PG_HVAC | PG_COMPON | PG_OBJETO | PG_WDATOS | PG_ERR, WV_FLOAT, 1,  t_int),
   DATAEGETWEB( "t_ext",   PG_HVAC | PG_COMPON | PG_OBJETO | PG_WDATOS,          WV_FLOAT, 1,  t_ext),
   DATAEGETWEB( "t_imp",   PG_HVAC | PG_OBJETO,                                  WV_FLOAT, 1,  t_imp),
   DATAEGETWEB( "t_ret",   PG_HVAC | PG_OBJETO | PG_ERR,                         WV_FLOAT, 1,  t_ret),
   DATAEGETWEB( "hum",     PG_HVAC | PG_WDATOS,                                  WV_U8,    0,  hum),
   DATAEGETWEB( "co2",     PG_HVAC | PG_WDATOS | PG_ERR,                         WV_U16,   0,  co2),
   DATAEGETWEB( "pid_out", PG_HVAC | PG_OBJETO | PG_WDATOS,                      WV_FLOAT, 1,  pid_out),
   DATAEGETWEB( "c1_st",   PG_HVAC | PG_COMPON | PG_OBJETO,                      WV_U8,    0,  c_st[0]),
   DATAEGETWEB( "c1_hz",   PG_HVAC | PG_OBJETO,                                  WV_U8,    0,  c1_hz),
   DATAEGETWEB( "c1_a",    PG_HVAC | PG_COMPON | PG_OBJETO,                      WV_FLOAT, 1,  c_a[0]),
   DATAEGETWEB( "c1_hp",   PG_HVAC | PG_OBJETO | PG_WDATOS | PG_ERR,             WV_FLOAT, 1,  c_hp[0]),
   DATAEGETWEB( "c1_lp",   PG_HVAC | PG_OBJETO | PG_ERR,                         WV_FLOAT, 1,  c_lp[0]),
   DATAEGETWEB( "c1_td",   PG_HVAC | PG_OBJETO | PG_ERR,                         WV_FLOAT, 1,  c_td[0]),
   DATAEGETWEB( "c1_ts",   PG_HVAC | PG_OBJETO,                                  WV_FLOAT, 1,  c_ts[0]),
   DATAEGETWEB( "c1_eev",  PG_HVAC | PG_OBJETO,                                  WV_U8,    0,  c_eev[0]),
   DATAEGETWEB( "c1_sh",   PG_HVAC | PG_OBJETO,                                  WV_FLOAT, 1,  c_sh[0]),
   DATAEGETWEB( "c1_sc",   PG_HVAC | PG_OBJETO,                                  WV_FLOAT, 1,  c_sc[0]),
   DATAEGETWEB( "c1_fc",   PG_HVAC | PG_OBJETO,                                  WV_U8,    0,  c_fc[0]),
   DATAEGETWEB( "c1_4v",   PG_HVAC | PG_OBJETO,                                  WV_BOOL,  0,  c_4v[0]),
   DATAEGETWEB( "c1_hg",   PG_HVAC | PG_OBJETO,                                  WV_BOOL,  0,  c_hg[0]),
   DATAEGETWEB( "c2_st",   PG_HVAC | PG_COMPON | PG_OBJETO,                      WV_U8,    0,  c_st[1]),
   DATAEGETWEB( "c2_a",    PG_HVAC | PG_COMPON | PG_OBJETO,                      WV_FLOAT, 1,  c_a[1]),
   DATAEGETWEB( "c2_hp",   PG_HVAC | PG_OBJETO | PG_ERR,                         WV_FLOAT, 1,  c_hp[1]),
   DATAEGETWEB( "c2_lp",   PG_HVAC | PG_OBJETO | PG_ERR,                         WV_FLOAT, 1,  c_lp[1]),
   DATAEGETWEB( "c2_yd",   PG_HVAC | PG_COMPON | PG_OBJETO,                      WV_U8,    0,  c2_yd),
   DATAEGETWEB( "out_w",   PG_HVAC | PG_COMPON,                                  WV_U16,   0,  out_w),
   DATAEGETWEB( "in_w",    PG_HVAC | PG_COMPON | PG_OBJETO,                      WV_U8,    0,  in_w),
   DATAEGETWEB( "al_w",    PG_HVAC | PG_COMPON | PG_OBJETO | PG_ERR,             WV_U16,   0,  al_w),
   DATAEGETWEB( "ev_fan",  PG_HVAC | PG_COMPON | PG_OBJETO,                      WV_U8,    0,  ev_fan),
   DATAEGETWEB( "dp_filt", PG_HVAC | PG_COMPON | PG_OBJETO | PG_ERR,             WV_U16,   0,  dp_filt),
   DATAEGETWEB( "caudal",  PG_HVAC | PG_COMPON | PG_OBJETO,                      WV_U16,   0,  caudal_aire),

   /* == empiezan en PG_COMPON  (4) == */
   //MACRO      ID        PAGINAS                            TIPO      DEC VARIABLE
   DATAEGETWEB( "kw",     PG_COMPON | PG_OBJETO | PG_WDATOS, WV_FLOAT, 1,  kw),
   DATAEGETWEB( "desq",   PG_COMPON | PG_ERR,                WV_FLOAT, 1,  desq),
   DATAEGETWEB( "fl_fre", PG_COMPON | PG_OBJETO,             WV_U8,    0,  fl_fre),
   DATAEGETWEB( "fl_ret", PG_COMPON | PG_OBJETO,             WV_U8,    0,  fl_ret),

   /* == empiezan en PG_OBJETO  (38) == */
   //MACRO      ID         PAGINAS                TIPO     DEC VARIABLE
   DATAEGETWEB( "dev_sn",  PG_OBJETO | PG_WDATOS, WV_STR,  15, dev_sn),
   DATAEGETWEB( "f0_n",    PG_OBJETO | PG_WDATOS, WV_STR,  19, fw_n[0]),
   DATAEGETWEB( "f0_v",    PG_OBJETO | PG_WDATOS, WV_STR,  15, fw_v[0]),
   DATAEGETWEB( "f0_c",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_crc[0]),
   DATAEGETWEB( "f0_z",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_size[0]),
   DATAEGETWEB( "f0_k",    PG_OBJETO | PG_WDATOS, WV_BOOL, 0,  fw_ok[0]),
   DATAEGETWEB( "f1_n",    PG_OBJETO | PG_WDATOS, WV_STR,  19, fw_n[1]),
   DATAEGETWEB( "f1_v",    PG_OBJETO | PG_WDATOS, WV_STR,  15, fw_v[1]),
   DATAEGETWEB( "f1_c",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_crc[1]),
   DATAEGETWEB( "f1_z",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_size[1]),
   DATAEGETWEB( "f1_k",    PG_OBJETO | PG_WDATOS, WV_BOOL, 0,  fw_ok[1]),
   DATAEGETWEB( "f2_n",    PG_OBJETO | PG_WDATOS, WV_STR,  19, fw_n[2]),
   DATAEGETWEB( "f2_v",    PG_OBJETO | PG_WDATOS, WV_STR,  15, fw_v[2]),
   DATAEGETWEB( "f2_c",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_crc[2]),
   DATAEGETWEB( "f2_z",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_size[2]),
   DATAEGETWEB( "f2_k",    PG_OBJETO | PG_WDATOS, WV_BOOL, 0,  fw_ok[2]),
   DATAEGETWEB( "f3_n",    PG_OBJETO | PG_WDATOS, WV_STR,  19, fw_n[3]),
   DATAEGETWEB( "f3_v",    PG_OBJETO | PG_WDATOS, WV_STR,  15, fw_v[3]),
   DATAEGETWEB( "f3_c",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_crc[3]),
   DATAEGETWEB( "f3_z",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_size[3]),
   DATAEGETWEB( "f3_k",    PG_OBJETO | PG_WDATOS, WV_BOOL, 0,  fw_ok[3]),
   DATAEGETWEB( "f4_n",    PG_OBJETO | PG_WDATOS, WV_STR,  19, fw_n[4]),
   DATAEGETWEB( "f4_v",    PG_OBJETO | PG_WDATOS, WV_STR,  15, fw_v[4]),
   DATAEGETWEB( "f4_c",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_crc[4]),
   DATAEGETWEB( "f4_z",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_size[4]),
   DATAEGETWEB( "f4_k",    PG_OBJETO | PG_WDATOS, WV_BOOL, 0,  fw_ok[4]),
   DATAEGETWEB( "f5_n",    PG_OBJETO | PG_WDATOS, WV_STR,  19, fw_n[5]),
   DATAEGETWEB( "f5_v",    PG_OBJETO | PG_WDATOS, WV_STR,  15, fw_v[5]),
   DATAEGETWEB( "f5_c",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_crc[5]),
   DATAEGETWEB( "f5_z",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_size[5]),
   DATAEGETWEB( "f5_k",    PG_OBJETO | PG_WDATOS, WV_BOOL, 0,  fw_ok[5]),
   DATAEGETWEB( "f6_n",    PG_OBJETO | PG_WDATOS, WV_STR,  19, fw_n[6]),
   DATAEGETWEB( "f6_v",    PG_OBJETO | PG_WDATOS, WV_STR,  15, fw_v[6]),
   DATAEGETWEB( "f6_c",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_crc[6]),
   DATAEGETWEB( "f6_z",    PG_OBJETO | PG_WDATOS, WV_U32,  0,  fw_size[6]),
   DATAEGETWEB( "f6_k",    PG_OBJETO | PG_WDATOS, WV_BOOL, 0,  fw_ok[6]),
   DATAEGETWEB( "net_st",  PG_OBJETO | PG_WDATOS, WV_U8,   0,  net_state),
   DATAEGETWEB( "net_cip", PG_OBJETO | PG_WDATOS, WV_U32,  0,  net_cip),

   /* == empiezan en PG_WDATOS  (1) == */
   //MACRO      ID      PAGINAS             TIPO   DEC VARIABLE
   DATAEGETWEB( "cr0l", PG_WDATOS | PG_CAN, WV_U8, 0,  web_bus_rx[0][0].len),

   /* == empiezan en PG_CAN  (1) == */
   //MACRO      ID       PAGINAS          TIPO   DEC VARIABLE
   DATAEGETWEB( "cb_st", PG_CAN | PG_ERR, WV_U8, 0,  web_bus_st[0].estado),

   /* == empiezan en PG_TRDP  (1) == */
   //MACRO      ID       PAGINAS           TIPO   DEC VARIABLE
   DATAEGETWEB( "db_st", PG_TRDP | PG_ERR, WV_U8, 0,  web_bus_st[1].estado),

   /* #############################################################################################
    * 2. SET  -  LECTURA Y ESCRITURA (web -> placa)   (166 variables)
    * ############################################################################################# */

   /* ----- 2.1 UNA SOLA PAGINA (155) ------------------------------------------------------------ */

   /* == PG_WIDGETS  -  widgets.html - catalogo de widgets  (3) == */
   //MACRO     ID        PAGINAS     TIPO     DEC VARIABLE     MIN MAX
   DATASETWEB( "modo",   PG_WIDGETS, WV_U8,   0,  demo_modo,   0,  2),
   DATASETWEB( "enable", PG_WIDGETS, WV_BOOL, 0,  demo_enable, 0,  1),
   DATASETWEB( "brillo", PG_WIDGETS, WV_U8,   0,  demo_brillo, 0,  100),

   /* == PG_HVAC  -  hvac.html - climatizacion, circuito frigorifico  (5) == */
   //MACRO     ID          PAGINAS  TIPO      DEC VARIABLE  MIN MAX
   DATASETWEB( "pid_mode", PG_HVAC, WV_U8,    0,  pid_mode, 0,  1),
   DATASETWEB( "pid_man",  PG_HVAC, WV_FLOAT, 1,  pid_man,  0,  100),
   DATASETWEB( "pid_kp",   PG_HVAC, WV_FLOAT, 2,  pid_kp,   0,  50),
   DATASETWEB( "pid_ki",   PG_HVAC, WV_FLOAT, 2,  pid_ki,   0,  10),
   DATASETWEB( "pid_kd",   PG_HVAC, WV_FLOAT, 2,  pid_kd,   0,  10),

   /* == PG_COMPON  -  compon.html - componentes, mando manual, electrico  (10) == */
   //MACRO     ID           PAGINAS    TIPO   DEC VARIABLE   MIN MAX
   DATASETWEB( "m_c1",      PG_COMPON, WV_U8, 0,  m_c1,      0,  2),
   DATASETWEB( "m_c2",      PG_COMPON, WV_U8, 0,  m_c2,      0,  2),
   DATASETWEB( "m_ev",      PG_COMPON, WV_U8, 0,  m_ev,      0,  2),
   DATASETWEB( "m_cd",      PG_COMPON, WV_U8, 0,  m_cd,      0,  2),
   DATASETWEB( "m_hg",      PG_COMPON, WV_U8, 0,  m_hg,      0,  2),
   DATASETWEB( "m_4v",      PG_COMPON, WV_U8, 0,  m_4v,      0,  2),
   DATASETWEB( "m_cv",      PG_COMPON, WV_U8, 0,  m_cv,      0,  2),
   DATASETWEB( "m_fl",      PG_COMPON, WV_U8, 0,  m_fl,      0,  2),
   DATASETWEB( "fl_fre_sp", PG_COMPON, WV_U8, 0,  fl_fre_sp, 0,  100),
   DATASETWEB( "fl_ret_sp", PG_COMPON, WV_U8, 0,  fl_ret_sp, 0,  100),

   /* == PG_OBJETO  -  objeto.html - identificacion, versiones, red, soporte  (6) == */
   //MACRO     ID           PAGINAS    TIPO     DEC VARIABLE     MIN MAX
   DATASETWEB( "net_dhcp",  PG_OBJETO, WV_BOOL, 0,  net_dhcp,    0,  1),
   DATASETWEB( "net_mask",  PG_OBJETO, WV_U32,  0,  net_mask,    0,  4.29497e+09),
   DATASETWEB( "net_gw",    PG_OBJETO, WV_U32,  0,  net_gw,      0,  4.29497e+09),
   DATASETWEB( "net_dns",   PG_OBJETO, WV_U32,  0,  net_dns,     0,  4.29497e+09),
   DATASETWEB( "net_dto",   PG_OBJETO, WV_U16,  0,  net_dhcp_to, 1,  300),
   DATASETWEB( "net_lease", PG_OBJETO, WV_U32,  0,  net_lease,   60, 604800),

   /* == PG_CAN  -  can.html - tramas y senales CAN  (63) == */
   //MACRO     ID      PAGINAS TIPO      DEC VARIABLE                MIN     MAX
   DATASETWEB( "cr1a", PG_CAN, WV_U32,   0,  web_bus_rx[0][1].lo,    0,      4.29497e+09),
   DATASETWEB( "cr1b", PG_CAN, WV_U32,   0,  web_bus_rx[0][1].hi,    0,      4.29497e+09),
   DATASETWEB( "cr2a", PG_CAN, WV_U32,   0,  web_bus_rx[0][2].lo,    0,      4.29497e+09),
   DATASETWEB( "cr2b", PG_CAN, WV_U32,   0,  web_bus_rx[0][2].hi,    0,      4.29497e+09),
   DATASETWEB( "cr3a", PG_CAN, WV_U32,   0,  web_bus_rx[0][3].lo,    0,      4.29497e+09),
   DATASETWEB( "cr3b", PG_CAN, WV_U32,   0,  web_bus_rx[0][3].hi,    0,      4.29497e+09),
   DATASETWEB( "cr4a", PG_CAN, WV_U32,   0,  web_bus_rx[0][4].lo,    0,      4.29497e+09),
   DATASETWEB( "cr4b", PG_CAN, WV_U32,   0,  web_bus_rx[0][4].hi,    0,      4.29497e+09),
   DATASETWEB( "cr5a", PG_CAN, WV_U32,   0,  web_bus_rx[0][5].lo,    0,      4.29497e+09),
   DATASETWEB( "cr5b", PG_CAN, WV_U32,   0,  web_bus_rx[0][5].hi,    0,      4.29497e+09),
   DATASETWEB( "cx0i", PG_CAN, WV_U32,   0,  web_bus_tx[0][0].id,    0,      4.29497e+09),
   DATASETWEB( "cx0p", PG_CAN, WV_U32,   0,  web_bus_tx[0][0].per,   0,      60000),
   DATASETWEB( "cx0e", PG_CAN, WV_BOOL,  0,  web_bus_tx[0][0].en,    0,      1),
   DATASETWEB( "cx1i", PG_CAN, WV_U32,   0,  web_bus_tx[0][1].id,    0,      4.29497e+09),
   DATASETWEB( "cx1l", PG_CAN, WV_U8,    0,  web_bus_tx[0][1].len,   0,      8),
   DATASETWEB( "cx1a", PG_CAN, WV_U32,   0,  web_bus_tx[0][1].lo,    0,      4.29497e+09),
   DATASETWEB( "cx1b", PG_CAN, WV_U32,   0,  web_bus_tx[0][1].hi,    0,      4.29497e+09),
   DATASETWEB( "cx1p", PG_CAN, WV_U32,   0,  web_bus_tx[0][1].per,   0,      60000),
   DATASETWEB( "cx1e", PG_CAN, WV_BOOL,  0,  web_bus_tx[0][1].en,    0,      1),
   DATASETWEB( "cx2i", PG_CAN, WV_U32,   0,  web_bus_tx[0][2].id,    0,      4.29497e+09),
   DATASETWEB( "cx2l", PG_CAN, WV_U8,    0,  web_bus_tx[0][2].len,   0,      8),
   DATASETWEB( "cx2a", PG_CAN, WV_U32,   0,  web_bus_tx[0][2].lo,    0,      4.29497e+09),
   DATASETWEB( "cx2b", PG_CAN, WV_U32,   0,  web_bus_tx[0][2].hi,    0,      4.29497e+09),
   DATASETWEB( "cx2p", PG_CAN, WV_U32,   0,  web_bus_tx[0][2].per,   0,      60000),
   DATASETWEB( "cx2e", PG_CAN, WV_BOOL,  0,  web_bus_tx[0][2].en,    0,      1),
   DATASETWEB( "cx3i", PG_CAN, WV_U32,   0,  web_bus_tx[0][3].id,    0,      4.29497e+09),
   DATASETWEB( "cx3l", PG_CAN, WV_U8,    0,  web_bus_tx[0][3].len,   0,      8),
   DATASETWEB( "cx3a", PG_CAN, WV_U32,   0,  web_bus_tx[0][3].lo,    0,      4.29497e+09),
   DATASETWEB( "cx3b", PG_CAN, WV_U32,   0,  web_bus_tx[0][3].hi,    0,      4.29497e+09),
   DATASETWEB( "cx3p", PG_CAN, WV_U32,   0,  web_bus_tx[0][3].per,   0,      60000),
   DATASETWEB( "cx3e", PG_CAN, WV_BOOL,  0,  web_bus_tx[0][3].en,    0,      1),
   DATASETWEB( "gf0",  PG_CAN, WV_BOOL,  0,  web_sig_rt[0][0].fen,   0,      1),
   DATASETWEB( "gx0",  PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][0].fval,  -1e+09, 1e+09),
   DATASETWEB( "gf1",  PG_CAN, WV_BOOL,  0,  web_sig_rt[0][1].fen,   0,      1),
   DATASETWEB( "gx1",  PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][1].fval,  -1e+09, 1e+09),
   DATASETWEB( "gf2",  PG_CAN, WV_BOOL,  0,  web_sig_rt[0][2].fen,   0,      1),
   DATASETWEB( "gx2",  PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][2].fval,  -1e+09, 1e+09),
   DATASETWEB( "gf3",  PG_CAN, WV_BOOL,  0,  web_sig_rt[0][3].fen,   0,      1),
   DATASETWEB( "gx3",  PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][3].fval,  -1e+09, 1e+09),
   DATASETWEB( "gf4",  PG_CAN, WV_BOOL,  0,  web_sig_rt[0][4].fen,   0,      1),
   DATASETWEB( "gx4",  PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][4].fval,  -1e+09, 1e+09),
   DATASETWEB( "gf5",  PG_CAN, WV_BOOL,  0,  web_sig_rt[0][5].fen,   0,      1),
   DATASETWEB( "gx5",  PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][5].fval,  -1e+09, 1e+09),
   DATASETWEB( "gf6",  PG_CAN, WV_BOOL,  0,  web_sig_rt[0][6].fen,   0,      1),
   DATASETWEB( "gx6",  PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][6].fval,  -1e+09, 1e+09),
   DATASETWEB( "gf7",  PG_CAN, WV_BOOL,  0,  web_sig_rt[0][7].fen,   0,      1),
   DATASETWEB( "gx7",  PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][7].fval,  -1e+09, 1e+09),
   DATASETWEB( "gf8",  PG_CAN, WV_BOOL,  0,  web_sig_rt[0][8].fen,   0,      1),
   DATASETWEB( "gx8",  PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][8].fval,  -1e+09, 1e+09),
   DATASETWEB( "gf9",  PG_CAN, WV_BOOL,  0,  web_sig_rt[0][9].fen,   0,      1),
   DATASETWEB( "gx9",  PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][9].fval,  -1e+09, 1e+09),
   DATASETWEB( "gf10", PG_CAN, WV_BOOL,  0,  web_sig_rt[0][10].fen,  0,      1),
   DATASETWEB( "gx10", PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][10].fval, -1e+09, 1e+09),
   DATASETWEB( "gf11", PG_CAN, WV_BOOL,  0,  web_sig_rt[0][11].fen,  0,      1),
   DATASETWEB( "gx11", PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][11].fval, -1e+09, 1e+09),
   DATASETWEB( "gf12", PG_CAN, WV_BOOL,  0,  web_sig_rt[0][12].fen,  0,      1),
   DATASETWEB( "gx12", PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][12].fval, -1e+09, 1e+09),
   DATASETWEB( "gf13", PG_CAN, WV_BOOL,  0,  web_sig_rt[0][13].fen,  0,      1),
   DATASETWEB( "gx13", PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][13].fval, -1e+09, 1e+09),
   DATASETWEB( "gf14", PG_CAN, WV_BOOL,  0,  web_sig_rt[0][14].fen,  0,      1),
   DATASETWEB( "gx14", PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][14].fval, -1e+09, 1e+09),
   DATASETWEB( "gf15", PG_CAN, WV_BOOL,  0,  web_sig_rt[0][15].fen,  0,      1),
   DATASETWEB( "gx15", PG_CAN, WV_FLOAT, 3,  web_sig_rt[0][15].fval, -1e+09, 1e+09),

   /* == PG_TRDP  -  trdp.html - tramas y senales TRDP  (68) == */
   //MACRO     ID      PAGINAS  TIPO      DEC VARIABLE                MIN     MAX
   DATASETWEB( "dr0a", PG_TRDP, WV_U32,   0,  web_bus_rx[1][0].lo,    0,      4.29497e+09),
   DATASETWEB( "dr0b", PG_TRDP, WV_U32,   0,  web_bus_rx[1][0].hi,    0,      4.29497e+09),
   DATASETWEB( "dr1a", PG_TRDP, WV_U32,   0,  web_bus_rx[1][1].lo,    0,      4.29497e+09),
   DATASETWEB( "dr1b", PG_TRDP, WV_U32,   0,  web_bus_rx[1][1].hi,    0,      4.29497e+09),
   DATASETWEB( "dr2a", PG_TRDP, WV_U32,   0,  web_bus_rx[1][2].lo,    0,      4.29497e+09),
   DATASETWEB( "dr2b", PG_TRDP, WV_U32,   0,  web_bus_rx[1][2].hi,    0,      4.29497e+09),
   DATASETWEB( "dr3a", PG_TRDP, WV_U32,   0,  web_bus_rx[1][3].lo,    0,      4.29497e+09),
   DATASETWEB( "dr3b", PG_TRDP, WV_U32,   0,  web_bus_rx[1][3].hi,    0,      4.29497e+09),
   DATASETWEB( "dr4a", PG_TRDP, WV_U32,   0,  web_bus_rx[1][4].lo,    0,      4.29497e+09),
   DATASETWEB( "dr4b", PG_TRDP, WV_U32,   0,  web_bus_rx[1][4].hi,    0,      4.29497e+09),
   DATASETWEB( "dr5a", PG_TRDP, WV_U32,   0,  web_bus_rx[1][5].lo,    0,      4.29497e+09),
   DATASETWEB( "dr5b", PG_TRDP, WV_U32,   0,  web_bus_rx[1][5].hi,    0,      4.29497e+09),
   DATASETWEB( "dx0i", PG_TRDP, WV_U32,   0,  web_bus_tx[1][0].id,    0,      4.29497e+09),
   DATASETWEB( "dx0l", PG_TRDP, WV_U8,    0,  web_bus_tx[1][0].len,   0,      8),
   DATASETWEB( "dx0a", PG_TRDP, WV_U32,   0,  web_bus_tx[1][0].lo,    0,      4.29497e+09),
   DATASETWEB( "dx0b", PG_TRDP, WV_U32,   0,  web_bus_tx[1][0].hi,    0,      4.29497e+09),
   DATASETWEB( "dx0p", PG_TRDP, WV_U32,   0,  web_bus_tx[1][0].per,   0,      60000),
   DATASETWEB( "dx0e", PG_TRDP, WV_BOOL,  0,  web_bus_tx[1][0].en,    0,      1),
   DATASETWEB( "dx1i", PG_TRDP, WV_U32,   0,  web_bus_tx[1][1].id,    0,      4.29497e+09),
   DATASETWEB( "dx1l", PG_TRDP, WV_U8,    0,  web_bus_tx[1][1].len,   0,      8),
   DATASETWEB( "dx1a", PG_TRDP, WV_U32,   0,  web_bus_tx[1][1].lo,    0,      4.29497e+09),
   DATASETWEB( "dx1b", PG_TRDP, WV_U32,   0,  web_bus_tx[1][1].hi,    0,      4.29497e+09),
   DATASETWEB( "dx1p", PG_TRDP, WV_U32,   0,  web_bus_tx[1][1].per,   0,      60000),
   DATASETWEB( "dx1e", PG_TRDP, WV_BOOL,  0,  web_bus_tx[1][1].en,    0,      1),
   DATASETWEB( "dx2i", PG_TRDP, WV_U32,   0,  web_bus_tx[1][2].id,    0,      4.29497e+09),
   DATASETWEB( "dx2l", PG_TRDP, WV_U8,    0,  web_bus_tx[1][2].len,   0,      8),
   DATASETWEB( "dx2a", PG_TRDP, WV_U32,   0,  web_bus_tx[1][2].lo,    0,      4.29497e+09),
   DATASETWEB( "dx2b", PG_TRDP, WV_U32,   0,  web_bus_tx[1][2].hi,    0,      4.29497e+09),
   DATASETWEB( "dx2p", PG_TRDP, WV_U32,   0,  web_bus_tx[1][2].per,   0,      60000),
   DATASETWEB( "dx2e", PG_TRDP, WV_BOOL,  0,  web_bus_tx[1][2].en,    0,      1),
   DATASETWEB( "dx3i", PG_TRDP, WV_U32,   0,  web_bus_tx[1][3].id,    0,      4.29497e+09),
   DATASETWEB( "dx3l", PG_TRDP, WV_U8,    0,  web_bus_tx[1][3].len,   0,      8),
   DATASETWEB( "dx3a", PG_TRDP, WV_U32,   0,  web_bus_tx[1][3].lo,    0,      4.29497e+09),
   DATASETWEB( "dx3b", PG_TRDP, WV_U32,   0,  web_bus_tx[1][3].hi,    0,      4.29497e+09),
   DATASETWEB( "dx3p", PG_TRDP, WV_U32,   0,  web_bus_tx[1][3].per,   0,      60000),
   DATASETWEB( "dx3e", PG_TRDP, WV_BOOL,  0,  web_bus_tx[1][3].en,    0,      1),
   DATASETWEB( "tf0",  PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][0].fen,   0,      1),
   DATASETWEB( "tx0",  PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][0].fval,  -1e+09, 1e+09),
   DATASETWEB( "tf1",  PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][1].fen,   0,      1),
   DATASETWEB( "tx1",  PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][1].fval,  -1e+09, 1e+09),
   DATASETWEB( "tf2",  PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][2].fen,   0,      1),
   DATASETWEB( "tx2",  PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][2].fval,  -1e+09, 1e+09),
   DATASETWEB( "tf3",  PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][3].fen,   0,      1),
   DATASETWEB( "tx3",  PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][3].fval,  -1e+09, 1e+09),
   DATASETWEB( "tf4",  PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][4].fen,   0,      1),
   DATASETWEB( "tx4",  PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][4].fval,  -1e+09, 1e+09),
   DATASETWEB( "tf5",  PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][5].fen,   0,      1),
   DATASETWEB( "tx5",  PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][5].fval,  -1e+09, 1e+09),
   DATASETWEB( "tf6",  PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][6].fen,   0,      1),
   DATASETWEB( "tx6",  PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][6].fval,  -1e+09, 1e+09),
   DATASETWEB( "tf7",  PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][7].fen,   0,      1),
   DATASETWEB( "tx7",  PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][7].fval,  -1e+09, 1e+09),
   DATASETWEB( "tf8",  PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][8].fen,   0,      1),
   DATASETWEB( "tx8",  PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][8].fval,  -1e+09, 1e+09),
   DATASETWEB( "tf9",  PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][9].fen,   0,      1),
   DATASETWEB( "tx9",  PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][9].fval,  -1e+09, 1e+09),
   DATASETWEB( "tf10", PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][10].fen,  0,      1),
   DATASETWEB( "tx10", PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][10].fval, -1e+09, 1e+09),
   DATASETWEB( "tf11", PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][11].fen,  0,      1),
   DATASETWEB( "tx11", PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][11].fval, -1e+09, 1e+09),
   DATASETWEB( "tf12", PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][12].fen,  0,      1),
   DATASETWEB( "tx12", PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][12].fval, -1e+09, 1e+09),
   DATASETWEB( "tf13", PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][13].fen,  0,      1),
   DATASETWEB( "tx13", PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][13].fval, -1e+09, 1e+09),
   DATASETWEB( "tf14", PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][14].fen,  0,      1),
   DATASETWEB( "tx14", PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][14].fval, -1e+09, 1e+09),
   DATASETWEB( "tf15", PG_TRDP, WV_BOOL,  0,  web_sig_rt[1][15].fen,  0,      1),
   DATASETWEB( "tx15", PG_TRDP, WV_FLOAT, 3,  web_sig_rt[1][15].fval, -1e+09, 1e+09),

   /* ----- 2.2 VARIAS PAGINAS (11) -------------------------------------------------------------- */

   /* == empiezan en PG_SISTEMA  (1) == */
   //MACRO     ID          PAGINAS                  TIPO      DEC VARIABLE       MIN MAX
   DATASETWEB( "setpoint", PG_SISTEMA | PG_WIDGETS, WV_FLOAT, 1,  demo_setpoint, 10, 35),

   /* == empiezan en PG_HVAC  (2) == */
   //MACRO     ID       PAGINAS                       TIPO      DEC VARIABLE MIN MAX
   DATASETWEB( "hmodo", PG_HVAC | PG_OBJETO,          WV_U8,    0,  hv_modo, 0,  4),
   DATASETWEB( "sp_t",  PG_HVAC | PG_OBJETO | PG_ERR, WV_FLOAT, 1,  sp_t,    10, 30),

   /* == empiezan en PG_OBJETO  (3) == */
   //MACRO     ID        PAGINAS                         TIPO    DEC VARIABLE   MIN   MAX
   DATASETWEB( "rtc",    PG_OBJETO | PG_WDATOS,          WV_U32, 0,  rtc_epoch, 0,    4.29497e+09),
   DATASETWEB( "rtc_tz", PG_OBJETO | PG_WDATOS | PG_ERR, WV_I32, 0,  rtc_tz,    -720, 840),
   DATASETWEB( "net_ip", PG_OBJETO | PG_WDATOS,          WV_U32, 0,  net_ip,    0,    4.29497e+09),

   /* == empiezan en PG_WDATOS  (5) == */
   //MACRO     ID      PAGINAS             TIPO    DEC VARIABLE              MIN MAX
   DATASETWEB( "cr0a", PG_WDATOS | PG_CAN, WV_U32, 0,  web_bus_rx[0][0].lo,  0,  4.29497e+09),
   DATASETWEB( "cr0b", PG_WDATOS | PG_CAN, WV_U32, 0,  web_bus_rx[0][0].hi,  0,  4.29497e+09),
   DATASETWEB( "cx0l", PG_WDATOS | PG_CAN, WV_U8,  0,  web_bus_tx[0][0].len, 0,  8),
   DATASETWEB( "cx0a", PG_WDATOS | PG_CAN, WV_U32, 0,  web_bus_tx[0][0].lo,  0,  4.29497e+09),
   DATASETWEB( "cx0b", PG_WDATOS | PG_CAN, WV_U32, 0,  web_bus_tx[0][0].hi,  0,  4.29497e+09),
 };

#define WEB_VARS_N  (sizeof(web_vars) / sizeof(web_vars[0]))








/* Paginas que reciben el texto del terminal (WebTerm_Print) */
#define TERM_PAGES  (PG_SISTEMA | PG_WIDGETS | PG_INDUSTRIA | PG_INTERNO | PG_HVAC)
uint16_t WebTerm_Pages(void) { return TERM_PAGES; }
/* ===========================================================================
 * 4) COMANDOS (botones de la web:  <button data-cmd="nombre" data-arg="x">).
 *    Se ejecutan en el hilo de red: que sean cortas (poner un flag, cambiar una
 *    variable, avisar a otro hilo con una cola). Nada de esperas largas.
 * =========================================================================== */
static void cmd_reset_cont(const char *arg) { (void) arg; demo_contador = 0; }
static void cmd_toggle_led(const char *arg) { (void) arg; demo_led = !demo_led; }
static void cmd_saludo(const char *arg)     { WebTerm_Print("comando saludo, arg:"); WebTerm_Print(arg); }
static void cmd_start(const char *arg)      { (void) arg; ind_auto = 0; ind_estado = 2; }
static void cmd_stop(const char *arg)       { (void) arg; ind_auto = 0; ind_estado = 0; }
static void cmd_emergencia(const char *arg) { (void) arg; ind_auto = 0; ind_estado = 3; WebTerm_Print("PARO DE EMERGENCIA"); }
static void cmd_auto(const char *arg)       { (void) arg; ind_auto = 1; }
static void cmd_reset_mcu(const char *arg)  { (void) arg; WebTerm_Print("reiniciando..."); WebSys_RequestReset(); }

/* Configuracion de red: la web escribe net_* y pulsa "net_aplicar". Reemplaza WebNet_Apply() en tu codigo. */
__attribute__((weak)) void WebNet_Apply(void) { WebTerm_Print("net_aplicar: WebNet_Apply() no esta implementada"); }
static void cmd_net_aplicar(const char *arg) { (void) arg; WebNet_Apply(); }

/* Forzado de bus:  data-cmd="force" data-arg="bus,tipo,valor"   bus 0 CAN / 1 TRDP, tipo tx / rx / sg (señales), valor 1 / 0 */
static void cmd_force(const char *arg) {
  int bus = (arg[0] == '1') ? 1 : 0;
  const char *p = strchr(arg, ',');
  if (!p) return;
  p++;
  int tipo = (p[0] == 't') ? 1 : ((p[0] == 's') ? 2 : 0);
  const char *q = strchr(p, ',');
  if (!q) return;
  WebBus_Force(bus, tipo, q[1] == '1');
  WebTerm_Print(q[1] == '1' ? "forzado de bus ACTIVADO" : "forzado de bus desactivado");
}

/* Errores: err_ack (arg = ID, vacio = todos), err_clr_cnt, err_clr_hist, err_test (arg = ID: simula el error 8 s) */
static void cmd_err_ack(const char *arg)      { WebErr_Ack(atoi(arg)); }
static void cmd_err_clr_cnt(const char *arg)  { (void) arg; WebErr_ClearCounters(); WebTerm_Print("contadores de errores a cero"); }
static void cmd_err_clr_hist(const char *arg) { (void) arg; WebErr_ClearHistory(); WebTerm_Print("historico de errores borrado"); }
static void cmd_err_test(const char *arg)     { WebErr_Test(atoi(arg)); }

static const WebCmd_t web_cmds[] = {
  { "err_ack",      cmd_err_ack      },
  { "err_clr_cnt",  cmd_err_clr_cnt  },
  { "err_clr_hist", cmd_err_clr_hist },
  { "err_test",     cmd_err_test     },
  { "net_aplicar", cmd_net_aplicar },
  { "force",       cmd_force       },
  { "reset_cont", cmd_reset_cont },
  { "toggle_led", cmd_toggle_led },
  { "saludo",     cmd_saludo     },
  { "start",      cmd_start      },
  { "stop",       cmd_stop       },
  { "emergencia", cmd_emergencia },
  { "auto",       cmd_auto       },
  { "reset_mcu",  cmd_reset_mcu  },
};

/* ===========================================================================
 * 5) TERMINAL.  WEB -> PLACA:  esta funcion recibe cada linea que escribes en la web.
 *    PLACA -> WEB:  llama a WebTerm_Print("texto") desde donde quieras
 *    (por ejemplo cuando tu UART reciba una linea).
 * =========================================================================== */
void WebTerm_OnLine(const char *line) {
  /* Ejemplo: reenviar a tu UART (descomenta y adapta):
   *    MSG_Enviar(line);          // o HAL_UART_Transmit(&huart3, (uint8_t*)line, strlen(line), 100);
   */
  if (strcmp(line, "help") == 0) {
    WebTerm_Print("comandos: help, led, cont");
  } else if (strcmp(line, "led") == 0) {
    WebTerm_Print(demo_led ? "led = ON" : "led = OFF");
  } else if (strcmp(line, "cont") == 0) {
    char b[24] = "cont = "; size_t n = 7; char t[12]; int k = 0;
    uint32_t v = demo_contador;
    do { t[k++] = (char) ('0' + v % 10); v /= 10; } while (v);
    while (k) b[n++] = t[--k];
    b[n] = '\0';
    WebTerm_Print(b);
  } else {
    WebTerm_Print("no entendido (prueba: help)");
  }
}

/* ===========================================================================
 * 6) ARCHIVOS.
 *    SUBIDA (<div data-upload>):  se llama con el archivo completo en RAM.
 *    DESCARGA (<a href="/api/download?name=datos.csv">):  tu devuelves un buffer.
 *    Devuelve 0 si todo va bien. Elige tu mismo que nombres aceptas.
 * =========================================================================== */
static uint8_t up_buf[2048];  static size_t up_len = 0;  static char up_name[33] = "";

static size_t build_csv(char *buf, size_t size);   /* mas abajo */
static char   dl_buf[640];

int WebFile_Upload(const char *name, const uint8_t *data, size_t len, char *reply, size_t rsize) {
  /* Aqui guardarias en flash / tarjeta SD / buffer propio, o interpretarias el archivo
   * (por ejemplo una configuracion). Demo: guarda los 2 primeros KB en RAM. */
  size_t n = len < sizeof(up_buf) ? len : sizeof(up_buf);
  memcpy(up_buf, data, n);
  up_len = n;
  strncpy(up_name, name, sizeof(up_name) - 1);
  up_name[sizeof(up_name) - 1] = '\0';

  size_t i = 0; const char *t = "recibido ok: ";
  while (*t && i + 1 < rsize) reply[i++] = *t++;
  for (const char *p = up_name; *p && i + 1 < rsize; p++) reply[i++] = *p;
  reply[i] = '\0';
  return 0;
}

int WebFile_Download(const char *name, const uint8_t **data, size_t *len, const char **mime) {
  if (strcmp(name, "datos.csv") == 0) {                  /* todas las variables en CSV */
    *len = build_csv(dl_buf, sizeof(dl_buf));
    *data = (const uint8_t *) dl_buf;
    *mime = "text/csv";
    return *len ? 0 : -1;
  }
  if (strcmp(name, "subido.bin") == 0 && up_len > 0) {   /* devuelve lo ultimo subido */
    *data = up_buf; *len = up_len; *mime = "application/octet-stream";
    return 0;
  }
  return -1;                                             /* nombre no permitido */
}

/* ===========================================================================
 * A partir de aqui no hace falta tocar nada.
 * =========================================================================== */


/* ---- bloqueo para el terminal (varios hilos pueden llamar a WebTerm_Print) ---- */
#ifdef WEBVARS_USE_RTOS
static osMutexId_t term_mtx;
#define LOCK()   do { if (term_mtx) osMutexAcquire(term_mtx, osWaitForever); } while (0)
#define UNLOCK() do { if (term_mtx) osMutexRelease(term_mtx); } while (0)
#else
#define LOCK()   do {} while (0)
#define UNLOCK() do {} while (0)
#endif

/* ---- escritor con control de desbordamiento: si algo no cabe, ok pasa a 0 ---- */
typedef struct { char *buf; size_t size; size_t len; int ok; } Out_t;

static void out_ch(Out_t *o, char c) {
  if (!o->ok) return;
  if (o->len + 1 >= o->size) { o->ok = 0; return; }   /* deja sitio al '\0' */
  o->buf[o->len++] = c;
}
static void out_str(Out_t *o, const char *s) { while (*s) out_ch(o, *s++); }

/* Texto JSON entre comillas: escapa " \ y los caracteres de control */
static void out_jstr(Out_t *o, const char *s) {
  out_ch(o, '"');
  for (; *s; s++) {
    unsigned char c = (unsigned char) *s;
    if (c == '"' || c == '\\') { out_ch(o, '\\'); out_ch(o, (char) c); }
    else if (c >= 0x20)        { out_ch(o, (char) c); }
  }
  out_ch(o, '"');
}

static void out_u64(Out_t *o, unsigned long long v) {
  char t[24]; int n = 0;
  do { t[n++] = (char) ('0' + (v % 10)); v /= 10; } while (v);
  while (n) out_ch(o, t[--n]);
}
static void out_i64(Out_t *o, long long v) {
  if (v < 0) { out_ch(o, '-'); out_u64(o, (unsigned long long) (-(v + 1)) + 1ULL); }
  else out_u64(o, (unsigned long long) v);
}

/* float -> texto sin printf (newlib-nano no imprime floats) */
static void out_float(Out_t *o, float f, unsigned dec) {
  if (dec > 6) dec = 6;
  if (isnan(f) || isinf(f) || fabsf(f) > 1.0e9f) { out_str(o, "null"); return; }
  unsigned long long scale = 1;
  for (unsigned i = 0; i < dec; i++) scale *= 10;
  int neg = (f < 0.0f);
  double a = neg ? -(double) f : (double) f;
  unsigned long long q = (unsigned long long) (a * (double) scale + 0.5);
  if (neg && q != 0) out_ch(o, '-');                  /* evita "-0.0" */
  out_u64(o, q / scale);
  if (dec) {
    out_ch(o, '.');
    unsigned long long frac = q % scale;
    for (unsigned long long d = scale / 10; d; d /= 10) out_ch(o, (char) ('0' + (frac / d) % 10));
  }
}

static void out_value(Out_t *o, const WebVar_t *v) {
  switch (v->type) {
    case WV_U8:    out_u64(o, *(const volatile uint8_t  *) v->ptr); break;
    case WV_U16:   out_u64(o, *(const volatile uint16_t *) v->ptr); break;
    case WV_U32:   out_u64(o, *(const volatile uint32_t *) v->ptr); break;
    case WV_I32:   out_i64(o, *(const volatile int32_t  *) v->ptr); break;
    case WV_FLOAT: out_float(o, *(const volatile float  *) v->ptr, v->decimals); break;
    case WV_BOOL:  out_str(o, *(const volatile uint8_t  *) v->ptr ? "true" : "false"); break;
    case WV_STR: {                                      /* texto: se copia y se escapa */
      char t[40]; size_t n = 0, mx = v->decimals < sizeof(t) ? v->decimals : sizeof(t) - 1;
      const volatile char *p = (const volatile char *) v->ptr;
      while (n < mx && p[n]) { t[n] = p[n]; n++; }
      t[n] = '\0';
      out_jstr(o, t);
      break;
    }
  }
}

/* ---- API publica: variables ---- */
void WebVars_Init(void) {
#ifdef WEBVARS_USE_RTOS
  if (!term_mtx) term_mtx = osMutexNew(NULL);
#endif
}

uint16_t WebVars_PageFromUri(const char *uri, size_t len) {
  static const char pre[] = "/ws/";
  if (len <= sizeof(pre) - 1 || memcmp(uri, pre, sizeof(pre) - 1) != 0) return 0;
  uri += sizeof(pre) - 1;
  len -= sizeof(pre) - 1;
  for (size_t i = 0; i < sizeof(web_pages) / sizeof(web_pages[0]); i++) {
    if (strlen(web_pages[i].name) == len && memcmp(uri, web_pages[i].name, len) == 0)
      return web_pages[i].pg;
  }
  return 0;
}

size_t WebVars_BuildData(uint16_t page, char *buf, size_t size) {
  Out_t o = { buf, size, 0, size > 0 };
  int first = 1;
  out_ch(&o, '{');
  for (size_t i = 0; i < WEB_VARS_N; i++) {
    if (!(web_vars[i].pages & page)) continue;
    if (!first) out_ch(&o, ',');
    first = 0;
    out_jstr(&o, web_vars[i].key);
    out_ch(&o, ':');
    out_value(&o, &web_vars[i]);
  }
  out_ch(&o, '}');
  if (!o.ok || first) { if (size) buf[0] = '\0'; return 0; }
  buf[o.len] = '\0';
  return o.len;
}

static size_t build_csv(char *buf, size_t size) {
  Out_t o = { buf, size, 0, size > 0 };
  out_str(&o, "variable,valor\n");
  for (size_t i = 0; i < WEB_VARS_N; i++) {
    if (!(web_vars[i].pages & PG_SISTEMA)) continue;          /* CSV de las variables de la pagina Sistema */
    out_str(&o, web_vars[i].key);
    out_ch(&o, ',');
    out_value(&o, &web_vars[i]);
    out_ch(&o, '\n');
  }
  if (!o.ok) return 0;
  return o.len;
}

/* ---- escritura desde la web ---- */
static int parse_num(const char *s, double *out) {
  if (strcmp(s, "true") == 0 || strcmp(s, "on") == 0)  { *out = 1; return 1; }
  if (strcmp(s, "false") == 0 || strcmp(s, "off") == 0) { *out = 0; return 1; }
  char *end; double v = strtod(s, &end);
  if (end == s || *end != '\0' || isnan(v) || isinf(v)) return 0;
  *out = v; return 1;
}

/* Devuelve NULL si todo va bien, o el texto del error */
static const char *set_var(uint16_t page, const char *key, const char *val) {
  for (size_t i = 0; i < WEB_VARS_N; i++) {
    const WebVar_t *v = &web_vars[i];
    if (strcmp(v->key, key) != 0 || !(v->pages & page)) continue;
    if (!v->rw) return "variable de solo lectura";
    double x;
    if (!parse_num(val, &x)) return "valor no valido";
    if (v->min < v->max && (x < v->min || x > v->max)) return "fuera de rango";
    switch (v->type) {
      case WV_U8:    if (x < 0 || x > 255)        return "fuera de rango"; *(volatile uint8_t  *) v->ptr = (uint8_t)  x; break;
      case WV_U16:   if (x < 0 || x > 65535)      return "fuera de rango"; *(volatile uint16_t *) v->ptr = (uint16_t) x; break;
      case WV_U32:   if (x < 0 || x > 4294967295.0) return "fuera de rango"; *(volatile uint32_t *) v->ptr = (uint32_t) x; break;
      case WV_I32:   if (x < -2147483648.0 || x > 2147483647.0) return "fuera de rango"; *(volatile int32_t *) v->ptr = (int32_t) x; break;
      case WV_FLOAT: *(volatile float   *) v->ptr = (float) x; break;
      case WV_BOOL:  *(volatile uint8_t *) v->ptr = (x != 0.0); break;
      case WV_STR:   return "variable de texto, solo lectura";
    }
    return NULL;
  }
  return "variable desconocida";
}

static size_t reply_msg(char *reply, size_t rsize, int ok, const char *txt) {
  Out_t o = { reply, rsize, 0, rsize > 0 };
  out_str(&o, "{\"_r\":");
  char t[80]; size_t n = 0;
  if (!ok) { const char *e = "error: "; while (*e) t[n++] = *e++; }
  for (const char *p = txt; *p && n + 1 < sizeof(t); p++) t[n++] = *p;
  t[n] = '\0';
  out_jstr(&o, t);
  out_ch(&o, '}');
  if (!o.ok) { if (rsize) reply[0] = '\0'; return 0; }
  reply[o.len] = '\0';
  return o.len;
}

size_t WebVars_OnMessage(uint16_t page, const char *msg, size_t len, char *reply, size_t rsize) {
  char b[128];
  if (len < 3 || len >= sizeof(b) || msg[1] != '|') return 0;
  memcpy(b, msg, len); b[len] = '\0';
  char kind = b[0];
  char *p1 = b + 2;

  if (kind == 'S') {                                   /* S|id|valor */
    char *p2 = strchr(p1, '|');
    if (!p2) return reply_msg(reply, rsize, 0, "formato");
    *p2++ = '\0';
    const char *e = set_var(page, p1, p2);
    return e ? reply_msg(reply, rsize, 0, e) : reply_msg(reply, rsize, 1, "ok");
  }
  if (kind == 'C') {                                   /* C|nombre|arg */
    char *arg = strchr(p1, '|');
    if (arg) *arg++ = '\0'; else arg = (char *) "";
    for (size_t i = 0; i < sizeof(web_cmds) / sizeof(web_cmds[0]); i++) {
      if (strcmp(web_cmds[i].name, p1) == 0) { web_cmds[i].fn(arg); return reply_msg(reply, rsize, 1, p1); }
    }
    return reply_msg(reply, rsize, 0, "comando desconocido");
  }
  if (kind == 'T' && (TERM_PAGES & page)) {            /* T|linea */
    WebTerm_OnLine(p1);
    return 0;
  }
  return 0;
}

/* ---- terminal: cola circular de lineas ---- */
#define TERM_LINES  8
#define TERM_LEN    96
static char     term_q[TERM_LINES][TERM_LEN];
static unsigned term_head = 0, term_tail = 0;          /* head = siguiente a escribir */

void WebTerm_Print(const char *line) {
  LOCK();
  if (((term_head + 1) % TERM_LINES) != term_tail) {   /* si esta llena se descarta */
    strncpy(term_q[term_head], line, TERM_LEN - 1);
    term_q[term_head][TERM_LEN - 1] = '\0';
    term_head = (term_head + 1) % TERM_LINES;
  }
  UNLOCK();
}

size_t WebTerm_PopJson(char *out, size_t size) {
  char line[TERM_LEN];
  LOCK();
  if (term_head == term_tail) { UNLOCK(); return 0; }
  memcpy(line, term_q[term_tail], TERM_LEN);
  term_tail = (term_tail + 1) % TERM_LINES;
  UNLOCK();

  Out_t o = { out, size, 0, size > 0 };
  out_str(&o, "{\"_t\":");
  out_jstr(&o, line);
  out_ch(&o, '}');
  if (!o.ok) { if (size) out[0] = '\0'; return 0; }
  out[o.len] = '\0';
  return o.len;
}



