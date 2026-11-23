/**
 * @file    tim3_oc.h
 * @brief   TIM3 kanal 1 v načinu Output Compare (toggle) na pinu PB4.
 *
 *          Nastavitev registrov TIM3 je napisana "bare-metal".
 *          PB4 mora biti že nastavljen kot alternativna funkcija AF2 (TIM3_CH1).
 *
 *          VRSTNI RED KLICEV:
 *              tim3_timebase_init();   // najprej: reset TIM3, PSC, ARR, zagon
 *              tim3_oc_init();         // potem:   kanal 1
 *          (tim3_timebase_init() resetira TIM3 in bi pobrisal nastavitve kanala.)
 */
#ifndef TIM3_OC_H
#define TIM3_OC_H

#include "TIM3_timebase.h"

/* ------------------------------------------------------------------------- */
/*  Parametri                                                                 */
/* ------------------------------------------------------------------------- */

/**
 * Vrednost primerjave CCR1.
 *
 * Ko števec CNT doseže CCR1, se izhod OC1 (PB4) preklopi. To se zgodi enkrat
 * na periodo, z zamikom CCR1 / f_CNT za dogodkom UEV.
 * Primer: CCR1 = 2500 pri f_CNT = 10 kHz -> preklop 250 ms za vsakim UEV.
 *
 * Frekvenca signala na PB4 je zato f_UEV / 2 (en preklop na periodo).
 */
#define TIM3_OC1_CCR       2500u

/** 1 = ob ujemanju CNT == CCR1 sproži še prekinitev (zastavica CC1IF). */
#define TIM3_OC1_IRQ       1

/* CCR1 mora biti <= ARR, sicer do ujemanja nikoli ne pride. */
_Static_assert(TIM3_OC1_CCR <= TIM3_ARR_VALUE, "TIM3: CCR1 je večji od ARR");

/* ------------------------------------------------------------------------- */
/*  Vmesnik                                                                   */
/* ------------------------------------------------------------------------- */

/**
 * @brief  Nastavi TIM3 kanal 1 kot Output Compare v načinu toggle.
 *         Pred tem mora biti klican tim3_timebase_init().
 */
void tim3_oc_init(void);

#endif /* TIM3_OC_H */
