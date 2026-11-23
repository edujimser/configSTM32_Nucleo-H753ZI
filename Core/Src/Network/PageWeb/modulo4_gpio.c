
#include "main.h"                 /* registros del STM32H7: GPIOx, RCC */
#include "Network/PageWeb/modulo4_gpio.h"

/*******************************************************************************************/
/*                              VISOR DE PINES GPIO  -  MODULO 4                          */
/*******************************************************************************************/

volatile GpioInfo_t gpio_info;

/* Puertos en orden A, B, C... (los que no existan en el chip no se compilan) */
static GPIO_TypeDef * const puertos[] = {
  GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH,
#ifdef GPIOI
  GPIOI,
#endif
#ifdef GPIOJ
  GPIOJ,
#endif
#ifdef GPIOK
  GPIOK,
#endif
};
#define N_PUERTOS ((int) (sizeof(puertos) / sizeof(puertos[0])))

void Modulo4_Gpio(void) {
  volatile GpioInfo_t *g = &gpio_info;           /* solo lo escribe esta funcion */
  uint8_t on = 0;

  for (int i = 0; i < GPIOINFO_PORTS; i++) {
    volatile GpioPort_t *p = &g->p[i];
    /* RCC_AHB4ENR: bit 0 = GPIOA, bit 1 = GPIOB, ... (mismo orden que el array) */
    uint8_t clk = (i < N_PUERTOS) && ((RCC->AHB4ENR >> i) & 1u);
    p->clk = clk;
    if (!clk) {                                   /* sin reloj: los registros no responden */
      p->idr = p->odr = p->otyper = 0;
      p->moder = p->pupdr = p->afrl = p->afrh = 0;
      continue;
    }
    GPIO_TypeDef *r = puertos[i];
    p->idr    = (uint16_t) r->IDR;
    p->odr    = (uint16_t) r->ODR;
    p->otyper = (uint16_t) r->OTYPER;
    p->moder  = r->MODER;
    p->pupdr  = r->PUPDR;
    p->afrl   = r->AFR[0];
    p->afrh   = r->AFR[1];
    on++;
  }
  g->ports_on = on;
}
