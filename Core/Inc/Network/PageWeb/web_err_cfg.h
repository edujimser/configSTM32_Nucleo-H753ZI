/* =============================================================================
 * web_err_cfg.h  -  LISTADO DE ERRORES (el unico archivo que editas para añadir/quitar errores)
 *
 * Cada linea  X( ID, NOMBRE, SEVERIDAD, RETARDO_s, CONDICION, "VARIABLES", "DESCRIPCION", VALOR )
 *
 *   ID          numero unico del error (el que ve el usuario): 101, 202...
 *   NOMBRE      nombre corto SIN comillas ni espacios (identificador C): AP1_C1
 *   SEVERIDAD   ERR_INFO, ERR_AVISO, ERR_ALARMA o ERR_CRITICO
 *   RETARDO_s   segundos que la condicion debe mantenerse para que el error se active (0 = en el acto)
 *   CONDICION   expresion C entre parentesis, verdadera (!=0) cuando hay error. Usa tus variables.
 *   "VARIABLES" texto con los IDs de variable web que la activan (separados por espacios).
 *               Si la variable esta en la pagina "errores" (PG_ERR) se muestra su valor en vivo.
 *   "DESCRIPCION" texto para el usuario
 *   VALOR       expresion numerica que se guarda en el historico al activarse el error (0 si no hace falta)
 *
 *  - Maximo WEBERR_MAX (32) errores. Cada linea acaba con  \  excepto la ultima.
 *  - Las condiciones se evaluan 1 vez por segundo en el hilo de red: que sean expresiones rapidas.
 * ============================================================================= */
#ifndef WEB_ERR_CFG_H
#define WEB_ERR_CFG_H

/* Cabeceras con las variables que usan las condiciones (cambia por las tuyas) */
#include "web_demo.h"
#include "web_sys.h"
#include "web_bus.h"

/* Fuente de hora (segundos UNIX) para fechar los errores */
#define WEBERR_NOW()  (rtc_epoch)

#define WEBERR_LIST(X) \
  X( 101, AP1_C1,    ERR_ALARMA,  0, (al_w & 0x0001u),               "al_w c1_hp",   "Alta presion circuito 1 (presostato disparado)",   c_hp[0]) \
  X( 102, BP1_C1,    ERR_ALARMA,  0, (al_w & 0x0002u),               "al_w c1_lp",   "Baja presion circuito 1",                          c_lp[0]) \
  X( 103, AP1_C2,    ERR_ALARMA,  0, (al_w & 0x0004u),               "al_w c2_hp",   "Alta presion circuito 2 (presostato disparado)",   c_hp[1]) \
  X( 104, BP1_C2,    ERR_ALARMA,  0, (al_w & 0x0008u),               "al_w c2_lp",   "Baja presion circuito 2",                          c_lp[1]) \
  X( 107, SONDA,     ERR_AVISO,   0, (al_w & 0x0040u),               "al_w t_ret",   "Sonda de temperatura con fallo (abierta o en corto)", 0) \
  X( 108, FILTRO,    ERR_AVISO,   5, (dp_filt > 250u),               "dp_filt",      "Filtro de aire sucio (presion diferencial alta)",  dp_filt) \
  X( 112, DESEQ,     ERR_ALARMA,  3, (desq > 6.0f),                  "desq",         "Desequilibrio de fases elevado",                   desq) \
  X( 113, CO2_ALTO,  ERR_AVISO,  10, (co2 > 1400u),                  "co2",          "CO2 interior por encima del limite",               co2) \
  X( 114, TDESC_C1,  ERR_CRITICO, 0, (c_td[0] > 105.0f),             "c1_td",        "Temperatura de descarga del compresor 1 excesiva", c_td[0]) \
  X( 120, T_INT_ALTA,ERR_AVISO,  20, (t_int > 30.0f),                "t_int sp_t",   "Temperatura interior demasiado alta",              t_int) \
  X( 201, CPU_ALTA,  ERR_AVISO,   5, (sys_cpu > 90u),                "s_cpu",        "Carga de CPU del STM32 por encima del 90 %",       sys_cpu) \
  X( 202, HEAP_ALTO, ERR_AVISO,   5, (sys_heap_pct > 85u),           "s_hpct",       "Heap de FreeRTOS casi lleno",                      sys_heap_pct) \
  X( 301, CAN_BUS,   ERR_ALARMA,  3, (web_bus_st[0].estado != 2),    "cb_st",        "Bus CAN sin comunicacion o con errores",           web_bus_st[0].estado) \
  X( 302, TRDP_BUS,  ERR_ALARMA,  3, (web_bus_st[1].estado != 2),    "db_st",        "Red TRDP sin comunicacion",                        web_bus_st[1].estado)

#endif
