/**
 * @file    TIM3_timebase.h
 * @brief   Časovna baza s TIM3 in prekinitvijo ob dogodku posodobitve (UEV).
 *
 *          Nastavitev registrov TIM3 je napisana "bare-metal" (neposredno
 *          v registre). Ura (RCC) in NVIC sta nastavljena s HAL.
 */
#ifndef TIM3_TIMEBASE_H
#define TIM3_TIMEBASE_H

#include "stm32h7xx_hal.h"

/* ------------------------------------------------------------------------- */
/*  Parametri časovne baze                                                    */
/* ------------------------------------------------------------------------- */

/**
 * Frekvenca ure, ki pride do TIM3 (f_TIM).
 *
 * TIM3 je na vodilu APB1 (domena D2). Vrednost preberi v CubeMX:
 * Clock Configuration -> "APB1 Timer clocks".
 * Pri jedru 480 MHz, HCLK 240 MHz in APB1 = HCLK/2 je f_TIM = 240 MHz
 * (ura timerjev se podvoji, kadar je delilnik APB1 večji od 1).
 */
#define TIM3_CLK_HZ        64000000u

/** Želena frekvenca štetja števca CNT (f_CNT): en "tik" = 100 us. */
#define TIM3_CNT_HZ        10000u

/** Želena frekvenca dogodka posodobitve (UEV), tj. prekinitev: 1 Hz. */
#define TIM3_UEV_HZ        1u

/** Prioriteta prekinitve TIM3 v NVIC (0 = najvišja). */
#define TIM3_IRQ_PRIO      5u

/* Izračunani vrednosti za PSC in ARR (glej razlago v TIM3_timebase.c). */
#define TIM3_PSC_VALUE     (TIM3_CLK_HZ / TIM3_CNT_HZ - 1u)
#define TIM3_ARR_VALUE     (TIM3_CNT_HZ / TIM3_UEV_HZ - 1u)

/* TIM3 je 16-biten: PSC in ARR morata biti <= 65535. Preverimo ob prevajanju. */
_Static_assert(TIM3_PSC_VALUE <= 0xFFFFu, "TIM3: PSC presega 16 bitov");
_Static_assert(TIM3_ARR_VALUE <= 0xFFFFu, "TIM3: ARR presega 16 bitov");
_Static_assert(TIM3_CLK_HZ % TIM3_CNT_HZ == 0u, "TIM3: f_TIM ni deljiva s f_CNT");
_Static_assert(TIM3_CNT_HZ % TIM3_UEV_HZ == 0u, "TIM3: f_CNT ni deljiva s f_UEV");

/* ------------------------------------------------------------------------- */
/*  Vmesnik                                                                   */
/* ------------------------------------------------------------------------- */

/**
 * @brief  Nastavi TIM3 kot časovno bazo, omogoči prekinitev UEV in zažene števec.
 *
 * Po klicu se TIM3_IRQHandler() izvede s frekvenco TIM3_UEV_HZ.
 * Handler mora pobrisati zastavico UIF v TIM3->SR.
 */
void tim3_timebase_init(void);

#endif /* TIM3_TIMEBASE_H */
