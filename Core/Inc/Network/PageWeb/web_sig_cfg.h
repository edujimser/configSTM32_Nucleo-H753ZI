/* =============================================================================
 * web_sig_cfg.h  -  SEÑALES DE COMUNICACION (CAN / TRDP): el unico archivo que editas
 *
 * Una señal es un valor dentro de una trama (como en un DBC). La web la muestra en
 * valor fisico y permite FORZARLA de forma individual (casilla FORZAR + valor).
 *
 *  RECEPCION   X( NOMBRE, ID_TRAMA, BIT_INICIO, LONGITUD, CON_SIGNO, FACTOR, OFFSET, "UNIDAD", "DESCRIPCION", ASIGNAR )
 *              ASIGNAR = sentencia que guarda el valor fisico en tu variable; el valor se llama v:
 *              por ejemplo   mi_temp = v   o   mi_modo = (uint8_t) v
 *  TRANSMISION X( NOMBRE, ID_TRAMA, BIT_INICIO, LONGITUD, CON_SIGNO, FACTOR, OFFSET, "UNIDAD", "DESCRIPCION", LEER )
 *              LEER = expresion con el valor fisico que se envia (tu variable)
 *  TRAMAS TX   X( ID_TRAMA, DLC, PERIODO_ms )   las tramas que se transmiten solas, con las señales TX dentro
 *
 *   NOMBRE      identificador C unico (sin comillas ni espacios). ID_TRAMA: id CAN o ComId TRDP.
 *   BIT_INICIO  numeracion Intel: bit 0 = bit menos significativo del byte 0.   LONGITUD 1 a 32 bits.
 *   valor fisico = raw * FACTOR + OFFSET.      Maximo 16 señales (RX + TX) por bus.
 *   Cada linea acaba con  \  excepto la ultima de cada lista (las listas pueden estar vacias).
 *
 *  "Forzar" solo actua si el boton FORZAR SEÑALES del bus esta activo (se libera solo a los 10 min):
 *    RX forzada: tu variable recibe el valor forzado aunque el bus diga otra cosa.
 *    TX forzada: se transmite el valor forzado en vez de tu variable.
 * ============================================================================= */
#ifndef WEB_SIG_CFG_H
#define WEB_SIG_CFG_H

#include "web_demo.h"          /* variables que usan las señales (cambia por las tuyas) */

/* ------------------------------ CAN ------------------------------ */
#define WEBSIG_CAN_RX(X) \
  X( CAN_T_INT,   0x100u,  0, 16, 0, 0.1f,  0.0f, "C",   "Temperatura interior",      sg_can_rx[0] = v) \
  X( CAN_CONT,    0x100u, 32, 32, 0, 1.0f,  0.0f, "",    "Contador de la trama",      sg_can_rx[1] = v) \
  X( CAN_HP1,     0x1A5u,  0, 16, 0, 0.01f, 0.0f, "bar", "Alta presion C1",           sg_can_rx[2] = v) \
  X( CAN_LP1,     0x1A5u, 16, 16, 0, 0.01f, 0.0f, "bar", "Baja presion C1",           sg_can_rx[3] = v) \
  X( CAN_MODO,    0x1A5u, 32,  8, 0, 1.0f,  0.0f, "",    "Modo real de la unidad",    sg_can_rx[4] = v) \
  X( CAN_KW,      0x3A0u,  0, 16, 0, 0.1f,  0.0f, "kW",  "Potencia electrica",        sg_can_rx[5] = v)

#define WEBSIG_CAN_TX(X) \
  X( CAN_SP,      0x200u,  0, 16, 0, 0.1f,  0.0f, "C",   "Consigna de temperatura",   sp_t) \
  X( CAN_T_INT_TX,0x200u, 16, 16, 0, 0.1f,  0.0f, "C",   "Temperatura interior",      t_int) \
  X( CAN_MODO_TX, 0x200u, 32,  8, 0, 1.0f,  0.0f, "",    "Modo ordenado",             hv_modo) \
  X( CAN_CO2_TX,  0x210u,  0, 16, 0, 1.0f,  0.0f, "ppm", "CO2 interior",              co2)

#define WEBSIG_CAN_FRAMES(X) \
  X( 0x200u, 8,  500) \
  X( 0x210u, 2, 1000)

/* ------------------------------ TRDP ------------------------------ */
#define WEBSIG_TRDP_RX(X) \
  X( TD_T_INT,    1001u,   0, 16, 0, 0.1f,  0.0f, "C",   "Temperatura interior",      sg_trdp_rx[0] = v) \
  X( TD_SP,       1001u,  16, 16, 0, 0.1f,  0.0f, "C",   "Consigna",                  sg_trdp_rx[1] = v) \
  X( TD_KW,       1010u,   0, 16, 0, 0.1f,  0.0f, "kW",  "Potencia electrica",        sg_trdp_rx[2] = v)

#define WEBSIG_TRDP_TX(X) \
  X( TD_T_INT_TX, 2001u,   0, 16, 0, 0.1f,  0.0f, "C",   "Temperatura interior",      t_int) \
  X( TD_SP_TX,    2001u,  16, 16, 0, 0.1f,  0.0f, "C",   "Consigna",                  sp_t) \
  X( TD_HP_TX,    2001u,  32, 16, 0, 0.01f, 0.0f, "bar", "Alta presion C1",           c_hp[0])

#define WEBSIG_TRDP_FRAMES(X) \
  X( 2001u, 8, 1000)

#endif
