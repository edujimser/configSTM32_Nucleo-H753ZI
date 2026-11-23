#include <string.h>
#include "main.h"                 /* HAL + registros del STM32H7 (RCC, PWR, FLASH, DBGMCU) */
#include "Network/PageWeb/modulo3_clocks.h"

/*******************************************************************************************/
/*                         RELOJES Y TEMPERATURA DEL MICRO  -  MODULO 3                    */
/*******************************************************************************************/

/* ---- ADC para temperatura / Vref+ / VBAT --------------------------------------------------
 * Solo se compila si el ADC esta activado en CubeMX (HAL_ADC_MODULE_ENABLED).
 * Por defecto usa hadc3 (en el STM32H753 los canales internos estan SOLO en el ADC3).   */
#ifdef HAL_ADC_MODULE_ENABLED
#ifndef MOD6_ADC
#define MOD6_ADC hadc3
#endif
extern ADC_HandleTypeDef MOD6_ADC;
#endif

volatile ClockInfo_t clock_info;

/* ---- utilidades ---- */
static uint8_t escala_vos(void) {
  /* PWR_D3CR.VOS: 3 = escala 1, 2 = escala 2, 1 = escala 3.  Escala 0 = escala 1 + overdrive */
  uint32_t v = (PWR->D3CR & PWR_D3CR_VOS) >> PWR_D3CR_VOS_Pos;
#ifdef SYSCFG_PWRCR_ODEN
  if (v == 3u && (SYSCFG->PWRCR & SYSCFG_PWRCR_ODEN)) return 0;
#endif
  return (v == 3u) ? 1 : (v == 2u) ? 2 : 3;
}

static uint16_t osciladores(void) {
  uint16_t b = 0;
  if (RCC->CR   & RCC_CR_HSIRDY)    b |= OSC_HSI;
  if (RCC->CR   & RCC_CR_CSIRDY)    b |= OSC_CSI;
  if (RCC->CR   & RCC_CR_HSERDY)    b |= OSC_HSE;
  if (RCC->CR   & RCC_CR_HSI48RDY)  b |= OSC_HSI48;
  if (RCC->CSR  & RCC_CSR_LSIRDY)   b |= OSC_LSI;
  if (RCC->BDCR & RCC_BDCR_LSERDY)  b |= OSC_LSE;
  if (RCC->CR   & RCC_CR_PLL1RDY)   b |= OSC_PLL1;
  if (RCC->CR   & RCC_CR_PLL2RDY)   b |= OSC_PLL2;
  if (RCC->CR   & RCC_CR_PLL3RDY)   b |= OSC_PLL3;
  return b;
}

#ifdef HAL_ADC_MODULE_ENABLED
/* Lee un canal del ADC (conversion unica, por sondeo). Devuelve -1 si falla. */
static int32_t leer_adc(uint32_t canal) {
  ADC_ChannelConfTypeDef c;
  memset(&c, 0, sizeof(c));
  c.Channel      = canal;
  c.Rank         = ADC_REGULAR_RANK_1;
  c.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;   /* el sensor necesita >= 9 us de muestreo */
  c.SingleDiff   = ADC_SINGLE_ENDED;
  c.OffsetNumber = ADC_OFFSET_NONE;
  if (HAL_ADC_ConfigChannel(&MOD6_ADC, &c) != HAL_OK) return -1;
  if (HAL_ADC_Start(&MOD6_ADC) != HAL_OK) return -1;
  int32_t v = -1;
  if (HAL_ADC_PollForConversion(&MOD6_ADC, 10) == HAL_OK) v = (int32_t) HAL_ADC_GetValue(&MOD6_ADC);
  HAL_ADC_Stop(&MOD6_ADC);
  return v;
}

/* ---- Calibraciones de fabrica del STM32H74x/H75x (RM0433 / datasheet) ----------------
 * Se definen aqui A PROPOSITO en vez de usar las macros de la HAL: en las versiones de
 * CubeMX 6.17/6.18 la macro __LL_ADC_CALC_VREFANALOG_VOLTAGE escala a 12 bits y en el
 * H743/H753 da Vref+ x16 (53 V) y temperaturas de ~5000 C.
 * Todas estas calibraciones estan tomadas a 16 bits y con Vref+ = 3.3 V.              */
#define CAL_VREFINT    (*(const volatile uint16_t *) 0x1FF1E860u)   /* VREFINT a 30 C      */
#define CAL_TS1        (*(const volatile uint16_t *) 0x1FF1E820u)   /* sensor a 30 C       */
#define CAL_TS2        (*(const volatile uint16_t *) 0x1FF1E840u)   /* sensor a 110/130 C  */
#define CAL_VREF_MV    3300.0f
#define CAL_TS1_C      30.0f
#define CAL_TS2_C      ((((DBGMCU->IDCODE) >> 16) <= 0x1003u) ? 110.0f : 130.0f)  /* rev Y : rev V */

/* Bits reales del ADC segun la resolucion configurada en CubeMX */
static uint32_t bits_adc(uint32_t res) {
  if (res == ADC_RESOLUTION_16B) return 16;
#ifdef ADC_RESOLUTION_14B
  if (res == ADC_RESOLUTION_14B) return 14;
#endif
  if (res == ADC_RESOLUTION_12B) return 12;
#ifdef ADC_RESOLUTION_10B
  if (res == ADC_RESOLUTION_10B) return 10;
#endif
  return 8;
}

static void medir_analogico(volatile ClockInfo_t *r) {
  static uint8_t calibrado = 0;
  if (!calibrado) {                                /* calibracion del ADC, una sola vez */
    if (HAL_ADCEx_Calibration_Start(&MOD6_ADC, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK) { r->adc_ok = 0; return; }
    calibrado = 1;
  }
  int32_t vrefint = leer_adc(ADC_CHANNEL_VREFINT);
  int32_t ts      = leer_adc(ADC_CHANNEL_TEMPSENSOR);
  int32_t vbat    = leer_adc(ADC_CHANNEL_VBAT);
  if (vrefint <= 0 || ts < 0) { r->adc_ok = 0; return; }

  /* Todo se lleva a 16 bits, que es como estan las calibraciones */
  uint32_t sh = 16u - bits_adc(MOD6_ADC.Init.Resolution);
  float vrefint16 = (float) ((uint32_t) vrefint << sh);
  float ts16      = (float) ((uint32_t) ts      << sh);

  /* Vref+ real: la referencia interna mide siempre lo mismo (unos 1.21 V);
   * si el ADC la lee "mas grande" de lo que leyo en fabrica, es que Vref+ es menor */
  float vref = (float) CAL_VREFINT * CAL_VREF_MV / vrefint16;               /* mV */
  if (vref < 1600.0f || vref > 3700.0f) { r->adc_ok = 0; return; }          /* lectura no valida */

  /* Temperatura: recta entre los dos puntos de fabrica, con la lectura llevada a 3.3 V */
  float ts33 = ts16 * vref / CAL_VREF_MV;
  float c1 = (float) CAL_TS1, c2 = (float) CAL_TS2;
  float t  = (c2 != c1) ? CAL_TS1_C + (ts33 - c1) * (CAL_TS2_C - CAL_TS1_C) / (c2 - c1) : 0.0f;

  /* VBAT: el canal mide VBAT/4 */
  float vb = (vbat >= 0) ? (float) ((uint32_t) vbat << sh) * vref / 65535.0f * 4.0f : 0.0f;

  r->vref_mv = (uint16_t) (vref + 0.5f);
  r->vbat_mv = (uint16_t) (vb + 0.5f);
  r->temp_c  = t;
  if (!r->adc_ok || t < r->temp_min) r->temp_min = t;
  if (!r->adc_ok || t > r->temp_max) r->temp_max = t;
  r->adc_ok  = 1;
}
#endif

/* =============================================================================
 * Recalcular todo (cada segundo)
 * ============================================================================= */
void Modulo3_Clocks(void) {
  volatile ClockInfo_t *r = &clock_info;          /* solo lo escribe esta funcion */
  PLL1_ClocksTypeDef p1; PLL2_ClocksTypeDef p2; PLL3_ClocksTypeDef p3;

  r->sysclk = HAL_RCC_GetSysClockFreq();
  r->cpu    = SystemCoreClock;
  r->hclk   = HAL_RCC_GetHCLKFreq();
  r->apb1   = HAL_RCC_GetPCLK1Freq();
  r->apb2   = HAL_RCC_GetPCLK2Freq();
  r->apb3   = HAL_RCCEx_GetD1PCLK1Freq();
  r->apb4   = HAL_RCCEx_GetD3PCLK1Freq();

  HAL_RCCEx_GetPLL1ClockFreq(&p1);                /* devuelven 0 si el PLL esta apagado */
  HAL_RCCEx_GetPLL2ClockFreq(&p2);
  HAL_RCCEx_GetPLL3ClockFreq(&p3);
  r->pll1_p = p1.PLL1_P_Frequency; r->pll1_q = p1.PLL1_Q_Frequency; r->pll1_r = p1.PLL1_R_Frequency;
  r->pll2_p = p2.PLL2_P_Frequency; r->pll2_q = p2.PLL2_Q_Frequency; r->pll2_r = p2.PLL2_R_Frequency;
  r->pll3_p = p3.PLL3_P_Frequency; r->pll3_q = p3.PLL3_Q_Frequency; r->pll3_r = p3.PLL3_R_Frequency;

  r->hse_hz  = HSE_VALUE;
  r->hsi_hz  = HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV) >> RCC_CR_HSIDIV_Pos);
  r->sys_src = (uint8_t) ((RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos);
  r->pll_src = (uint8_t) ((RCC->PLLCKSELR & RCC_PLLCKSELR_PLLSRC) >> RCC_PLLCKSELR_PLLSRC_Pos);
  r->osc_on  = osciladores();
  r->vos     = escala_vos();
  r->flash_ws = (uint8_t) (FLASH->ACR & FLASH_ACR_LATENCY);

  r->dev_id = (uint16_t) (DBGMCU->IDCODE & DBGMCU_IDCODE_DEV_ID_Msk);
  r->rev_id = (uint16_t) (DBGMCU->IDCODE >> DBGMCU_IDCODE_REV_ID_Pos);
  r->rev_txt[0] = (r->rev_id == 0x1003u) ? 'Y' : (r->rev_id == 0x2003u) ? 'V' : '?';
  r->rev_txt[1] = '\0';

#ifdef HAL_ADC_MODULE_ENABLED
  medir_analogico(r);
#else
  r->adc_ok = 0;                                   /* ADC3 no activado en CubeMX */
#endif
}