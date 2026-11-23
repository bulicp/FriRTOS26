/**
 * @file    tim3_timebase.c
 * @brief   Časovna baza s TIM3 in prekinitvijo ob dogodku posodobitve (UEV).
 *
 * ===========================================================================
 *  KAKO DELUJE ČASOVNA BAZA
 * ===========================================================================
 *
 *   f_TIM ──► [ PSC ] ──► f_CNT ──► [ CNT: 0, 1, 2, ..., ARR ] ──► UEV
 *            (delilnik)             (števec navzgor)          (prekoračitev)
 *
 *   - PSC (prescaler) deli vhodno uro:     f_CNT = f_TIM / (PSC + 1)
 *   - CNT šteje od 0 do ARR, nato se vrne na 0 in sproži
 *     dogodek posodobitve UEV (update event).
 *   - Frekvenca UEV:                       f_UEV = f_CNT / (ARR + 1)
 *
 *   Skupaj:   f_UEV = f_TIM / ((PSC + 1) * (ARR + 1))
 *
 *   Primer:   f_TIM = 240 MHz, PSC = 23999, ARR = 9999
 *             f_CNT = 240 MHz / 24000 = 10 kHz   (en tik = 100 us)
 *             f_UEV = 10 kHz / 10000   = 1 Hz
 *
 *   Ob UEV strojna oprema postavi zastavico UIF v TIM3->SR. Če je v
 *   TIM3->DIER postavljen bit UIE, zastavica UIF sproži prekinitev TIM3_IRQn.
 *
 * ===========================================================================
 *  UPORABLJENI REGISTRI IN BITI TIM3 (RM0433, poglavje o TIM2-TIM5)
 * ===========================================================================
 *
 *   CR1   bit 0      CEN    vklop števca
 *         bit 1      UDIS   prepoved dogodka UEV
 *         bit 2      URS    vir prekinitve UEV (1 = samo prekoračitev)
 *         bit 3      OPM    enkratni način (one-pulse)
 *         bit 4      DIR    smer štetja (0 = navzgor)
 *         bita 6:5   CMS    način štetja (00 = edge-aligned)
 *         bit 7      ARPE   medpomnjenje ARR
 *   SMCR  bita 2:0   SMS    slave mode (000 = notranja ura f_TIM)
 *   PSC   bita 15:0         delilnik ure
 *   ARR   bita 15:0         vrednost, do katere šteje števec
 *   CNT   bita 15:0         števec
 *   EGR   bit 0      UG     programsko proženje dogodka UEV
 *   SR    bit 0      UIF    zastavica dogodka UEV
 *   DIER  bit 0      UIE    omogočanje prekinitve ob UEV
 */

#include "TIM3_timebase.h"

void tim3_timebase_init(void)
{
    /* -----------------------------------------------------------------------
     * 1) URA IN RESET (HAL)
     *    Brez ure so registri TIM3 "mrtvi": pisanje nima učinka, branje vrne 0.
     *    Reset postavi vse registre TIM3 v začetno stanje, ne glede na to,
     *    kaj je bilo nastavljeno pred tem (npr. pri ponovnem zagonu iz debuggerja).
     * --------------------------------------------------------------------- */
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM3_FORCE_RESET();
    __HAL_RCC_TIM3_RELEASE_RESET();

    /* -----------------------------------------------------------------------
     * 2) CR1: NAČIN ŠTETJA (števec mora biti med nastavljanjem ustavljen)
     *
     *    bit 0  CEN  = 0   števec ustavljen (vklopimo ga šele na koncu)
     *    bit 1  UDIS = 0   dogodek UEV je dovoljen
     *    bit 2  URS  = 1   prekinitev UEV sproži SAMO prekoračitev števca,
     *                      ne pa programski UEV prek EGR.UG (glej korak 5)
     *    bit 3  OPM  = 0   števec teče neprekinjeno
     *    bit 4  DIR  = 0   štetje navzgor: 0 -> ARR, nato UEV in nazaj na 0
     *    b 6:5  CMS  = 00  "edge-aligned" način (smer določa DIR)
     *    bit 7  ARPE = 1   ARR je medpomnjen: nova vrednost ARR začne veljati
     *                      šele ob naslednjem UEV, zato perioda med
     *                      delovanjem ne "poskoči"
     * --------------------------------------------------------------------- */
    TIM3->CR1 = (1u << 7)       /* ARPE = 1 */
              | (1u << 2);      /* URS  = 1 */

    /* -----------------------------------------------------------------------
     * 3) SMCR: VIR URE
     *    SMS (biti 2:0) = 000: slave mode izklopljen, števec šteje notranjo
     *    uro f_TIM. (Po resetu je že 0; zapišemo zaradi jasnosti.)
     * --------------------------------------------------------------------- */
    TIM3->SMCR = 0;

    /* -----------------------------------------------------------------------
     * 4) PSC, ARR, CNT: PERIODA
     *    f_CNT = f_TIM / (PSC + 1)
     *    f_UEV = f_CNT / (ARR + 1)
     *    Vrednosti sta izračunani v tim3_timebase.h.
     * --------------------------------------------------------------------- */
    TIM3->PSC = TIM3_PSC_VALUE;
    TIM3->ARR = TIM3_ARR_VALUE;
    TIM3->CNT = 0;

    /* -----------------------------------------------------------------------
     * 5) EGR.UG (bit 0): PROGRAMSKI UEV
     *
     *    PSC je VEDNO medpomnjen, ARR pa tudi, ker je ARPE = 1. Vpisani
     *    vrednosti sta zaenkrat samo v "preload" registrih. Timer dejansko
     *    uporablja senčne (shadow) registre, ki imajo po resetu
     *    PSC = 0 in ARR = 0xFFFF. Preload se v senčni register prenese šele
     *    ob dogodku UEV.
     *
     *    Brez tega koraka bi prva perioda trajala 65536 / 240 MHz = 273 us
     *    namesto 1 s in prva prekinitev bi prišla prezgodaj. Šele ta prvi
     *    UEV bi naložil PSC in ARR, od tam naprej bi bilo vse pravilno.
     *
     *    UG sproži UEV programsko: preload -> senčni registri, CNT = 0.
     *    Ker je URS = 1, ta UEV ne postavi UIF in ne sproži prekinitve.
     *    (Enako naredi HAL v TIM_Base_SetConfig().)
     * --------------------------------------------------------------------- */
    TIM3->EGR = (1u << 0);      /* UG = 1 */

    /* -----------------------------------------------------------------------
     * 6) SR: POBRIŠEMO MOREBITNE ZASTAVICE
     *
     *    Biti v SR so tipa "rc_w0": z zapisom 0 bit pobrišemo, zapis 1 nima
     *    učinka. Zato brišemo z navadnim prirejanjem (=), nikoli z &=.
     *    (Z URS = 1 bi moral biti UIF že 0; brisanje je samo varovalka.)
     * --------------------------------------------------------------------- */
    TIM3->SR = 0;

    /* -----------------------------------------------------------------------
     * 7) DIER.UIE (bit 0): OMOGOČIMO PREKINITEV OB UEV
     *    Od zdaj UIF = 1 pomeni zahtevo za prekinitev TIM3_IRQn.
     * --------------------------------------------------------------------- */
    TIM3->DIER = (1u << 0);     /* UIE = 1 */

    /* -----------------------------------------------------------------------
     * 8) NVIC (HAL): prioriteta in omogočanje TIM3_IRQn
     *    DIER.UIE omogoči zahtevo v timerju, NVIC jo mora še spustiti do jedra.
     * --------------------------------------------------------------------- */
    HAL_NVIC_SetPriority(TIM3_IRQn, TIM3_IRQ_PRIO, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    /* -----------------------------------------------------------------------
     * 9) CR1.CEN (bit 0): ZAŽENEMO ŠTEVEC
     *    Ostale bite CR1 ohranimo, zato tu uporabimo |=.
     * --------------------------------------------------------------------- */
    TIM3->CR1 |= (1u << 0);     /* CEN = 1 */
}
