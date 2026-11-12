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

void WebTerm_OnLine(const char *line);   /* la defines tu en la seccion 5 */

#ifndef WEBVARS_HOST_TEST            /* en el PC (pruebas) no hay RTOS */
#include "cmsis_os2.h"
#define WEBVARS_USE_RTOS 1
#endif

/* ===========================================================================
 * 1) VARIABLES.  Aqui van tus variables propias (extern float mi_temperatura;).
 *    Las de demostracion estan en web_demo.c y las del sistema en web_sys.c.
 * =========================================================================== */

/* ===========================================================================
 * 2) TABLA DE VARIABLES.
 *    Solo lectura:  WV (ID, paginas, tipo, decimales, variable)
 *    Escribible:    WVW(ID, paginas, tipo, decimales, variable, minimo, maximo)
 *    Texto:         WV (ID, paginas, WV_STR, tamano_del_array, array_de_char)
 *    La web solo puede escribir variables WVW, y solo desde las paginas listadas.
 * =========================================================================== */
#define WV(k, pg, t, d, v)         { k, pg, t, d, (const volatile void *) &(v), 0, 0, 0 }
#define WVW(k, pg, t, d, v, a, b)  { k, pg, t, d, (const volatile void *) &(v), 1, a, b }

static const WebVar_t web_vars[] = {
  WV ("cont",      PG_DATOS | PG_SISTEMA,                         WV_U32,   0, demo_contador),
  WV ("temp",      PG_DATOS | PG_SISTEMA | PG_WIDGETS | PG_WDATOS, WV_FLOAT, 1, demo_temp),
  WV ("led",       PG_DATOS | PG_SISTEMA | PG_WIDGETS | PG_WDATOS, WV_BOOL,  0, demo_led),
  WV ("contador2", PG_CONFIG,                                     WV_U32,   0, demo_contador2),
  WV ("nivel",     PG_SISTEMA | PG_WIDGETS | PG_INDUSTRIA | PG_WDATOS, WV_U8,    0, demo_nivel),
  WV ("roll",      PG_WIDGETS,                                    WV_FLOAT, 1, demo_roll),
  WV ("pitch",     PG_WIDGETS,                                    WV_FLOAT, 1, demo_pitch),
  WV ("yaw",       PG_WIDGETS | PG_WDATOS,                        WV_FLOAT, 1, demo_yaw),
  WV ("rpm",       PG_WIDGETS | PG_WDATOS,                        WV_U16,   0, demo_rpm),
  WVW("setpoint",  PG_WIDGETS | PG_SISTEMA,                       WV_FLOAT, 1, demo_setpoint, 10, 35),
  WVW("modo",      PG_WIDGETS,                                    WV_U8,    0, demo_modo,     0, 2),
  WVW("enable",    PG_WIDGETS,                                    WV_BOOL,  0, demo_enable,   0, 1),
  WVW("brillo",    PG_WIDGETS,                                    WV_U8,    0, demo_brillo,   0, 100),
  WV ("m1_run",    PG_INDUSTRIA,                                  WV_BOOL,  0, ind_run[0]),
  WV ("m2_run",    PG_INDUSTRIA,                                  WV_BOOL,  0, ind_run[1]),
  WV ("m3_run",    PG_INDUSTRIA,                                  WV_BOOL,  0, ind_run[2]),
  WV ("m1_rpm",    PG_INDUSTRIA,                                  WV_U16,   0, ind_rpm[0]),
  WV ("m2_rpm",    PG_INDUSTRIA,                                  WV_U16,   0, ind_rpm[1]),
  WV ("m3_rpm",    PG_INDUSTRIA,                                  WV_U16,   0, ind_rpm[2]),
  WV ("m1_t",      PG_INDUSTRIA,                                  WV_FLOAT, 1, ind_temp[0]),
  WV ("m2_t",      PG_INDUSTRIA,                                  WV_FLOAT, 1, ind_temp[1]),
  WV ("m3_t",      PG_INDUSTRIA,                                  WV_FLOAT, 1, ind_temp[2]),
  WV ("m1_a",      PG_INDUSTRIA,                                  WV_FLOAT, 1, ind_amp[0]),
  WV ("m2_a",      PG_INDUSTRIA,                                  WV_FLOAT, 1, ind_amp[1]),
  WV ("m3_a",      PG_INDUSTRIA,                                  WV_FLOAT, 1, ind_amp[2]),
  WV ("estado",    PG_INDUSTRIA,                                  WV_U8,    0, ind_estado),
  WV ("entradas",  PG_INDUSTRIA,                                  WV_U8,    0, ind_entradas),
  WV ("salidas",   PG_INDUSTRIA,                                  WV_U8,    0, ind_salidas),
  WV ("presion",   PG_INDUSTRIA,                                  WV_FLOAT, 1, ind_presion),
  WV ("caudal",    PG_INDUSTRIA | PG_OBJETO,                      WV_FLOAT, 1, ind_caudal),
  WV ("piezas",    PG_INDUSTRIA,                                  WV_U32,   0, ind_piezas),
  WV ("uptime",    PG_INDUSTRIA,                                  WV_U32,   0, ind_uptime),
  WV ("valvula",   PG_INDUSTRIA,                                  WV_BOOL,  0, ind_valvula),
  WVW("hmodo",     PG_HVAC | PG_OBJETO,                           WV_U8,    0, hv_modo,       0, 4),
  WV ("hreal",     PG_HVAC | PG_OBJETO,                           WV_U8,    0, hv_real),
  WVW("sp_t",      PG_HVAC | PG_OBJETO | PG_ERR,                  WV_FLOAT, 1, sp_t,          10, 30),
  WV ("t_int",     PG_HVAC | PG_COMPON | PG_OBJETO | PG_WDATOS | PG_ERR, WV_FLOAT, 1, t_int),
  WV ("t_ext",     PG_HVAC | PG_COMPON | PG_OBJETO | PG_WDATOS,   WV_FLOAT, 1, t_ext),
  WV ("t_imp",     PG_HVAC | PG_OBJETO,                           WV_FLOAT, 1, t_imp),
  WV ("t_ret",     PG_HVAC | PG_OBJETO | PG_ERR,                  WV_FLOAT, 1, t_ret),
  WV ("hum",       PG_HVAC | PG_WDATOS,                           WV_U8,    0, hum),
  WV ("co2",       PG_HVAC | PG_WDATOS | PG_ERR,                  WV_U16,   0, co2),
  WV ("pid_out",   PG_HVAC | PG_OBJETO | PG_WDATOS,               WV_FLOAT, 1, pid_out),
  WVW("pid_mode",  PG_HVAC,                                       WV_U8,    0, pid_mode,      0, 1),
  WVW("pid_man",   PG_HVAC,                                       WV_FLOAT, 1, pid_man,       0, 100),
  WVW("pid_kp",    PG_HVAC,                                       WV_FLOAT, 2, pid_kp,        0, 50),
  WVW("pid_ki",    PG_HVAC,                                       WV_FLOAT, 2, pid_ki,        0, 10),
  WVW("pid_kd",    PG_HVAC,                                       WV_FLOAT, 2, pid_kd,        0, 10),
  WV ("c1_st",     PG_HVAC | PG_COMPON | PG_OBJETO,               WV_U8,    0, c_st[0]),
  WV ("c1_hz",     PG_HVAC | PG_OBJETO,                           WV_U8,    0, c1_hz),
  WV ("c1_a",      PG_HVAC | PG_COMPON | PG_OBJETO,               WV_FLOAT, 1, c_a[0]),
  WV ("c1_hp",     PG_HVAC | PG_OBJETO | PG_WDATOS | PG_ERR,      WV_FLOAT, 1, c_hp[0]),
  WV ("c1_lp",     PG_HVAC | PG_OBJETO | PG_ERR,                  WV_FLOAT, 1, c_lp[0]),
  WV ("c1_td",     PG_HVAC | PG_OBJETO | PG_ERR,                  WV_FLOAT, 1, c_td[0]),
  WV ("c1_ts",     PG_HVAC | PG_OBJETO,                           WV_FLOAT, 1, c_ts[0]),
  WV ("c1_eev",    PG_HVAC | PG_OBJETO,                           WV_U8,    0, c_eev[0]),
  WV ("c1_sh",     PG_HVAC | PG_OBJETO,                           WV_FLOAT, 1, c_sh[0]),
  WV ("c1_sc",     PG_HVAC | PG_OBJETO,                           WV_FLOAT, 1, c_sc[0]),
  WV ("c1_fc",     PG_HVAC | PG_OBJETO,                           WV_U8,    0, c_fc[0]),
  WV ("c1_4v",     PG_HVAC | PG_OBJETO,                           WV_BOOL,  0, c_4v[0]),
  WV ("c1_hg",     PG_HVAC | PG_OBJETO,                           WV_BOOL,  0, c_hg[0]),
  WV ("c2_st",     PG_HVAC | PG_COMPON | PG_OBJETO,               WV_U8,    0, c_st[1]),
  WV ("c2_a",      PG_HVAC | PG_COMPON | PG_OBJETO,               WV_FLOAT, 1, c_a[1]),
  WV ("c2_hp",     PG_HVAC | PG_OBJETO | PG_ERR,                  WV_FLOAT, 1, c_hp[1]),
  WV ("c2_lp",     PG_HVAC | PG_OBJETO | PG_ERR,                  WV_FLOAT, 1, c_lp[1]),
  WV ("c2_td",     PG_HVAC,                                       WV_FLOAT, 1, c_td[1]),
  WV ("c2_ts",     PG_HVAC,                                       WV_FLOAT, 1, c_ts[1]),
  WV ("c2_eev",    PG_HVAC,                                       WV_U8,    0, c_eev[1]),
  WV ("c2_sh",     PG_HVAC,                                       WV_FLOAT, 1, c_sh[1]),
  WV ("c2_sc",     PG_HVAC,                                       WV_FLOAT, 1, c_sc[1]),
  WV ("c2_fc",     PG_HVAC,                                       WV_U8,    0, c_fc[1]),
  WV ("c2_4v",     PG_HVAC,                                       WV_BOOL,  0, c_4v[1]),
  WV ("c2_hg",     PG_HVAC,                                       WV_BOOL,  0, c_hg[1]),
  WV ("c2_yd",     PG_HVAC | PG_COMPON | PG_OBJETO,               WV_U8,    0, c2_yd),
  WV ("c2_tyd",    PG_HVAC,                                       WV_U8,    0, c2_tyd),
  WV ("out_w",     PG_HVAC | PG_COMPON,                           WV_U16,   0, out_w),
  WV ("in_w",      PG_HVAC | PG_COMPON | PG_OBJETO,               WV_U8,    0, in_w),
  WV ("al_w",      PG_HVAC | PG_COMPON | PG_OBJETO | PG_ERR,      WV_U16,   0, al_w),
  WV ("ev_fan",    PG_HVAC | PG_COMPON | PG_OBJETO,               WV_U8,    0, ev_fan),
  WV ("dp_filt",   PG_HVAC | PG_COMPON | PG_OBJETO | PG_ERR,      WV_U16,   0, dp_filt),
  WV ("caudal",    PG_HVAC | PG_COMPON | PG_OBJETO,               WV_U16,   0, caudal_aire),
  WVW("m_c1",      PG_COMPON,                                     WV_U8,    0, m_c1,          0, 2),
  WVW("m_c2",      PG_COMPON,                                     WV_U8,    0, m_c2,          0, 2),
  WVW("m_ev",      PG_COMPON,                                     WV_U8,    0, m_ev,          0, 2),
  WVW("m_cd",      PG_COMPON,                                     WV_U8,    0, m_cd,          0, 2),
  WVW("m_hg",      PG_COMPON,                                     WV_U8,    0, m_hg,          0, 2),
  WVW("m_4v",      PG_COMPON,                                     WV_U8,    0, m_4v,          0, 2),
  WVW("m_cv",      PG_COMPON,                                     WV_U8,    0, m_cv,          0, 2),
  WVW("m_fl",      PG_COMPON,                                     WV_U8,    0, m_fl,          0, 2),
  WV ("i_l1",      PG_COMPON,                                     WV_FLOAT, 1, i_l[0]),
  WV ("i_l2",      PG_COMPON,                                     WV_FLOAT, 1, i_l[1]),
  WV ("i_l3",      PG_COMPON,                                     WV_FLOAT, 1, i_l[2]),
  WV ("v_ll",      PG_COMPON,                                     WV_U16,   0, v_ll),
  WV ("kw",        PG_COMPON | PG_OBJETO | PG_WDATOS,             WV_FLOAT, 1, kw),
  WV ("kwh",       PG_COMPON,                                     WV_U32,   0, wh),
  WV ("desq",      PG_COMPON | PG_ERR,                            WV_FLOAT, 1, desq),
  WV ("fl_fre",    PG_COMPON | PG_OBJETO,                         WV_U8,    0, fl_fre),
  WVW("fl_fre_sp", PG_COMPON,                                     WV_U8,    0, fl_fre_sp,     0, 100),
  WV ("fl_ret",    PG_COMPON | PG_OBJETO,                         WV_U8,    0, fl_ret),
  WVW("fl_ret_sp", PG_COMPON,                                     WV_U8,    0, fl_ret_sp,     0, 100),
  WV ("cv_a",      PG_COMPON,                                     WV_FLOAT, 1, cv_a),
  WV ("ev_a",      PG_COMPON,                                     WV_FLOAT, 1, ev_a),
  WV ("s_cpu",     PG_INTERNO | PG_WDATOS | PG_ERR,               WV_U8,    0, sys_cpu),
  WV ("s_stats",   PG_INTERNO,                                    WV_BOOL,  0, sys_stats_ok),
  WV ("s_hpct",    PG_INTERNO | PG_WDATOS | PG_ERR,               WV_U8,    0, sys_heap_pct),
  WV ("s_htot",    PG_INTERNO,                                    WV_U32,   0, sys_heap_tot),
  WV ("s_hfree",   PG_INTERNO,                                    WV_U32,   0, sys_heap_free),
  WV ("s_hmin",    PG_INTERNO,                                    WV_U32,   0, sys_heap_min),
  WV ("s_ram",     PG_INTERNO,                                    WV_U32,   0, sys_ram_stat),
  WV ("s_up",      PG_INTERNO | PG_OBJETO | PG_WDATOS,            WV_U32,   0, sys_up),
  WV ("s_nth",     PG_INTERNO,                                    WV_U8,    0, sys_threads),
  WV ("s_ws",      PG_INTERNO,                                    WV_U8,    0, sys_ws),
  WV ("t0_n",      PG_INTERNO,                                    WV_STR,   15, sys_th_n[0]),
  WV ("t0_s",      PG_INTERNO,                                    WV_U8,    0, sys_th_s[0]),
  WV ("t0_p",      PG_INTERNO,                                    WV_U8,    0, sys_th_p[0]),
  WV ("t0_c",      PG_INTERNO,                                    WV_U8,    0, sys_th_c[0]),
  WV ("t0_f",      PG_INTERNO,                                    WV_U16,   0, sys_th_f[0]),
  WV ("t1_n",      PG_INTERNO,                                    WV_STR,   15, sys_th_n[1]),
  WV ("t1_s",      PG_INTERNO,                                    WV_U8,    0, sys_th_s[1]),
  WV ("t1_p",      PG_INTERNO,                                    WV_U8,    0, sys_th_p[1]),
  WV ("t1_c",      PG_INTERNO,                                    WV_U8,    0, sys_th_c[1]),
  WV ("t1_f",      PG_INTERNO,                                    WV_U16,   0, sys_th_f[1]),
  WV ("t2_n",      PG_INTERNO,                                    WV_STR,   15, sys_th_n[2]),
  WV ("t2_s",      PG_INTERNO,                                    WV_U8,    0, sys_th_s[2]),
  WV ("t2_p",      PG_INTERNO,                                    WV_U8,    0, sys_th_p[2]),
  WV ("t2_c",      PG_INTERNO,                                    WV_U8,    0, sys_th_c[2]),
  WV ("t2_f",      PG_INTERNO,                                    WV_U16,   0, sys_th_f[2]),
  WV ("t3_n",      PG_INTERNO,                                    WV_STR,   15, sys_th_n[3]),
  WV ("t3_s",      PG_INTERNO,                                    WV_U8,    0, sys_th_s[3]),
  WV ("t3_p",      PG_INTERNO,                                    WV_U8,    0, sys_th_p[3]),
  WV ("t3_c",      PG_INTERNO,                                    WV_U8,    0, sys_th_c[3]),
  WV ("t3_f",      PG_INTERNO,                                    WV_U16,   0, sys_th_f[3]),
  WV ("t4_n",      PG_INTERNO,                                    WV_STR,   15, sys_th_n[4]),
  WV ("t4_s",      PG_INTERNO,                                    WV_U8,    0, sys_th_s[4]),
  WV ("t4_p",      PG_INTERNO,                                    WV_U8,    0, sys_th_p[4]),
  WV ("t4_c",      PG_INTERNO,                                    WV_U8,    0, sys_th_c[4]),
  WV ("t4_f",      PG_INTERNO,                                    WV_U16,   0, sys_th_f[4]),
  WV ("t5_n",      PG_INTERNO,                                    WV_STR,   15, sys_th_n[5]),
  WV ("t5_s",      PG_INTERNO,                                    WV_U8,    0, sys_th_s[5]),
  WV ("t5_p",      PG_INTERNO,                                    WV_U8,    0, sys_th_p[5]),
  WV ("t5_c",      PG_INTERNO,                                    WV_U8,    0, sys_th_c[5]),
  WV ("t5_f",      PG_INTERNO,                                    WV_U16,   0, sys_th_f[5]),
  WV ("t6_n",      PG_INTERNO,                                    WV_STR,   15, sys_th_n[6]),
  WV ("t6_s",      PG_INTERNO,                                    WV_U8,    0, sys_th_s[6]),
  WV ("t6_p",      PG_INTERNO,                                    WV_U8,    0, sys_th_p[6]),
  WV ("t6_c",      PG_INTERNO,                                    WV_U8,    0, sys_th_c[6]),
  WV ("t6_f",      PG_INTERNO,                                    WV_U16,   0, sys_th_f[6]),
  WV ("t7_n",      PG_INTERNO,                                    WV_STR,   15, sys_th_n[7]),
  WV ("t7_s",      PG_INTERNO,                                    WV_U8,    0, sys_th_s[7]),
  WV ("t7_p",      PG_INTERNO,                                    WV_U8,    0, sys_th_p[7]),
  WV ("t7_c",      PG_INTERNO,                                    WV_U8,    0, sys_th_c[7]),
  WV ("t7_f",      PG_INTERNO,                                    WV_U16,   0, sys_th_f[7]),
  WV ("t8_n",      PG_INTERNO,                                    WV_STR,   15, sys_th_n[8]),
  WV ("t8_s",      PG_INTERNO,                                    WV_U8,    0, sys_th_s[8]),
  WV ("t8_p",      PG_INTERNO,                                    WV_U8,    0, sys_th_p[8]),
  WV ("t8_c",      PG_INTERNO,                                    WV_U8,    0, sys_th_c[8]),
  WV ("t8_f",      PG_INTERNO,                                    WV_U16,   0, sys_th_f[8]),
  WV ("t9_n",      PG_INTERNO,                                    WV_STR,   15, sys_th_n[9]),
  WV ("t9_s",      PG_INTERNO,                                    WV_U8,    0, sys_th_s[9]),
  WV ("t9_p",      PG_INTERNO,                                    WV_U8,    0, sys_th_p[9]),
  WV ("t9_c",      PG_INTERNO,                                    WV_U8,    0, sys_th_c[9]),
  WV ("t9_f",      PG_INTERNO,                                    WV_U16,   0, sys_th_f[9]),
  WV ("t10_n",     PG_INTERNO,                                    WV_STR,   15, sys_th_n[10]),
  WV ("t10_s",     PG_INTERNO,                                    WV_U8,    0, sys_th_s[10]),
  WV ("t10_p",     PG_INTERNO,                                    WV_U8,    0, sys_th_p[10]),
  WV ("t10_c",     PG_INTERNO,                                    WV_U8,    0, sys_th_c[10]),
  WV ("t10_f",     PG_INTERNO,                                    WV_U16,   0, sys_th_f[10]),
  WV ("t11_n",     PG_INTERNO,                                    WV_STR,   15, sys_th_n[11]),
  WV ("t11_s",     PG_INTERNO,                                    WV_U8,    0, sys_th_s[11]),
  WV ("t11_p",     PG_INTERNO,                                    WV_U8,    0, sys_th_p[11]),
  WV ("t11_c",     PG_INTERNO,                                    WV_U8,    0, sys_th_c[11]),
  WV ("t11_f",     PG_INTERNO,                                    WV_U16,   0, sys_th_f[11]),
  WV ("dev_name",  PG_OBJETO,                                     WV_STR,   23, dev_name),
  WV ("dev_sn",    PG_OBJETO | PG_WDATOS,                         WV_STR,   15, dev_sn),
  WV ("dev_hw",    PG_OBJETO,                                     WV_STR,   11, dev_hw),
  WV ("dev_mac",   PG_OBJETO,                                     WV_STR,   17, dev_mac),
  WV ("f0_n",      PG_OBJETO | PG_WDATOS,                         WV_STR,   19, fw_n[0]),
  WV ("f0_v",      PG_OBJETO | PG_WDATOS,                         WV_STR,   15, fw_v[0]),
  WV ("f0_c",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_crc[0]),
  WV ("f0_z",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_size[0]),
  WV ("f0_k",      PG_OBJETO | PG_WDATOS,                         WV_BOOL,  0, fw_ok[0]),
  WV ("f1_n",      PG_OBJETO | PG_WDATOS,                         WV_STR,   19, fw_n[1]),
  WV ("f1_v",      PG_OBJETO | PG_WDATOS,                         WV_STR,   15, fw_v[1]),
  WV ("f1_c",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_crc[1]),
  WV ("f1_z",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_size[1]),
  WV ("f1_k",      PG_OBJETO | PG_WDATOS,                         WV_BOOL,  0, fw_ok[1]),
  WV ("f2_n",      PG_OBJETO | PG_WDATOS,                         WV_STR,   19, fw_n[2]),
  WV ("f2_v",      PG_OBJETO | PG_WDATOS,                         WV_STR,   15, fw_v[2]),
  WV ("f2_c",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_crc[2]),
  WV ("f2_z",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_size[2]),
  WV ("f2_k",      PG_OBJETO | PG_WDATOS,                         WV_BOOL,  0, fw_ok[2]),
  WV ("f3_n",      PG_OBJETO | PG_WDATOS,                         WV_STR,   19, fw_n[3]),
  WV ("f3_v",      PG_OBJETO | PG_WDATOS,                         WV_STR,   15, fw_v[3]),
  WV ("f3_c",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_crc[3]),
  WV ("f3_z",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_size[3]),
  WV ("f3_k",      PG_OBJETO | PG_WDATOS,                         WV_BOOL,  0, fw_ok[3]),
  WV ("f4_n",      PG_OBJETO | PG_WDATOS,                         WV_STR,   19, fw_n[4]),
  WV ("f4_v",      PG_OBJETO | PG_WDATOS,                         WV_STR,   15, fw_v[4]),
  WV ("f4_c",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_crc[4]),
  WV ("f4_z",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_size[4]),
  WV ("f4_k",      PG_OBJETO | PG_WDATOS,                         WV_BOOL,  0, fw_ok[4]),
  WV ("f5_n",      PG_OBJETO | PG_WDATOS,                         WV_STR,   19, fw_n[5]),
  WV ("f5_v",      PG_OBJETO | PG_WDATOS,                         WV_STR,   15, fw_v[5]),
  WV ("f5_c",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_crc[5]),
  WV ("f5_z",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_size[5]),
  WV ("f5_k",      PG_OBJETO | PG_WDATOS,                         WV_BOOL,  0, fw_ok[5]),
  WV ("f6_n",      PG_OBJETO | PG_WDATOS,                         WV_STR,   19, fw_n[6]),
  WV ("f6_v",      PG_OBJETO | PG_WDATOS,                         WV_STR,   15, fw_v[6]),
  WV ("f6_c",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_crc[6]),
  WV ("f6_z",      PG_OBJETO | PG_WDATOS,                         WV_U32,   0, fw_size[6]),
  WV ("f6_k",      PG_OBJETO | PG_WDATOS,                         WV_BOOL,  0, fw_ok[6]),
  WVW("rtc",       PG_OBJETO | PG_WDATOS,                         WV_U32,   0, rtc_epoch,     0, 4.29497e+09),
  WVW("rtc_tz",    PG_OBJETO | PG_WDATOS | PG_ERR,                WV_I32,   0, rtc_tz,        -720, 840),
  WVW("net_dhcp",  PG_OBJETO,                                     WV_BOOL,  0, net_dhcp,      0, 1),
  WVW("net_ip",    PG_OBJETO | PG_WDATOS,                         WV_U32,   0, net_ip,        0, 4.29497e+09),
  WVW("net_mask",  PG_OBJETO,                                     WV_U32,   0, net_mask,      0, 4.29497e+09),
  WVW("net_gw",    PG_OBJETO,                                     WV_U32,   0, net_gw,        0, 4.29497e+09),
  WVW("net_dns",   PG_OBJETO,                                     WV_U32,   0, net_dns,       0, 4.29497e+09),
  WVW("net_dto",   PG_OBJETO,                                     WV_U16,   0, net_dhcp_to,   1, 300),
  WVW("net_lease", PG_OBJETO,                                     WV_U32,   0, net_lease,     60, 604800),
  WV ("net_st",    PG_OBJETO | PG_WDATOS,                         WV_U8,    0, net_state),
  WV ("net_cip",   PG_OBJETO | PG_WDATOS,                         WV_U32,   0, net_cip),
  WV ("net_cmask", PG_OBJETO,                                     WV_U32,   0, net_cmask),
  WV ("net_cgw",   PG_OBJETO,                                     WV_U32,   0, net_cgw),
  WV ("net_cdns",  PG_OBJETO,                                     WV_U32,   0, net_cdns),
  WV ("net_left",  PG_OBJETO,                                     WV_U32,   0, net_left),
  WV ("net_t1",    PG_OBJETO,                                     WV_U32,   0, net_t1),
  WV ("net_t2",    PG_OBJETO,                                     WV_U32,   0, net_t2),
  WV ("net_srv",   PG_OBJETO,                                     WV_U32,   0, net_srv),
  WV ("bat",       PG_WDATOS,                                     WV_U8,    0, dem_bat),
  WV ("rssi",      PG_WDATOS,                                     WV_U8,    0, dem_rssi),
  WV ("bat_chg",   PG_WDATOS,                                     WV_BOOL,  0, dem_chg),
  WV ("cr0i",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][0].id),
  WV ("cr0l",      PG_CAN | PG_WDATOS,                            WV_U8,    0, web_bus_rx[0][0].len),
  WVW("cr0a",      PG_CAN | PG_WDATOS,                            WV_U32,   0, web_bus_rx[0][0].lo, 0, 4.29497e+09),
  WVW("cr0b",      PG_CAN | PG_WDATOS,                            WV_U32,   0, web_bus_rx[0][0].hi, 0, 4.29497e+09),
  WV ("cr0n",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][0].cnt),
  WV ("cr0h",      PG_CAN,                                        WV_U16,   0, web_bus_rx[0][0].hz),
  WV ("cr0t",      PG_CAN,                                        WV_U16,   0, web_bus_rx[0][0].age),
  WV ("cr1i",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][1].id),
  WV ("cr1l",      PG_CAN,                                        WV_U8,    0, web_bus_rx[0][1].len),
  WVW("cr1a",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][1].lo, 0, 4.29497e+09),
  WVW("cr1b",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][1].hi, 0, 4.29497e+09),
  WV ("cr1n",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][1].cnt),
  WV ("cr1h",      PG_CAN,                                        WV_U16,   0, web_bus_rx[0][1].hz),
  WV ("cr1t",      PG_CAN,                                        WV_U16,   0, web_bus_rx[0][1].age),
  WV ("cr2i",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][2].id),
  WV ("cr2l",      PG_CAN,                                        WV_U8,    0, web_bus_rx[0][2].len),
  WVW("cr2a",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][2].lo, 0, 4.29497e+09),
  WVW("cr2b",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][2].hi, 0, 4.29497e+09),
  WV ("cr2n",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][2].cnt),
  WV ("cr2h",      PG_CAN,                                        WV_U16,   0, web_bus_rx[0][2].hz),
  WV ("cr2t",      PG_CAN,                                        WV_U16,   0, web_bus_rx[0][2].age),
  WV ("cr3i",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][3].id),
  WV ("cr3l",      PG_CAN,                                        WV_U8,    0, web_bus_rx[0][3].len),
  WVW("cr3a",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][3].lo, 0, 4.29497e+09),
  WVW("cr3b",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][3].hi, 0, 4.29497e+09),
  WV ("cr3n",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][3].cnt),
  WV ("cr3h",      PG_CAN,                                        WV_U16,   0, web_bus_rx[0][3].hz),
  WV ("cr3t",      PG_CAN,                                        WV_U16,   0, web_bus_rx[0][3].age),
  WV ("cr4i",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][4].id),
  WV ("cr4l",      PG_CAN,                                        WV_U8,    0, web_bus_rx[0][4].len),
  WVW("cr4a",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][4].lo, 0, 4.29497e+09),
  WVW("cr4b",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][4].hi, 0, 4.29497e+09),
  WV ("cr4n",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][4].cnt),
  WV ("cr4h",      PG_CAN,                                        WV_U16,   0, web_bus_rx[0][4].hz),
  WV ("cr4t",      PG_CAN,                                        WV_U16,   0, web_bus_rx[0][4].age),
  WV ("cr5i",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][5].id),
  WV ("cr5l",      PG_CAN,                                        WV_U8,    0, web_bus_rx[0][5].len),
  WVW("cr5a",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][5].lo, 0, 4.29497e+09),
  WVW("cr5b",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][5].hi, 0, 4.29497e+09),
  WV ("cr5n",      PG_CAN,                                        WV_U32,   0, web_bus_rx[0][5].cnt),
  WV ("cr5h",      PG_CAN,                                        WV_U16,   0, web_bus_rx[0][5].hz),
  WV ("cr5t",      PG_CAN,                                        WV_U16,   0, web_bus_rx[0][5].age),
  WVW("cx0i",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][0].id, 0, 4.29497e+09),
  WVW("cx0l",      PG_CAN | PG_WDATOS,                            WV_U8,    0, web_bus_tx[0][0].len, 0, 8),
  WVW("cx0a",      PG_CAN | PG_WDATOS,                            WV_U32,   0, web_bus_tx[0][0].lo, 0, 4.29497e+09),
  WVW("cx0b",      PG_CAN | PG_WDATOS,                            WV_U32,   0, web_bus_tx[0][0].hi, 0, 4.29497e+09),
  WVW("cx0p",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][0].per, 0, 60000),
  WVW("cx0e",      PG_CAN,                                        WV_BOOL,  0, web_bus_tx[0][0].en, 0, 1),
  WVW("cx1i",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][1].id, 0, 4.29497e+09),
  WVW("cx1l",      PG_CAN,                                        WV_U8,    0, web_bus_tx[0][1].len, 0, 8),
  WVW("cx1a",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][1].lo, 0, 4.29497e+09),
  WVW("cx1b",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][1].hi, 0, 4.29497e+09),
  WVW("cx1p",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][1].per, 0, 60000),
  WVW("cx1e",      PG_CAN,                                        WV_BOOL,  0, web_bus_tx[0][1].en, 0, 1),
  WVW("cx2i",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][2].id, 0, 4.29497e+09),
  WVW("cx2l",      PG_CAN,                                        WV_U8,    0, web_bus_tx[0][2].len, 0, 8),
  WVW("cx2a",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][2].lo, 0, 4.29497e+09),
  WVW("cx2b",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][2].hi, 0, 4.29497e+09),
  WVW("cx2p",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][2].per, 0, 60000),
  WVW("cx2e",      PG_CAN,                                        WV_BOOL,  0, web_bus_tx[0][2].en, 0, 1),
  WVW("cx3i",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][3].id, 0, 4.29497e+09),
  WVW("cx3l",      PG_CAN,                                        WV_U8,    0, web_bus_tx[0][3].len, 0, 8),
  WVW("cx3a",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][3].lo, 0, 4.29497e+09),
  WVW("cx3b",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][3].hi, 0, 4.29497e+09),
  WVW("cx3p",      PG_CAN,                                        WV_U32,   0, web_bus_tx[0][3].per, 0, 60000),
  WVW("cx3e",      PG_CAN,                                        WV_BOOL,  0, web_bus_tx[0][3].en, 0, 1),
  WV ("cb_frx",    PG_CAN,                                        WV_BOOL,  0, web_bus_st[0].frx),
  WV ("cb_ftx",    PG_CAN,                                        WV_BOOL,  0, web_bus_st[0].ftx),
  WV ("cb_st",     PG_CAN | PG_ERR,                               WV_U8,    0, web_bus_st[0].estado),
  WV ("cb_load",   PG_CAN,                                        WV_U8,    0, web_bus_st[0].carga),
  WV ("cb_left",   PG_CAN,                                        WV_U16,   0, web_bus_st[0].left),
  WV ("cb_rxn",    PG_CAN,                                        WV_U32,   0, web_bus_st[0].rxn),
  WV ("cb_txn",    PG_CAN,                                        WV_U32,   0, web_bus_st[0].txn),
  WV ("cb_err",    PG_CAN,                                        WV_U32,   0, web_bus_st[0].err),
  WV ("cb_drop",   PG_CAN,                                        WV_U32,   0, web_bus_st[0].drop),
  WV ("dr0i",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][0].id),
  WV ("dr0l",      PG_TRDP,                                       WV_U8,    0, web_bus_rx[1][0].len),
  WVW("dr0a",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][0].lo, 0, 4.29497e+09),
  WVW("dr0b",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][0].hi, 0, 4.29497e+09),
  WV ("dr0n",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][0].cnt),
  WV ("dr0h",      PG_TRDP,                                       WV_U16,   0, web_bus_rx[1][0].hz),
  WV ("dr0t",      PG_TRDP,                                       WV_U16,   0, web_bus_rx[1][0].age),
  WV ("dr1i",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][1].id),
  WV ("dr1l",      PG_TRDP,                                       WV_U8,    0, web_bus_rx[1][1].len),
  WVW("dr1a",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][1].lo, 0, 4.29497e+09),
  WVW("dr1b",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][1].hi, 0, 4.29497e+09),
  WV ("dr1n",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][1].cnt),
  WV ("dr1h",      PG_TRDP,                                       WV_U16,   0, web_bus_rx[1][1].hz),
  WV ("dr1t",      PG_TRDP,                                       WV_U16,   0, web_bus_rx[1][1].age),
  WV ("dr2i",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][2].id),
  WV ("dr2l",      PG_TRDP,                                       WV_U8,    0, web_bus_rx[1][2].len),
  WVW("dr2a",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][2].lo, 0, 4.29497e+09),
  WVW("dr2b",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][2].hi, 0, 4.29497e+09),
  WV ("dr2n",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][2].cnt),
  WV ("dr2h",      PG_TRDP,                                       WV_U16,   0, web_bus_rx[1][2].hz),
  WV ("dr2t",      PG_TRDP,                                       WV_U16,   0, web_bus_rx[1][2].age),
  WV ("dr3i",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][3].id),
  WV ("dr3l",      PG_TRDP,                                       WV_U8,    0, web_bus_rx[1][3].len),
  WVW("dr3a",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][3].lo, 0, 4.29497e+09),
  WVW("dr3b",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][3].hi, 0, 4.29497e+09),
  WV ("dr3n",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][3].cnt),
  WV ("dr3h",      PG_TRDP,                                       WV_U16,   0, web_bus_rx[1][3].hz),
  WV ("dr3t",      PG_TRDP,                                       WV_U16,   0, web_bus_rx[1][3].age),
  WV ("dr4i",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][4].id),
  WV ("dr4l",      PG_TRDP,                                       WV_U8,    0, web_bus_rx[1][4].len),
  WVW("dr4a",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][4].lo, 0, 4.29497e+09),
  WVW("dr4b",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][4].hi, 0, 4.29497e+09),
  WV ("dr4n",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][4].cnt),
  WV ("dr4h",      PG_TRDP,                                       WV_U16,   0, web_bus_rx[1][4].hz),
  WV ("dr4t",      PG_TRDP,                                       WV_U16,   0, web_bus_rx[1][4].age),
  WV ("dr5i",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][5].id),
  WV ("dr5l",      PG_TRDP,                                       WV_U8,    0, web_bus_rx[1][5].len),
  WVW("dr5a",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][5].lo, 0, 4.29497e+09),
  WVW("dr5b",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][5].hi, 0, 4.29497e+09),
  WV ("dr5n",      PG_TRDP,                                       WV_U32,   0, web_bus_rx[1][5].cnt),
  WV ("dr5h",      PG_TRDP,                                       WV_U16,   0, web_bus_rx[1][5].hz),
  WV ("dr5t",      PG_TRDP,                                       WV_U16,   0, web_bus_rx[1][5].age),
  WVW("dx0i",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][0].id, 0, 4.29497e+09),
  WVW("dx0l",      PG_TRDP,                                       WV_U8,    0, web_bus_tx[1][0].len, 0, 8),
  WVW("dx0a",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][0].lo, 0, 4.29497e+09),
  WVW("dx0b",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][0].hi, 0, 4.29497e+09),
  WVW("dx0p",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][0].per, 0, 60000),
  WVW("dx0e",      PG_TRDP,                                       WV_BOOL,  0, web_bus_tx[1][0].en, 0, 1),
  WVW("dx1i",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][1].id, 0, 4.29497e+09),
  WVW("dx1l",      PG_TRDP,                                       WV_U8,    0, web_bus_tx[1][1].len, 0, 8),
  WVW("dx1a",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][1].lo, 0, 4.29497e+09),
  WVW("dx1b",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][1].hi, 0, 4.29497e+09),
  WVW("dx1p",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][1].per, 0, 60000),
  WVW("dx1e",      PG_TRDP,                                       WV_BOOL,  0, web_bus_tx[1][1].en, 0, 1),
  WVW("dx2i",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][2].id, 0, 4.29497e+09),
  WVW("dx2l",      PG_TRDP,                                       WV_U8,    0, web_bus_tx[1][2].len, 0, 8),
  WVW("dx2a",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][2].lo, 0, 4.29497e+09),
  WVW("dx2b",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][2].hi, 0, 4.29497e+09),
  WVW("dx2p",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][2].per, 0, 60000),
  WVW("dx2e",      PG_TRDP,                                       WV_BOOL,  0, web_bus_tx[1][2].en, 0, 1),
  WVW("dx3i",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][3].id, 0, 4.29497e+09),
  WVW("dx3l",      PG_TRDP,                                       WV_U8,    0, web_bus_tx[1][3].len, 0, 8),
  WVW("dx3a",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][3].lo, 0, 4.29497e+09),
  WVW("dx3b",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][3].hi, 0, 4.29497e+09),
  WVW("dx3p",      PG_TRDP,                                       WV_U32,   0, web_bus_tx[1][3].per, 0, 60000),
  WVW("dx3e",      PG_TRDP,                                       WV_BOOL,  0, web_bus_tx[1][3].en, 0, 1),
  WV ("db_frx",    PG_TRDP,                                       WV_BOOL,  0, web_bus_st[1].frx),
  WV ("db_ftx",    PG_TRDP,                                       WV_BOOL,  0, web_bus_st[1].ftx),
  WV ("db_st",     PG_TRDP | PG_ERR,                              WV_U8,    0, web_bus_st[1].estado),
  WV ("db_load",   PG_TRDP,                                       WV_U8,    0, web_bus_st[1].carga),
  WV ("db_left",   PG_TRDP,                                       WV_U16,   0, web_bus_st[1].left),
  WV ("db_rxn",    PG_TRDP,                                       WV_U32,   0, web_bus_st[1].rxn),
  WV ("db_txn",    PG_TRDP,                                       WV_U32,   0, web_bus_st[1].txn),
  WV ("db_err",    PG_TRDP,                                       WV_U32,   0, web_bus_st[1].err),
  WV ("db_drop",   PG_TRDP,                                       WV_U32,   0, web_bus_st[1].drop),
  WV ("gv0",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][0].val),
  WVW("gf0",       PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][0].fen, 0, 1),
  WVW("gx0",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][0].fval, -1e+09, 1e+09),
  WV ("ga0",       PG_CAN,                                        WV_U16,   0, web_sig_rt[0][0].age),
  WV ("gv1",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][1].val),
  WVW("gf1",       PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][1].fen, 0, 1),
  WVW("gx1",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][1].fval, -1e+09, 1e+09),
  WV ("ga1",       PG_CAN,                                        WV_U16,   0, web_sig_rt[0][1].age),
  WV ("gv2",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][2].val),
  WVW("gf2",       PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][2].fen, 0, 1),
  WVW("gx2",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][2].fval, -1e+09, 1e+09),
  WV ("ga2",       PG_CAN,                                        WV_U16,   0, web_sig_rt[0][2].age),
  WV ("gv3",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][3].val),
  WVW("gf3",       PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][3].fen, 0, 1),
  WVW("gx3",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][3].fval, -1e+09, 1e+09),
  WV ("ga3",       PG_CAN,                                        WV_U16,   0, web_sig_rt[0][3].age),
  WV ("gv4",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][4].val),
  WVW("gf4",       PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][4].fen, 0, 1),
  WVW("gx4",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][4].fval, -1e+09, 1e+09),
  WV ("ga4",       PG_CAN,                                        WV_U16,   0, web_sig_rt[0][4].age),
  WV ("gv5",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][5].val),
  WVW("gf5",       PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][5].fen, 0, 1),
  WVW("gx5",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][5].fval, -1e+09, 1e+09),
  WV ("ga5",       PG_CAN,                                        WV_U16,   0, web_sig_rt[0][5].age),
  WV ("gv6",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][6].val),
  WVW("gf6",       PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][6].fen, 0, 1),
  WVW("gx6",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][6].fval, -1e+09, 1e+09),
  WV ("ga6",       PG_CAN,                                        WV_U16,   0, web_sig_rt[0][6].age),
  WV ("gv7",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][7].val),
  WVW("gf7",       PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][7].fen, 0, 1),
  WVW("gx7",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][7].fval, -1e+09, 1e+09),
  WV ("ga7",       PG_CAN,                                        WV_U16,   0, web_sig_rt[0][7].age),
  WV ("gv8",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][8].val),
  WVW("gf8",       PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][8].fen, 0, 1),
  WVW("gx8",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][8].fval, -1e+09, 1e+09),
  WV ("ga8",       PG_CAN,                                        WV_U16,   0, web_sig_rt[0][8].age),
  WV ("gv9",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][9].val),
  WVW("gf9",       PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][9].fen, 0, 1),
  WVW("gx9",       PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][9].fval, -1e+09, 1e+09),
  WV ("ga9",       PG_CAN,                                        WV_U16,   0, web_sig_rt[0][9].age),
  WV ("gv10",      PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][10].val),
  WVW("gf10",      PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][10].fen, 0, 1),
  WVW("gx10",      PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][10].fval, -1e+09, 1e+09),
  WV ("ga10",      PG_CAN,                                        WV_U16,   0, web_sig_rt[0][10].age),
  WV ("gv11",      PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][11].val),
  WVW("gf11",      PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][11].fen, 0, 1),
  WVW("gx11",      PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][11].fval, -1e+09, 1e+09),
  WV ("ga11",      PG_CAN,                                        WV_U16,   0, web_sig_rt[0][11].age),
  WV ("gv12",      PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][12].val),
  WVW("gf12",      PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][12].fen, 0, 1),
  WVW("gx12",      PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][12].fval, -1e+09, 1e+09),
  WV ("ga12",      PG_CAN,                                        WV_U16,   0, web_sig_rt[0][12].age),
  WV ("gv13",      PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][13].val),
  WVW("gf13",      PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][13].fen, 0, 1),
  WVW("gx13",      PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][13].fval, -1e+09, 1e+09),
  WV ("ga13",      PG_CAN,                                        WV_U16,   0, web_sig_rt[0][13].age),
  WV ("gv14",      PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][14].val),
  WVW("gf14",      PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][14].fen, 0, 1),
  WVW("gx14",      PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][14].fval, -1e+09, 1e+09),
  WV ("ga14",      PG_CAN,                                        WV_U16,   0, web_sig_rt[0][14].age),
  WV ("gv15",      PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][15].val),
  WVW("gf15",      PG_CAN,                                        WV_BOOL,  0, web_sig_rt[0][15].fen, 0, 1),
  WVW("gx15",      PG_CAN,                                        WV_FLOAT, 3, web_sig_rt[0][15].fval, -1e+09, 1e+09),
  WV ("ga15",      PG_CAN,                                        WV_U16,   0, web_sig_rt[0][15].age),
  WV ("tv0",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][0].val),
  WVW("tf0",       PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][0].fen, 0, 1),
  WVW("tx0",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][0].fval, -1e+09, 1e+09),
  WV ("ta0",       PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][0].age),
  WV ("tv1",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][1].val),
  WVW("tf1",       PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][1].fen, 0, 1),
  WVW("tx1",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][1].fval, -1e+09, 1e+09),
  WV ("ta1",       PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][1].age),
  WV ("tv2",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][2].val),
  WVW("tf2",       PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][2].fen, 0, 1),
  WVW("tx2",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][2].fval, -1e+09, 1e+09),
  WV ("ta2",       PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][2].age),
  WV ("tv3",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][3].val),
  WVW("tf3",       PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][3].fen, 0, 1),
  WVW("tx3",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][3].fval, -1e+09, 1e+09),
  WV ("ta3",       PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][3].age),
  WV ("tv4",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][4].val),
  WVW("tf4",       PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][4].fen, 0, 1),
  WVW("tx4",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][4].fval, -1e+09, 1e+09),
  WV ("ta4",       PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][4].age),
  WV ("tv5",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][5].val),
  WVW("tf5",       PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][5].fen, 0, 1),
  WVW("tx5",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][5].fval, -1e+09, 1e+09),
  WV ("ta5",       PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][5].age),
  WV ("tv6",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][6].val),
  WVW("tf6",       PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][6].fen, 0, 1),
  WVW("tx6",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][6].fval, -1e+09, 1e+09),
  WV ("ta6",       PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][6].age),
  WV ("tv7",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][7].val),
  WVW("tf7",       PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][7].fen, 0, 1),
  WVW("tx7",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][7].fval, -1e+09, 1e+09),
  WV ("ta7",       PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][7].age),
  WV ("tv8",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][8].val),
  WVW("tf8",       PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][8].fen, 0, 1),
  WVW("tx8",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][8].fval, -1e+09, 1e+09),
  WV ("ta8",       PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][8].age),
  WV ("tv9",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][9].val),
  WVW("tf9",       PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][9].fen, 0, 1),
  WVW("tx9",       PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][9].fval, -1e+09, 1e+09),
  WV ("ta9",       PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][9].age),
  WV ("tv10",      PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][10].val),
  WVW("tf10",      PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][10].fen, 0, 1),
  WVW("tx10",      PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][10].fval, -1e+09, 1e+09),
  WV ("ta10",      PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][10].age),
  WV ("tv11",      PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][11].val),
  WVW("tf11",      PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][11].fen, 0, 1),
  WVW("tx11",      PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][11].fval, -1e+09, 1e+09),
  WV ("ta11",      PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][11].age),
  WV ("tv12",      PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][12].val),
  WVW("tf12",      PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][12].fen, 0, 1),
  WVW("tx12",      PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][12].fval, -1e+09, 1e+09),
  WV ("ta12",      PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][12].age),
  WV ("tv13",      PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][13].val),
  WVW("tf13",      PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][13].fen, 0, 1),
  WVW("tx13",      PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][13].fval, -1e+09, 1e+09),
  WV ("ta13",      PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][13].age),
  WV ("tv14",      PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][14].val),
  WVW("tf14",      PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][14].fen, 0, 1),
  WVW("tx14",      PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][14].fval, -1e+09, 1e+09),
  WV ("ta14",      PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][14].age),
  WV ("tv15",      PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][15].val),
  WVW("tf15",      PG_TRDP,                                       WV_BOOL,  0, web_sig_rt[1][15].fen, 0, 1),
  WVW("tx15",      PG_TRDP,                                       WV_FLOAT, 3, web_sig_rt[1][15].fval, -1e+09, 1e+09),
  WV ("ta15",      PG_TRDP,                                       WV_U16,   0, web_sig_rt[1][15].age),
  WV ("cb_fsg",    PG_CAN,                                        WV_BOOL,  0, web_bus_st[0].fsg),
  WV ("db_fsg",    PG_TRDP,                                       WV_BOOL,  0, web_bus_st[1].fsg),
  WV ("er_act",    PG_ERR,                                        WV_U32,   0, err_act),
  WV ("er_una",    PG_ERR,                                        WV_U32,   0, err_una),
  WV ("er_n",      PG_ERR,                                        WV_U8,    0, err_n),
  WV ("er_cfg",    PG_ERR,                                        WV_U8,    0, err_cfg),
  WV ("er_tot",    PG_ERR,                                        WV_U32,   0, err_tot),
  WV ("ec0",       PG_ERR,                                        WV_U16,   0, err_cnt[0]),
  WV ("el0",       PG_ERR,                                        WV_U32,   0, err_last[0]),
  WV ("ec1",       PG_ERR,                                        WV_U16,   0, err_cnt[1]),
  WV ("el1",       PG_ERR,                                        WV_U32,   0, err_last[1]),
  WV ("ec2",       PG_ERR,                                        WV_U16,   0, err_cnt[2]),
  WV ("el2",       PG_ERR,                                        WV_U32,   0, err_last[2]),
  WV ("ec3",       PG_ERR,                                        WV_U16,   0, err_cnt[3]),
  WV ("el3",       PG_ERR,                                        WV_U32,   0, err_last[3]),
  WV ("ec4",       PG_ERR,                                        WV_U16,   0, err_cnt[4]),
  WV ("el4",       PG_ERR,                                        WV_U32,   0, err_last[4]),
  WV ("ec5",       PG_ERR,                                        WV_U16,   0, err_cnt[5]),
  WV ("el5",       PG_ERR,                                        WV_U32,   0, err_last[5]),
  WV ("ec6",       PG_ERR,                                        WV_U16,   0, err_cnt[6]),
  WV ("el6",       PG_ERR,                                        WV_U32,   0, err_last[6]),
  WV ("ec7",       PG_ERR,                                        WV_U16,   0, err_cnt[7]),
  WV ("el7",       PG_ERR,                                        WV_U32,   0, err_last[7]),
  WV ("ec8",       PG_ERR,                                        WV_U16,   0, err_cnt[8]),
  WV ("el8",       PG_ERR,                                        WV_U32,   0, err_last[8]),
  WV ("ec9",       PG_ERR,                                        WV_U16,   0, err_cnt[9]),
  WV ("el9",       PG_ERR,                                        WV_U32,   0, err_last[9]),
  WV ("ec10",      PG_ERR,                                        WV_U16,   0, err_cnt[10]),
  WV ("el10",      PG_ERR,                                        WV_U32,   0, err_last[10]),
  WV ("ec11",      PG_ERR,                                        WV_U16,   0, err_cnt[11]),
  WV ("el11",      PG_ERR,                                        WV_U32,   0, err_last[11]),
  WV ("ec12",      PG_ERR,                                        WV_U16,   0, err_cnt[12]),
  WV ("el12",      PG_ERR,                                        WV_U32,   0, err_last[12]),
  WV ("ec13",      PG_ERR,                                        WV_U16,   0, err_cnt[13]),
  WV ("el13",      PG_ERR,                                        WV_U32,   0, err_last[13]),
  WV ("ec14",      PG_ERR,                                        WV_U16,   0, err_cnt[14]),
  WV ("el14",      PG_ERR,                                        WV_U32,   0, err_last[14]),
  WV ("ec15",      PG_ERR,                                        WV_U16,   0, err_cnt[15]),
  WV ("el15",      PG_ERR,                                        WV_U32,   0, err_last[15]),
  WV ("ec16",      PG_ERR,                                        WV_U16,   0, err_cnt[16]),
  WV ("el16",      PG_ERR,                                        WV_U32,   0, err_last[16]),
  WV ("ec17",      PG_ERR,                                        WV_U16,   0, err_cnt[17]),
  WV ("el17",      PG_ERR,                                        WV_U32,   0, err_last[17]),
  WV ("ec18",      PG_ERR,                                        WV_U16,   0, err_cnt[18]),
  WV ("el18",      PG_ERR,                                        WV_U32,   0, err_last[18]),
  WV ("ec19",      PG_ERR,                                        WV_U16,   0, err_cnt[19]),
  WV ("el19",      PG_ERR,                                        WV_U32,   0, err_last[19]),
  WV ("ec20",      PG_ERR,                                        WV_U16,   0, err_cnt[20]),
  WV ("el20",      PG_ERR,                                        WV_U32,   0, err_last[20]),
  WV ("ec21",      PG_ERR,                                        WV_U16,   0, err_cnt[21]),
  WV ("el21",      PG_ERR,                                        WV_U32,   0, err_last[21]),
  WV ("ec22",      PG_ERR,                                        WV_U16,   0, err_cnt[22]),
  WV ("el22",      PG_ERR,                                        WV_U32,   0, err_last[22]),
  WV ("ec23",      PG_ERR,                                        WV_U16,   0, err_cnt[23]),
  WV ("el23",      PG_ERR,                                        WV_U32,   0, err_last[23]),
  WV ("ec24",      PG_ERR,                                        WV_U16,   0, err_cnt[24]),
  WV ("el24",      PG_ERR,                                        WV_U32,   0, err_last[24]),
  WV ("ec25",      PG_ERR,                                        WV_U16,   0, err_cnt[25]),
  WV ("el25",      PG_ERR,                                        WV_U32,   0, err_last[25]),
  WV ("ec26",      PG_ERR,                                        WV_U16,   0, err_cnt[26]),
  WV ("el26",      PG_ERR,                                        WV_U32,   0, err_last[26]),
  WV ("ec27",      PG_ERR,                                        WV_U16,   0, err_cnt[27]),
  WV ("el27",      PG_ERR,                                        WV_U32,   0, err_last[27]),
  WV ("ec28",      PG_ERR,                                        WV_U16,   0, err_cnt[28]),
  WV ("el28",      PG_ERR,                                        WV_U32,   0, err_last[28]),
  WV ("ec29",      PG_ERR,                                        WV_U16,   0, err_cnt[29]),
  WV ("el29",      PG_ERR,                                        WV_U32,   0, err_last[29]),
  WV ("ec30",      PG_ERR,                                        WV_U16,   0, err_cnt[30]),
  WV ("el30",      PG_ERR,                                        WV_U32,   0, err_last[30]),
  WV ("ec31",      PG_ERR,                                        WV_U16,   0, err_cnt[31]),
  WV ("el31",      PG_ERR,                                        WV_U32,   0, err_last[31]),
  WV ("h0t",       PG_ERR,                                        WV_U32,   0, err_hist[0].t),
  WV ("h0i",       PG_ERR,                                        WV_U16,   0, err_hist[0].id),
  WV ("h0e",       PG_ERR,                                        WV_U8,    0, err_hist[0].ev),
  WV ("h0d",       PG_ERR,                                        WV_U32,   0, err_hist[0].dur),
  WV ("h0v",       PG_ERR,                                        WV_FLOAT, 1, err_hist[0].val),
  WV ("h1t",       PG_ERR,                                        WV_U32,   0, err_hist[1].t),
  WV ("h1i",       PG_ERR,                                        WV_U16,   0, err_hist[1].id),
  WV ("h1e",       PG_ERR,                                        WV_U8,    0, err_hist[1].ev),
  WV ("h1d",       PG_ERR,                                        WV_U32,   0, err_hist[1].dur),
  WV ("h1v",       PG_ERR,                                        WV_FLOAT, 1, err_hist[1].val),
  WV ("h2t",       PG_ERR,                                        WV_U32,   0, err_hist[2].t),
  WV ("h2i",       PG_ERR,                                        WV_U16,   0, err_hist[2].id),
  WV ("h2e",       PG_ERR,                                        WV_U8,    0, err_hist[2].ev),
  WV ("h2d",       PG_ERR,                                        WV_U32,   0, err_hist[2].dur),
  WV ("h2v",       PG_ERR,                                        WV_FLOAT, 1, err_hist[2].val),
  WV ("h3t",       PG_ERR,                                        WV_U32,   0, err_hist[3].t),
  WV ("h3i",       PG_ERR,                                        WV_U16,   0, err_hist[3].id),
  WV ("h3e",       PG_ERR,                                        WV_U8,    0, err_hist[3].ev),
  WV ("h3d",       PG_ERR,                                        WV_U32,   0, err_hist[3].dur),
  WV ("h3v",       PG_ERR,                                        WV_FLOAT, 1, err_hist[3].val),
  WV ("h4t",       PG_ERR,                                        WV_U32,   0, err_hist[4].t),
  WV ("h4i",       PG_ERR,                                        WV_U16,   0, err_hist[4].id),
  WV ("h4e",       PG_ERR,                                        WV_U8,    0, err_hist[4].ev),
  WV ("h4d",       PG_ERR,                                        WV_U32,   0, err_hist[4].dur),
  WV ("h4v",       PG_ERR,                                        WV_FLOAT, 1, err_hist[4].val),
  WV ("h5t",       PG_ERR,                                        WV_U32,   0, err_hist[5].t),
  WV ("h5i",       PG_ERR,                                        WV_U16,   0, err_hist[5].id),
  WV ("h5e",       PG_ERR,                                        WV_U8,    0, err_hist[5].ev),
  WV ("h5d",       PG_ERR,                                        WV_U32,   0, err_hist[5].dur),
  WV ("h5v",       PG_ERR,                                        WV_FLOAT, 1, err_hist[5].val),
  WV ("h6t",       PG_ERR,                                        WV_U32,   0, err_hist[6].t),
  WV ("h6i",       PG_ERR,                                        WV_U16,   0, err_hist[6].id),
  WV ("h6e",       PG_ERR,                                        WV_U8,    0, err_hist[6].ev),
  WV ("h6d",       PG_ERR,                                        WV_U32,   0, err_hist[6].dur),
  WV ("h6v",       PG_ERR,                                        WV_FLOAT, 1, err_hist[6].val),
  WV ("h7t",       PG_ERR,                                        WV_U32,   0, err_hist[7].t),
  WV ("h7i",       PG_ERR,                                        WV_U16,   0, err_hist[7].id),
  WV ("h7e",       PG_ERR,                                        WV_U8,    0, err_hist[7].ev),
  WV ("h7d",       PG_ERR,                                        WV_U32,   0, err_hist[7].dur),
  WV ("h7v",       PG_ERR,                                        WV_FLOAT, 1, err_hist[7].val),
  WV ("h8t",       PG_ERR,                                        WV_U32,   0, err_hist[8].t),
  WV ("h8i",       PG_ERR,                                        WV_U16,   0, err_hist[8].id),
  WV ("h8e",       PG_ERR,                                        WV_U8,    0, err_hist[8].ev),
  WV ("h8d",       PG_ERR,                                        WV_U32,   0, err_hist[8].dur),
  WV ("h8v",       PG_ERR,                                        WV_FLOAT, 1, err_hist[8].val),
  WV ("h9t",       PG_ERR,                                        WV_U32,   0, err_hist[9].t),
  WV ("h9i",       PG_ERR,                                        WV_U16,   0, err_hist[9].id),
  WV ("h9e",       PG_ERR,                                        WV_U8,    0, err_hist[9].ev),
  WV ("h9d",       PG_ERR,                                        WV_U32,   0, err_hist[9].dur),
  WV ("h9v",       PG_ERR,                                        WV_FLOAT, 1, err_hist[9].val),
  WV ("h10t",      PG_ERR,                                        WV_U32,   0, err_hist[10].t),
  WV ("h10i",      PG_ERR,                                        WV_U16,   0, err_hist[10].id),
  WV ("h10e",      PG_ERR,                                        WV_U8,    0, err_hist[10].ev),
  WV ("h10d",      PG_ERR,                                        WV_U32,   0, err_hist[10].dur),
  WV ("h10v",      PG_ERR,                                        WV_FLOAT, 1, err_hist[10].val),
  WV ("h11t",      PG_ERR,                                        WV_U32,   0, err_hist[11].t),
  WV ("h11i",      PG_ERR,                                        WV_U16,   0, err_hist[11].id),
  WV ("h11e",      PG_ERR,                                        WV_U8,    0, err_hist[11].ev),
  WV ("h11d",      PG_ERR,                                        WV_U32,   0, err_hist[11].dur),
  WV ("h11v",      PG_ERR,                                        WV_FLOAT, 1, err_hist[11].val),
  WV ("h12t",      PG_ERR,                                        WV_U32,   0, err_hist[12].t),
  WV ("h12i",      PG_ERR,                                        WV_U16,   0, err_hist[12].id),
  WV ("h12e",      PG_ERR,                                        WV_U8,    0, err_hist[12].ev),
  WV ("h12d",      PG_ERR,                                        WV_U32,   0, err_hist[12].dur),
  WV ("h12v",      PG_ERR,                                        WV_FLOAT, 1, err_hist[12].val),
  WV ("h13t",      PG_ERR,                                        WV_U32,   0, err_hist[13].t),
  WV ("h13i",      PG_ERR,                                        WV_U16,   0, err_hist[13].id),
  WV ("h13e",      PG_ERR,                                        WV_U8,    0, err_hist[13].ev),
  WV ("h13d",      PG_ERR,                                        WV_U32,   0, err_hist[13].dur),
  WV ("h13v",      PG_ERR,                                        WV_FLOAT, 1, err_hist[13].val),
  WV ("h14t",      PG_ERR,                                        WV_U32,   0, err_hist[14].t),
  WV ("h14i",      PG_ERR,                                        WV_U16,   0, err_hist[14].id),
  WV ("h14e",      PG_ERR,                                        WV_U8,    0, err_hist[14].ev),
  WV ("h14d",      PG_ERR,                                        WV_U32,   0, err_hist[14].dur),
  WV ("h14v",      PG_ERR,                                        WV_FLOAT, 1, err_hist[14].val),
  WV ("h15t",      PG_ERR,                                        WV_U32,   0, err_hist[15].t),
  WV ("h15i",      PG_ERR,                                        WV_U16,   0, err_hist[15].id),
  WV ("h15e",      PG_ERR,                                        WV_U8,    0, err_hist[15].ev),
  WV ("h15d",      PG_ERR,                                        WV_U32,   0, err_hist[15].dur),
  WV ("h15v",      PG_ERR,                                        WV_FLOAT, 1, err_hist[15].val),
  WV ("h16t",      PG_ERR,                                        WV_U32,   0, err_hist[16].t),
  WV ("h16i",      PG_ERR,                                        WV_U16,   0, err_hist[16].id),
  WV ("h16e",      PG_ERR,                                        WV_U8,    0, err_hist[16].ev),
  WV ("h16d",      PG_ERR,                                        WV_U32,   0, err_hist[16].dur),
  WV ("h16v",      PG_ERR,                                        WV_FLOAT, 1, err_hist[16].val),
  WV ("h17t",      PG_ERR,                                        WV_U32,   0, err_hist[17].t),
  WV ("h17i",      PG_ERR,                                        WV_U16,   0, err_hist[17].id),
  WV ("h17e",      PG_ERR,                                        WV_U8,    0, err_hist[17].ev),
  WV ("h17d",      PG_ERR,                                        WV_U32,   0, err_hist[17].dur),
  WV ("h17v",      PG_ERR,                                        WV_FLOAT, 1, err_hist[17].val),
  WV ("h18t",      PG_ERR,                                        WV_U32,   0, err_hist[18].t),
  WV ("h18i",      PG_ERR,                                        WV_U16,   0, err_hist[18].id),
  WV ("h18e",      PG_ERR,                                        WV_U8,    0, err_hist[18].ev),
  WV ("h18d",      PG_ERR,                                        WV_U32,   0, err_hist[18].dur),
  WV ("h18v",      PG_ERR,                                        WV_FLOAT, 1, err_hist[18].val),
  WV ("h19t",      PG_ERR,                                        WV_U32,   0, err_hist[19].t),
  WV ("h19i",      PG_ERR,                                        WV_U16,   0, err_hist[19].id),
  WV ("h19e",      PG_ERR,                                        WV_U8,    0, err_hist[19].ev),
  WV ("h19d",      PG_ERR,                                        WV_U32,   0, err_hist[19].dur),
  WV ("h19v",      PG_ERR,                                        WV_FLOAT, 1, err_hist[19].val),
  WV ("rtc_e",     PG_ERR,                                        WV_U32,   0, rtc_epoch),
};
#define WEB_VARS_N  (sizeof(web_vars) / sizeof(web_vars[0]))

/* ===========================================================================
 * 3) PAGINAS: nombre de la ruta WebSocket -> bit de pagina.
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
};

/* Paginas que reciben el texto del terminal (WebTerm_Print) */
#define TERM_PAGES  (PG_SISTEMA | PG_WIDGETS | PG_INDUSTRIA | PG_INTERNO | PG_HVAC)

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

uint16_t WebTerm_Pages(void) { return TERM_PAGES; }

