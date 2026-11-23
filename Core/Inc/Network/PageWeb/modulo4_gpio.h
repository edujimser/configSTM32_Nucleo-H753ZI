#ifndef WEB_MODULO4_GPIO_H
#define WEB_MODULO4_GPIO_H

#include <stdint.h>

/*******************************************************************************************/
/*                              VISOR DE PINES GPIO  -  MODULO 4                          */
/*******************************************************************************************
 *  Que mide, por cada puerto (GPIOA, GPIOB, ...):
 *    - si el puerto tiene el reloj activado (si no, no se usa y sus registros leen 0)
 *    - nivel de cada pin (IDR), salida programada (ODR)
 *    - modo de cada pin (entrada, salida, funcion alternativa, analogico)
 *    - resistencias pull-up / pull-down, push-pull / open-drain y funcion alternativa (AF0..AF15)
 *  Solo LEE registros: no cambia nada del hardware.
 *
 *  Uso:   cada segundo, desde UNA tarea (p.ej. el hilo de red):  Modulo4_Gpio();
 *******************************************************************************************/

/* Puertos a mostrar, desde GPIOA. El STM32H753 tiene A..K (11), pero en el encapsulado
 * LQFP144 de la Nucleo solo estan cableados A..G y H0/H1: por defecto 8 (A..H).
 * Cada puerto son 8 variables en la tabla web.                                          */
#ifndef GPIOINFO_PORTS
#define GPIOINFO_PORTS  8
#endif

/* Modo de un pin (2 bits de MODER) */
#define PIN_ENTRADA    0
#define PIN_SALIDA     1
#define PIN_ALTERNATIVA 2
#define PIN_ANALOGICO  3

typedef struct {
  uint8_t  clk;        /* 1 = reloj del puerto activado (puerto en uso)              */
  uint16_t idr;        /* nivel real de cada pin (bit n = pin n)                     */
  uint16_t odr;        /* valor que se escribe en las salidas                        */
  uint16_t otyper;     /* bit n = 1: pin n open-drain, 0: push-pull                  */
  uint32_t moder;      /* 2 bits por pin: PIN_xxx                                    */
  uint32_t pupdr;      /* 2 bits por pin: 0 nada, 1 pull-up, 2 pull-down             */
  uint32_t afrl;       /* 4 bits por pin, pines 0..7: AF0..AF15                      */
  uint32_t afrh;       /* 4 bits por pin, pines 8..15                                */
} GpioPort_t;

typedef struct {
  uint8_t    ports_on;              /* puertos con reloj activado                      */
  GpioPort_t p[GPIOINFO_PORTS];     /* p[0] = GPIOA, p[1] = GPIOB, ...                 */
} GpioInfo_t;

extern volatile GpioInfo_t gpio_info;

void Modulo4_Gpio(void);         /* relee los registros (1 vez por segundo)         */

#endif
