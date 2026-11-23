/**
 * @file    tim3_pwm.c
 * @brief   TIM3 kanal 1 v načinu PWM (PWM mode 1) na pinu PB4.
 *
 * ===========================================================================
 *  KAKO DELUJE PWM
 * ===========================================================================
 *
 *   Časovna baza (PSC, ARR, CNT) je ista kot v tim3_timebase.c in določa
 *   FREKVENCO signala. Primerjalni register CCR1 določa RAZMERJE.
 *
 *   PWM mode 1, štetje navzgor:
 *        OC1REF = 1, dokler je CNT <  CCR1
 *        OC1REF = 0, ko je        CNT >= CCR1
 *
 *        CNT:   0 ......... CCR1 ......... ARR | 0 ......... CCR1 ...
 *               ┌──────────┐                   ┌──────────┐
 *   OC1REF:     │          │                   │          │
 *            ───┘          └───────────────────┘          └────────
 *               |<-- CCR1 -->|                  |
 *               |<------- ARR + 1 ------------->|
 *
 *   f_PWM  = f_UEV = f_CNT / (ARR + 1)
 *   duty   = CCR1 / (ARR + 1)
 *            CCR1 = 0       -> 0 %   (izhod ves čas 0)
 *            CCR1 > ARR     -> 100 % (izhod ves čas 1)
 *
 *   Ločljivost PWM je ARR + 1 korakov: večji ARR pomeni finejše nastavljanje
 *   razmerja, a pri isti f_CNT nižjo frekvenco.
 *
 *   PRIMERJAVA Z OUTPUT COMPARE (toggle):
 *     - OC toggle: izhod se preklopi ob ujemanju, frekvenca = f_UEV / 2,
 *                  razmerje vedno 50 %, CCR1 določa fazo.
 *     - PWM:       izhod se postavi ob UEV in pobriše ob ujemanju,
 *                  frekvenca = f_UEV, CCR1 določa razmerje.
 *
 * ===========================================================================
 *  UPORABLJENI REGISTRI IN BITI TIM3 (RM0433, poglavje o TIM2-TIM5)
 * ===========================================================================
 *
 *   CCMR1 (izhodni način, kanal 1)
 *         bita 1:0   CC1S   smer kanala (00 = izhod)
 *         bit 2      OC1FE  hitro omogočanje (za proženje s triggerjem)
 *         bit 3      OC1PE  medpomnjenje CCR1
 *         biti 6:4   OC1M[2:0]  način izhoda   } OC1M = {bit16, bit6:4}
 *         bit 16     OC1M[3]                   }
 *         bit 7      OC1CE  brisanje OC1REF z zunanjim signalom ETRF
 *
 *         OC1M:  0110 PWM mode 1 (aktivno, dokler CNT < CCR1)  <- tu
 *                0111 PWM mode 2 (neaktivno, dokler CNT < CCR1)
 *
 *   CCER  bit 0      CC1E   omogočanje izhoda OC1 na pin
 *         bit 1      CC1P   polarnost (0 = OC1 = OC1REF, 1 = invertirano)
 *         bit 3      CC1NP  mora biti 0, ko je kanal izhod
 *   CCR1  bita 15:0         vrednost primerjave = širina impulza v tikih
 *   EGR   bit 0      UG     programski UEV (prenos preload -> senčni)
 *   DIER  bit 1      CC1IE  prekinitev ob ujemanju (tu je ne potrebujemo)
 */

#include "TIM3_PWM.h"

void tim3_pwm_init(void)
{
    /* -----------------------------------------------------------------------
     * 1) CCER.CC1E = 0: IZHOD KANALA 1 IZKLOPLJEN
     *    Bita CC1S v CCMR1 lahko pišemo samo, ko je CC1E = 0.
     * --------------------------------------------------------------------- */
    TIM3->CCER &= ~(1u << 0);               /* CC1E = 0 */

    /* -----------------------------------------------------------------------
     * 2) CCMR1: KANAL 1 KOT IZHOD V NAČINU PWM MODE 1
     *
     *    b 1:0  CC1S  = 00    kanal 1 je izhod
     *    bit 2  OC1FE = 0     ni potrebno
     *    bit 3  OC1PE = 1     CCR1 JE medpomnjen: nova vrednost začne veljati
     *                         šele ob naslednjem UEV. Če bi CCR1 spremenili
     *                         sredi periode brez medpomnjenja, bi lahko
     *                         dobili en predolg ali prekratek impulz.
     *                         Referenčni priročnik za PWM medpomnjenje
     *                         izrecno zahteva (skupaj z ARPE = 1 v CR1,
     *                         ki je postavljen v tim3_timebase_init()).
     *    b 6:4  OC1M  = 110 } PWM mode 1 (OC1M = 0110)
     *    bit 16 OC1M3 = 0   }
     *    bit 7  OC1CE = 0     ETRF ne vpliva na OC1REF
     *
     *    Brišemo samo bite kanala 1 (biti 0..7 in 16); biti 8..15 in 24
     *    pripadajo kanalu 2 in jih ne spreminjamo.
     * --------------------------------------------------------------------- */
    uint32_t ccmr1 = TIM3->CCMR1;
    ccmr1 &= ~((0xFFu << 0) | (1u << 16));  /* pobriši vse bite kanala 1 */
    ccmr1 |=  (6u << 4)                     /* OC1M[2:0] = 110 -> PWM mode 1 */
           |  (1u << 3);                    /* OC1PE = 1 */
    TIM3->CCMR1 = ccmr1;

    /* -----------------------------------------------------------------------
     * 3) CCR1: ZAČETNO RAZMERJE
     *    Ker je OC1PE = 1, gre vrednost zaenkrat samo v preload register.
     * --------------------------------------------------------------------- */
    tim3_pwm_set_duty(TIM3_PWM_DUTY_INIT);

    /* -----------------------------------------------------------------------
     * 4) PREKINITEV OB UJEMANJU ONEMOGOČENA
     *    Za PWM CPE ne potrebujemo: izhod ustvarja strojna oprema.
     *    (Zastavica CC1IF se v SR še vedno postavlja ob vsakem ujemanju.)
     * --------------------------------------------------------------------- */
    TIM3->DIER &= ~(1u << 1);               /* CC1IE = 0 */

    /* -----------------------------------------------------------------------
     * 5) EGR.UG: PRENOS PRELOAD -> SENČNI REGISTER
     *    Brez tega bi prvi del periode tekel s CCR1 = 0 (izhod 0) do
     *    naslednjega UEV. UG prenese CCR1 takoj in začne novo periodo
     *    (CNT = 0). Ker je URS = 1, UG ne sproži prekinitve UEV.
     * --------------------------------------------------------------------- */
    TIM3->EGR = (1u << 0);                  /* UG = 1 */

    /* -----------------------------------------------------------------------
     * 6) CCER: OMOGOČIMO IZHOD NA PIN
     *
     *    bit 0  CC1E  = 1   signal OC1 gre na pin PB4
     *    bit 1  CC1P  = 0   aktivno visoko (OC1 = OC1REF)
     *    bit 3  CC1NP = 0   kanal je izhod
     *
     *    (TIM3 nima registra BDTR in bita MOE, ki ju potrebujeta TIM1/TIM8.)
     * --------------------------------------------------------------------- */
    TIM3->CCER = (TIM3->CCER & ~((1u << 1) | (1u << 3)))   /* CC1P = CC1NP = 0 */
               | (1u << 0);                                /* CC1E = 1 */
}

void tim3_pwm_set_duty(uint32_t permille)
{
    if (permille > 1000u)
    {
        permille = 1000u;
    }

    /* -----------------------------------------------------------------------
     * duty = CCR1 / (ARR + 1)   ->   CCR1 = duty * (ARR + 1)
     *
     * ARR preberemo iz registra, zato funkcija deluje pri katerikoli
     * nastavitvi časovne baze. Pri 1000 promilih je CCR1 = ARR + 1 > ARR,
     * kar da 100 % (izhod ves čas 1).
     * Produkt je največ 1000 * 65536 < 2^32, zato ni prekoračitve.
     *
     * CCR1 pri TIM3 je 16-biten: pri ARR = 0xFFFF bi ARR + 1 = 0x10000
     * vpisali kot 0 (0 % namesto 100 %). Zato vrednost omejimo na 0xFFFF.
     * --------------------------------------------------------------------- */
    uint32_t period = TIM3->ARR + 1u;
    uint32_t ccr = (permille * period) / 1000u;
    if (ccr > 0xFFFFu)
    {
        ccr = 0xFFFFu;
    }
    TIM3->CCR1 = ccr;
}
