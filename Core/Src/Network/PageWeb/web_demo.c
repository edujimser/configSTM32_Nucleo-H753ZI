/* =============================================================================
 * web_demo.c  -  Variables y simulacion de DEMOSTRACION (para ver las paginas con datos).
 * Cuando uses tus variables reales, borra este archivo y las lineas de la tabla de
 * web_vars.c que las usan. Una instalacion real de HVAC sustituye hvac_tick() por tus
 * lecturas (sondas, presiones, estados) y tus comandos.
 * ============================================================================= */
#include <math.h>
#include <stdint.h>
#include "Network/PageWeb/web_demo.h"
#include "Network/PageWeb/web_bus.h"
#include "Network/PageWeb/web_vars.h"
#include "Network/PageWeb/web_err.h"


volatile uint32_t demo_contador  = 0;
volatile float    demo_temp      = 20.0f;
volatile uint8_t  demo_led       = 0;
volatile uint32_t demo_contador2 = 0;
volatile uint8_t  demo_nivel     = 0;
volatile float    demo_roll      = 0.0f;
volatile float    demo_pitch     = 0.0f;
volatile float    demo_yaw       = 0.0f;
volatile uint16_t demo_rpm       = 0;
volatile float    demo_setpoint  = 22.0f;
volatile uint8_t  demo_modo      = 0;
volatile uint8_t  demo_enable    = 1;
volatile uint8_t  demo_brillo    = 50;
volatile uint8_t  ind_run[3]     = {1, 1, 0};
volatile uint16_t ind_rpm[3]     = {0, 0, 0};
volatile float    ind_temp[3]    = {40.0f, 40.0f, 40.0f};
volatile float    ind_amp[3]     = {0.0f, 0.0f, 0.0f};
volatile uint8_t  ind_estado     = 0;
volatile uint8_t  ind_auto       = 1;
volatile uint8_t  ind_entradas   = 0;
volatile uint8_t  ind_salidas    = 0;
volatile float    ind_presion    = 5.0f;
volatile float    ind_caudal     = 40.0f;
volatile uint32_t ind_piezas     = 0;
volatile uint32_t ind_uptime     = 0;
volatile uint8_t  ind_valvula    = 0;
volatile uint8_t  hv_modo        = 1;
volatile uint8_t  hv_real        = 0;
volatile float    sp_t           = 23.0f;
volatile float    t_int          = 26.0f;
volatile float    t_ext          = 31.0f;
volatile float    t_imp          = 26.0f;
volatile float    t_ret          = 26.0f;
volatile uint8_t  hum            = 50;
volatile uint16_t co2            = 600;
volatile float    pid_out        = 0.0f;
volatile uint8_t  pid_mode       = 0;
volatile float    pid_man        = 30.0f;
volatile float    pid_kp         = 12.0f;
volatile float    pid_ki         = 0.40f;
volatile float    pid_kd         = 0.0f;
volatile uint8_t  c_st[2]        = {0, 0};
volatile uint8_t  c1_hz          = 0;
volatile float    c_a[2]         = {0.0f, 0.0f};
volatile float    c_hp[2]        = {8.0f, 8.0f};
volatile float    c_lp[2]        = {8.0f, 8.0f};
volatile float    c_td[2]        = {30.0f, 30.0f};
volatile float    c_ts[2]        = {20.0f, 20.0f};
volatile uint8_t  c_eev[2]       = {0, 0};
volatile float    c_sh[2]        = {0.0f, 0.0f};
volatile float    c_sc[2]        = {0.0f, 0.0f};
volatile uint8_t  c_fc[2]        = {0, 0};
volatile uint8_t  c_4v[2]        = {0, 0};
volatile uint8_t  c_hg[2]        = {0, 0};
volatile uint8_t  c2_yd          = 0;
volatile uint8_t  c2_tyd         = 0;
volatile uint16_t out_w          = 0;
volatile uint8_t  in_w           = 0xFF;
volatile uint16_t al_w           = 0;
volatile uint8_t  ev_fan         = 0;
volatile uint16_t dp_filt        = 80;
volatile uint16_t caudal_aire    = 0;
volatile uint8_t  m_c1 = 0, m_c2 = 0, m_ev = 0, m_cd = 0, m_hg = 0, m_4v = 0, m_cv = 0, m_fl = 0;
volatile float    i_l[3]         = {0.0f, 0.0f, 0.0f};
volatile uint16_t v_ll           = 400;
volatile float    kw             = 0.0f;
volatile uint32_t wh             = 0;
volatile float    desq           = 0.0f;
volatile uint8_t  fl_fre         = 20;
volatile uint8_t  fl_fre_sp      = 30;
volatile uint8_t  fl_ret         = 80;
volatile uint8_t  fl_ret_sp      = 70;
volatile float    cv_a           = 0.0f;
volatile float    ev_a           = 0.0f;
volatile float    sg_can_rx[6]   = {0};
volatile float    sg_trdp_rx[3]  = {0};
char     dev_name[24]    = "Unidad HVAC TREN-01";
char     dev_sn[16]      = "SN-2026-000123";
char     dev_hw[12]      = "REV C";
char     dev_mac[18]     = "02:00:5E:10:00:01";
char     fw_n[7][20]     = {"Bootloader", "Firmware", "Web (packed_fs)", "Aplicacion HVAC", "Base CAN (DBC)", "XML TRDP", "Parametros"};
char     fw_v[7][16]     = {"1.0.3", "2.4.1", "2.4.1", "3.1.0", "1.8", "1.2", "5"};
uint32_t fw_crc[7]       = {0x1A2B3C4Du, 0x9F00AA11u, 0x5E5E1234u, 0xC0FFEE01u, 0x00BEEF77u, 0x77AA3201u, 0x12345678u};
uint32_t fw_size[7]      = {32768u, 412160u, 580000u, 96200u, 18322u, 6490u, 1024u};
uint8_t  fw_ok[7]        = {1, 1, 1, 1, 1, 1, 1};
uint32_t rtc_epoch       = 1793700000u;
int32_t  rtc_tz          = 60;
uint8_t  net_dhcp        = 1;
uint32_t net_ip          = 0xC0A80164u;
uint32_t net_mask        = 0xFFFFFF00u;
uint32_t net_gw          = 0xC0A80101u;
uint32_t net_dns         = 0x08080808u;
uint16_t net_dhcp_to     = 30;
uint32_t net_lease       = 86400u;
uint8_t  net_state       = 2;
uint32_t net_cip         = 0xC0A80164u;
uint32_t net_cmask       = 0xFFFFFF00u;
uint32_t net_cgw         = 0xC0A80101u;
uint32_t net_cdns        = 0x08080808u;
uint32_t net_left        = 86400u;
uint32_t net_t1          = 43200u;
uint32_t net_t2          = 75600u;
uint32_t net_srv         = 0xC0A80101u;
uint8_t  dem_bat         = 80;
uint8_t  dem_rssi        = 70;
uint8_t  dem_chg         = 0;

/* ------------------------------------------------------------------ utilidades */
static float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
static uint8_t u8c(float x) { return (uint8_t) clampf(x + 0.5f, 0.0f, 255.0f); }
static uint16_t u16c(float x) { return (uint16_t) clampf(x + 0.5f, 0.0f, 65535.0f); }
#define BIT(n, c)  ((uint16_t) ((c) ? (1u << (n)) : 0u))

/* ------------------------------------------------------------------ simulacion HVAC */
static uint32_t hv_tick = 0;
static float    pid_i   = 0.0f;
static uint8_t  run_t[2] = {0, 0};          /* segundos desde el arranque de cada compresor */
static uint8_t  prev_run[2] = {0, 0};
static float    whacc = 0.0f;

static uint8_t aplicar_mando(uint8_t mando, uint8_t auto_val) {   /* 0 AUTO, 1 OFF, 2 ON */
  return mando == 1 ? 0 : (mando == 2 ? 1 : auto_val);
}

static void hvac_tick(void) {
  hv_tick++;
  float t = (float) hv_tick;

  /* ---- ambiente y modo real: 0 PARADO 1 VENT 2 FRIO 3 CALOR 4 DESESCARCHE */
  t_ext = 31.0f + 4.0f * sinf(t * 0.02f);
  uint8_t real;
  switch (hv_modo) {
    case 0:  real = 0; break;
    case 2:  real = 2; break;
    case 3:  real = 3; break;
    case 4:  real = 1; break;
    default: real = (t_int > sp_t + 1.0f) ? 2 : ((t_int < sp_t - 1.0f) ? 3 : 1); break;
  }
  if (real == 3 && (hv_tick % 90u) < 8u) real = 4;
  hv_real = real;

  /* ---- PID de temperatura interior (frio: sube la salida si hace calor) */
  float err = (real == 3 || real == 4) ? (sp_t - t_int) : (t_int - sp_t);
  float out = 0.0f;
  if (real >= 2) {
    if (pid_mode == 0) {
      pid_i = clampf(pid_i + pid_ki * err, 0.0f, 100.0f);
      out = clampf(pid_kp * err + pid_i, 0.0f, 100.0f);
    } else {
      out = clampf(pid_man, 0.0f, 100.0f);
      pid_i = out;                                     /* sin saltos al volver a automatico */
    }
  } else {
    pid_i = 0.0f;
  }
  pid_out = out;

  /* ---- mando de compresores */
  uint8_t want1 = (real >= 2) && (out > 5.0f);
  uint8_t want2 = (real >= 2) && (out > 35.0f || (prev_run[1] && out > 20.0f));   /* con histeresis */
  uint8_t run[2] = { aplicar_mando(m_c1, want1), aplicar_mando(m_c2, want2) };
  for (int i = 0; i < 2; i++) {
    if (run[i] && !prev_run[i]) run_t[i] = 0;
    if (run[i]) { if (run_t[i] < 250) run_t[i]++; } else run_t[i] = 0;
    prev_run[i] = run[i];
  }

  /* ---- circuito frigorifico */
  uint8_t calor = (real == 3);
  for (int i = 0; i < 2; i++) {
    float f = (float) (i + 1);
    float osc = sinf(t * 0.5f + f);
    if (run[i]) {
      float hp = 14.0f + (t_ext - 25.0f) * 0.4f + out * 0.06f;
      float lp = 5.5f - out * 0.015f;
      if (calor) { hp = 15.0f + out * 0.06f; lp = 3.2f + (t_ext - 25.0f) * 0.05f; }
      if (real == 4) { hp = 12.0f; lp = 8.0f; }
      c_hp[i] = hp + 0.25f * osc;
      c_lp[i] = lp + 0.08f * osc;
      c_td[i] = 55.0f + out * 0.25f + (c_hp[i] - 14.0f) * 1.2f;
      c_sh[i] = 6.0f + 1.2f * osc;
      c_sc[i] = 5.0f + 0.8f * sinf(t * 0.3f + f);
      c_ts[i] = (calor ? -2.0f : 5.0f) + c_sh[i];
      c_eev[i] = u8c(25.0f + out * 0.5f);
      c_fc[i] = u8c(40.0f + out * 0.5f);
    } else {
      float eq = 8.0f + (t_ext - 25.0f) * 0.2f;
      c_hp[i] += (eq - c_hp[i]) * 0.3f;
      c_lp[i] += (eq - c_lp[i]) * 0.3f;
      c_td[i] += (t_ext - c_td[i]) * 0.1f;
      c_ts[i] += (t_ext - c_ts[i]) * 0.1f;
      c_sh[i] = 0.0f; c_sc[i] = 0.0f; c_eev[i] = 0; c_fc[i] = 0;
    }
    c_4v[i] = calor;
    c_hg[i] = (real == 4);
  }

  /* disparo de presostato de alta (demo): 6 s cada 200 s en el circuito 1 */
  uint8_t ap1_trip = (hv_tick % 200u) >= 150u && (hv_tick % 200u) < 156u;
  if (ap1_trip) { c_hp[0] = 27.5f; }
  if (ap1_trip) run[0] = 0;

  /* estados de compresor: 0 parado 1 arrancando 2 marcha 3 fallo */
  c_st[0] = ap1_trip ? 3 : (run[0] ? (run_t[0] < 3 ? 1 : 2) : 0);
  c1_hz   = run[0] ? u8c(30.0f + out * 0.6f) : 0;

  /* compresor 2: arranque estrella-triangulo (3 s en estrella) */
  if (run[1]) {
    uint8_t rem = run_t[1] <= 3 ? (uint8_t) (3 - run_t[1]) : 0;
    c2_yd = run_t[1] <= 3 ? 1 : 2;
    c2_tyd = rem;
  } else { c2_yd = 0; c2_tyd = 0; }
  c_st[1] = run[1] ? (c2_yd == 1 ? 1 : 2) : 0;

  /* corrientes */
  c_a[0] = run[0] ? 4.0f + c1_hz * 0.12f + 0.3f * sinf(t) : 0.0f;
  c_a[1] = !run[1] ? 0.0f : (c2_yd == 1 ? 7.0f : 15.0f + 0.5f * sinf(t * 0.8f));

  /* ---- aire */
  uint8_t evon = aplicar_mando(m_ev, real >= 1);
  ev_fan = evon ? u8c(80.0f) : 0;
  caudal_aire = (uint16_t) (ev_fan * 45u);
  ev_a = evon ? 3.2f + 0.1f * sinf(t * 0.6f) : 0.0f;
  dp_filt = (uint16_t) (80u + (hv_tick % 300u) * 9u / 10u);

  /* ---- convectores (resistencias de apoyo en calor) */
  uint8_t cv1 = aplicar_mando(m_cv, calor && out > 50.0f);
  uint8_t cv2 = aplicar_mando(m_cv, calor && out > 70.0f);
  uint8_t cv3 = aplicar_mando(m_cv, calor && out > 90.0f);
  cv_a = (float) (cv1 + cv2 + cv3) * 8.7f;

  /* ---- planta: la temperatura interior responde a la salida */
  t_int += (t_ext - t_int) * 0.004f + 0.003f;
  if (real == 2) t_int -= out * 0.002f;
  if (real == 3) t_int += out * 0.002f;
  t_int = clampf(t_int, 10.0f, 40.0f);
  t_imp = real == 2 ? t_int - 8.0f * out / 100.0f - 2.0f : (real >= 3 ? t_int + 12.0f * out / 100.0f + 2.0f : t_int);
  uint8_t sonda_fallo = (hv_tick % 120u) >= 100u && (hv_tick % 120u) < 106u;
  t_ret = sonda_fallo ? NAN : t_int + 0.5f;
  hum = u8c(48.0f + 6.0f * sinf(t * 0.05f));
  co2 = u16c(650.0f + 350.0f * sinf(t * 0.03f) + 350.0f);

  /* ---- trampillas: se mueven un 10 % por segundo hacia su objetivo */
  uint8_t sp_f = m_fl == 1 ? 0 : (m_fl == 2 ? 100 : fl_fre_sp);
  uint8_t sp_r = m_fl == 1 ? 0 : (m_fl == 2 ? 100 : fl_ret_sp);
  if (fl_fre < sp_f) fl_fre = (uint8_t) (fl_fre + 10 > sp_f ? sp_f : fl_fre + 10);
  else if (fl_fre > sp_f) fl_fre = (uint8_t) (fl_fre < sp_f + 10 ? sp_f : fl_fre - 10);
  if (fl_ret < sp_r) fl_ret = (uint8_t) (fl_ret + 10 > sp_r ? sp_r : fl_ret + 10);
  else if (fl_ret > sp_r) fl_ret = (uint8_t) (fl_ret < sp_r + 10 ? sp_r : fl_ret - 10);

  /* ---- salidas y entradas */
  uint8_t cd_on = aplicar_mando(m_cd, run[0] || run[1]);
  uint8_t hg_on = aplicar_mando(m_hg, real == 4);
  uint8_t v4_on = aplicar_mando(m_4v, calor);
  out_w = (uint16_t) (BIT(0, run[0]) | BIT(1, run[1]) | BIT(2, evon) | BIT(3, cd_on && c_fc[0]) | BIT(4, cd_on && c_fc[1]) |
          BIT(5, hg_on) | BIT(6, hg_on) | BIT(7, v4_on) | BIT(8, v4_on) | BIT(9, cv1) | BIT(10, cv2) | BIT(11, cv3) |
          BIT(12, fl_fre > 50) | BIT(13, run[1] && c2_yd == 1) | BIT(14, run[1] && c2_yd == 2) | BIT(15, al_w != 0));
  in_w = (uint8_t) (BIT(0, !ap1_trip) | BIT(1, 1) | BIT(2, 1) | BIT(3, 1) | BIT(4, 1) | BIT(5, 1) | BIT(6, 1) | BIT(7, 1));

  /* ---- alarmas (bit a 1 = alarma activa) */
  al_w = (uint16_t) (BIT(0, ap1_trip) | BIT(1, c_lp[0] < 2.0f && run[0]) | BIT(2, c_hp[1] > 26.0f) | BIT(3, c_lp[1] < 2.0f && run[1]) |
         BIT(6, sonda_fallo) | BIT(7, dp_filt > 250u) | BIT(11, desq > 6.0f) | BIT(12, co2 > 1400u) | BIT(13, c_td[0] > 105.0f));

  /* ---- electrico */
  float tot = c_a[0] + c_a[1] + ev_a + (float) (cd_on ? 4 : 0) * 0.5f + cv_a;
  i_l[0] = tot * (1.0f + 0.03f * sinf(t * 0.4f));
  i_l[1] = tot * (1.0f - 0.02f * sinf(t * 0.4f + 1.0f));
  i_l[2] = tot * (1.0f - 0.01f * sinf(t * 0.4f + 2.0f));
  v_ll = (uint16_t) (398u + (hv_tick % 5u));
  float imed = (i_l[0] + i_l[1] + i_l[2]) / 3.0f;
  float imax = i_l[0] > i_l[1] ? (i_l[0] > i_l[2] ? i_l[0] : i_l[2]) : (i_l[1] > i_l[2] ? i_l[1] : i_l[2]);
  desq = imed > 0.5f ? (imax - imed) / imed * 100.0f : 0.0f;
  kw = 1.732f * (float) v_ll * imed * 0.85f / 1000.0f;
  whacc += kw * 1000.0f / 3600.0f;
  wh = (uint32_t) whacc;
}

/* ------------------------------------------------------------------ simulacion: objeto y buses */
void WebNet_Apply(void) {                    /* DEMO: aqui aplicarias la configuracion a tu pila IP */
  if (net_dhcp) { net_state = 2; net_cip = 0xC0A80164u; net_cmask = 0xFFFFFF00u; net_cgw = 0xC0A80101u; net_cdns = 0x08080808u;
                  net_left = net_lease; net_t1 = net_lease / 2u; net_t2 = net_lease * 7u / 8u; }
  else { net_state = 3; net_cip = net_ip; net_cmask = net_mask; net_cgw = net_gw; net_cdns = net_dns; net_left = 0; net_t1 = 0; net_t2 = 0; }
  WebTerm_Print("red: configuracion aplicada (demo)");
}

static void put32(uint8_t *d, uint32_t v) { d[0] = (uint8_t) v; d[1] = (uint8_t) (v >> 8); d[2] = (uint8_t) (v >> 16); d[3] = (uint8_t) (v >> 24); }

static void extra_tick(void) {
  static uint32_t n = 0; n++;
  rtc_epoch++;
  if (net_dhcp && net_left > 0) net_left--;
  dem_bat = (uint8_t) (60u + (n / 3u) % 41u);
  dem_chg = (uint8_t) ((n / 20u) & 1u);
  dem_rssi = (uint8_t) (55u + (unsigned) (20.0f * sinf((float) n * 0.2f) + 20.0f));
  /* tramas simuladas: CAN (6 ids) y TRDP (4 ComId), varias por segundo */
  uint8_t d[8];
  for (uint32_t k = 0; k < 10u; k++) {
    put32(d, (uint32_t) (t_int * 10.0f)); put32(d + 4, (uint32_t) (n * 10u + k));
    WebBus_Rx(WEBBUS_CAN, 0x100u, 8, d);
  }
  for (uint32_t k = 0; k < 2u; k++) {
    put32(d, (uint32_t) (c_hp[0] * 100.0f) | ((uint32_t) (c_lp[0] * 100.0f) << 16)); put32(d + 4, (uint32_t) hv_real);
    WebBus_Rx(WEBBUS_CAN, 0x1A5u, 8, d);
  }
  put32(d, al_w); put32(d + 4, out_w); WebBus_Rx(WEBBUS_CAN, 0x2F0u, 4, d);
  if ((n % 5u) == 0u) { put32(d, (uint32_t) (kw * 10.0f)); put32(d + 4, wh); WebBus_Rx(WEBBUS_CAN, 0x3A0u, 8, d); }
  if ((n % 2u) == 0u) { put32(d, n); put32(d + 4, 0); WebBus_Rx(WEBBUS_CAN, 0x7FFu, 2, d); }
  put32(d, (uint32_t) (t_int * 10.0f) | ((uint32_t) (sp_t * 10.0f) << 16)); put32(d + 4, hv_real);
  WebBus_Rx(WEBBUS_TRDP, 1001u, 8, d);
  put32(d, al_w); put32(d + 4, 0); WebBus_Rx(WEBBUS_TRDP, 1002u, 8, d);
  if ((n % 2u) == 0u) { put32(d, (uint32_t) (kw * 10.0f)); put32(d + 4, wh); WebBus_Rx(WEBBUS_TRDP, 1010u, 8, d); }
  put32(d, n); put32(d + 4, 0xA5A5A5A5u); WebBus_Rx(WEBBUS_TRDP, 2000u, 8, d);
  WebBus_SetState(WEBBUS_CAN, 2, (uint8_t) (18u + (n % 7u)));
  WebBus_SetState(WEBBUS_TRDP, 2, (uint8_t) (4u + (n % 3u)));
  if ((n % 45u) == 0u) { WebBus_CountErr(WEBBUS_CAN); }
  /* si la web fuerza TX, en la demo solo se cuentan las tramas que saldrian */
  uint32_t id; uint8_t len; uint8_t dd[8];
  for (int b = 0; b < 2; b++) { int g = 0; while (g++ < 8 && WebBus_TxPoll(b, n * 1000u, &id, &len, dd)) { } }
}

void WebVars_DemoTick(void) {
  demo_contador++;
  demo_contador2 += 10;
  demo_temp  = 20.0f + (float) (demo_contador % 10) * 0.7f;
  demo_led   = (uint8_t) (demo_contador & 1u);
  demo_nivel = (uint8_t) ((demo_contador * 7u) % 101u);
  demo_roll  = 30.0f * sinf((float) demo_contador * 0.3f);
  demo_pitch = 20.0f * sinf((float) demo_contador * 0.2f);
  demo_yaw   = (float) ((demo_contador * 15u) % 360u);
  demo_rpm   = (uint16_t) (demo_enable ? 600u + (demo_contador * 97u) % 2400u : 0u);

  /* industria */
  if (ind_auto) ind_estado = (uint8_t) ((demo_contador / 6u) % 4u);
  ind_run[0] = (ind_estado == 2);
  ind_run[1] = (ind_estado == 2) && ((demo_contador % 14u) < 10u);
  ind_run[2] = (ind_estado == 1 || ind_estado == 2);
  for (int i = 0; i < 3; i++) {
    ind_rpm[i]  = (uint16_t) (ind_run[i] ? 1200u + (uint16_t) (i * 300) + (demo_contador * 37u + (unsigned) i * 50u) % 400u : 0u);
    float objetivo = ind_run[i] ? 55.0f + (float) i * 8.0f + 6.0f * sinf((float) demo_contador * 0.4f + (float) i) : 30.0f;
    ind_temp[i] += (objetivo - ind_temp[i]) * 0.3f;
    ind_amp[i]   = ind_run[i] ? 10.0f + (float) i + 2.0f * sinf((float) demo_contador * 0.7f) : 0.0f;
  }
  ind_entradas = (uint8_t) (demo_contador * 37u);
  ind_salidas  = (uint8_t) ((ind_run[0] ? 1u : 0u) | (ind_run[1] ? 2u : 0u) | (ind_run[2] ? 4u : 0u) | (ind_estado == 3 ? 128u : 0u));
  ind_presion  = 5.0f + 3.4f * sinf((float) demo_contador * 0.25f);
  ind_caudal   = 45.0f + 40.0f * sinf((float) demo_contador * 0.15f + 1.0f);
  if (ind_caudal < 0.0f) ind_caudal = 0.0f;
  if (ind_estado == 2) ind_piezas += 3u;
  ind_uptime  += 1u;
  ind_valvula  = (ind_estado == 2);

  hvac_tick();
  extra_tick();
}
