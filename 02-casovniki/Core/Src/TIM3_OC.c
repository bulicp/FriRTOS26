/**
 * @file    tim3_oc.c
 * @brief   TIM3 kanal 1 v načinu Output Compare (toggle) na pinu PB4.
 *
 * ===========================================================================
 *  KAKO DELUJE OUTPUT COMPARE
 * ===========================================================================
 *
 *   Časovna baza (PSC, ARR, CNT) je ista kot v tim3_timebase.c.
 *   Vsak kanal ima še svoj primerjalni register CCRx. Primerjalnik ves čas
 *   primerja CNT s CCR1:
 *
 *        CNT:  0 ... CCR1 ... ARR | 0 ... CCR1 ... ARR | 0 ...
 *                      ▲          ▲         ▲          ▲
 *                   ujemanje     UEV     ujemanje     UEV
 *
 *   Ob ujemanju (CNT == CCR1):
 *     - se interni signal OC1REF spremeni glede na način OC1M
 *       (toggle: OC1REF se preklopi),
 *     - strojna oprema postavi zastavico CC1IF v SR (VEDNO, ne glede na
 *       to, ali je prekinitev omogočena),
 *     - če je v DIER postavljen CC1IE, se sproži prekinitev TIM3_IRQn.
 *
 *   OC1REF gre skozi izhodno stopnjo (polarnost CC1P, omogočanje CC1E)
 *   na pin OC1 = PB4 (AF2).
 *
 *   V načinu toggle se izhod preklopi enkrat na periodo, zato je signal
 *   na PB4 pravokoten s frekvenco f_UEV / 2 in razmerjem 50 %.
 *   CCR1 določa samo FAZNI zamik glede na UEV, ne frekvence.
 *
 *   Celotno delo opravi strojna oprema: CPE za preklapljanje pina ne
 *   izvede niti enega ukaza.
 *
 * ===========================================================================
 *  UPORABLJENI REGISTRI IN BITI TIM3 (RM0433, poglavje o TIM2-TIM5)
 * ===========================================================================
 *
 *   CCMR1 (izhodni način, kanal 1)
 *         bita 1:0   CC1S   smer kanala (00 = izhod)
 *         bit 2      OC1FE  hitro omogočanje (samo za PWM/one-pulse)
 *         bit 3      OC1PE  medpomnjenje CCR1
 *         biti 6:4   OC1M[2:0]  način izhoda   } OC1M je 4-biten:
 *         bit 16     OC1M[3]                   } OC1M = {bit16, bit6:4}
 *         bit 7      OC1CE  brisanje OC1REF z zunanjim signalom ETRF
 *
 *         OC1M:  0000 frozen      (CNT == CCR1 ne vpliva na izhod)
 *                0001 active      (ob ujemanju OC1REF = 1)
 *                0010 inactive    (ob ujemanju OC1REF = 0)
 *                0011 toggle      (ob ujemanju se OC1REF preklopi)  <- tu
 *                0100 force inactive, 0101 force active
 *                0110 PWM mode 1, 0111 PWM mode 2
 *
 *   CCER  bit 0      CC1E   omogočanje izhoda OC1 na pin
 *         bit 1      CC1P   polarnost (0 = OC1 = OC1REF, 1 = invertirano)
 *         bit 3      CC1NP  mora biti 0, ko je kanal izhod
 *   CCR1  bita 15:0         vrednost primerjave
 *   DIER  bit 1      CC1IE  omogočanje prekinitve ob ujemanju
 *   SR    bit 1      CC1IF  zastavica ujemanja CNT == CCR1
 */

#include "TIM3_OC.h"

void tim3_oc_init(void)
{
    /* -----------------------------------------------------------------------
     * 1) CCER.CC1E = 0: IZHOD KANALA 1 IZKLOPLJEN
     *    Bita CC1S v CCMR1 lahko pišemo samo, ko je CC1E = 0.
     *    Po tim3_timebase_init() (reset TIM3) je CC1E že 0, a za vsak primer
     *    ga izrecno pobrišemo.
     * --------------------------------------------------------------------- */
    TIM3->CCER &= ~(1u << 0);               /* CC1E = 0 */

    /* -----------------------------------------------------------------------
     * 2) CCMR1: KANAL 1 KOT IZHOD V NAČINU TOGGLE
     *
     *    b 1:0  CC1S  = 00    kanal 1 je izhod
     *    bit 2  OC1FE = 0     ni potrebno (samo za PWM/one-pulse)
     *    bit 3  OC1PE = 0     CCR1 NI medpomnjen: nova vrednost CCR1 velja
     *                         takoj. Tako lahko v prekinitvi premaknemo
     *                         naslednje ujemanje (CCR1 += korak).
     *    b 6:4  OC1M  = 011 } toggle (OC1M = 0011)
     *    bit 16 OC1M3 = 0   }
     *    bit 7  OC1CE = 0     ETRF ne vpliva na OC1REF
     *
     *    Brišemo samo bite kanala 1 (biti 0..7 in 16); biti 8..15 in 24
     *    pripadajo kanalu 2 in jih ne spreminjamo.
     * --------------------------------------------------------------------- */
    uint32_t ccmr1 = TIM3->CCMR1;
    ccmr1 &= ~((0xFFu << 0) | (1u << 16));  /* pobriši vse bite kanala 1 */
    ccmr1 |=  (3u << 4);                    /* OC1M[2:0] = 011 -> toggle */
    TIM3->CCMR1 = ccmr1;

    /* -----------------------------------------------------------------------
     * 3) CCR1: TRENUTEK PREKLOPA ZNOTRAJ PERIODE
     *    Izhod se preklopi, ko je CNT == CCR1, tj. CCR1 / f_CNT po UEV.
     * --------------------------------------------------------------------- */
    TIM3->CCR1 = TIM3_OC1_CCR;

    /* -----------------------------------------------------------------------
     * 4) PREKINITEV OB UJEMANJU (neobvezno)
     *    Najprej pobrišemo CC1IF (rc_w0: zapis 0 briše, zapis 1 nima učinka),
     *    da ne dobimo takojšnje prekinitve zaradi starega ujemanja.
     *    Nato v DIER postavimo CC1IE. Ostalih bitov DIER (npr. UIE iz
     *    časovne baze) ne spreminjamo, zato |=.
     *    NVIC za TIM3_IRQn je že omogočen v tim3_timebase_init().
     * --------------------------------------------------------------------- */

    TIM3->SR    = ~(1u << 1);               /* pobriši CC1IF */
    TIM3->DIER |=  (1u << 1);               /* CC1IE = 1 */


    /* -----------------------------------------------------------------------
     * 5) CCER: OMOGOČIMO IZHOD NA PIN
     *
     *    bit 0  CC1E  = 1   signal OC1 gre na pin PB4
     *    bit 1  CC1P  = 0   brez invertiranja (OC1 = OC1REF)
     *    bit 3  CC1NP = 0   kanal je izhod
     *
     *    Začetni nivo: po resetu TIM3 je OC1REF = 0, zato je PB4 na začetku
     *    nizek, prvi preklop ga postavi na 1.
     *    (General-purpose timer TIM3 nima registra BDTR in bita MOE,
     *    ki ju potrebujeta TIM1/TIM8.)
     * --------------------------------------------------------------------- */
    TIM3->CCER = (TIM3->CCER & ~((1u << 1) | (1u << 3)))   /* CC1P = CC1NP = 0 */
               | (1u << 0);                                /* CC1E = 1 */
}


