/**
 * @file    tim3_pwm.h
 * @brief   TIM3 kanal 1 v načinu PWM (PWM mode 1) na pinu PB4.
 *
 *          Nastavitev registrov TIM3 je napisana "bare-metal".
 *          PB4 mora biti nastavljen kot alternativna funkcija AF2 (TIM3_CH1),
 *          npr. s HAL_TIM_MspPostInit(&htim3).
 *
 *          VRSTNI RED KLICEV:
 *              tim3_timebase_init();   // najprej: reset TIM3, PSC, ARR, zagon
 *              tim3_pwm_init();        // potem:   kanal 1 v načinu PWM
 *              HAL_TIM_MspPostInit(&htim3);   // PB4 -> AF2
 *
 *          tim3_oc_init() in tim3_pwm_init() sta alternativi za ISTI kanal:
 *          kliči samo eno od njiju.
 *
 *          FREKVENCA PWM = f_UEV iz TIM3_timebase.h. Za zatemnjevanje LED
 *          naj bo vsaj nekaj 100 Hz, npr. v TIM3_timebase.h nastavi
 *          (pri f_TIM = 64 MHz):
 *              TIM3_CNT_HZ = 1000000   (1 MHz -> PSC = 63)
 *              TIM3_UEV_HZ = 1000      (1 kHz -> ARR = 999, 1000 korakov)
 *          Pri 1 Hz (privzeto) PWM vidiš kot počasno utripanje z različno
 *          dolgim prižganim delom.
 */
#ifndef TIM3_PWM_H
#define TIM3_PWM_H

#include "TIM3_timebase.h"

/* ------------------------------------------------------------------------- */
/*  Parametri                                                                 */
/* ------------------------------------------------------------------------- */

/** Začetno razmerje (duty cycle) v promilih: 0 ... 1000 (250 = 25 %). */
#define TIM3_PWM_DUTY_INIT   250u

/* ------------------------------------------------------------------------- */
/*  Vmesnik                                                                   */
/* ------------------------------------------------------------------------- */

/**
 * @brief  Nastavi TIM3 kanal 1 kot PWM izhod (PWM mode 1, aktivno visoko).
 *         Pred tem mora biti klican tim3_timebase_init().
 */
void tim3_pwm_init(void);

/**
 * @brief  Nastavi razmerje PWM.
 * @param  permille  razmerje v promilih, 0 ... 1000 (večje vrednosti = 1000)
 *
 * Nova vrednost začne veljati ob naslednjem UEV (CCR1 je medpomnjen),
 * zato med periodo ni motenj na izhodu.
 */
void tim3_pwm_set_duty(uint32_t permille);

#endif /* TIM3_PWM_H */