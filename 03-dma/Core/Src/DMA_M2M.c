/**
 * @file    DMA_M2M.c
 * @brief   Prenos pomnilnik -> pomnilnik z DMA2, tok (stream) 0.
 *
 * ===========================================================================
 *  KAKO DELUJE DMA PRENOS POMNILNIK -> POMNILNIK
 * ===========================================================================
 *
 *   Brez DMA kopira CPE:   for (i = 0; i < n; i++) dst[i] = src[i];
 *   Vsako besedo prebere v register in jo zapiše nazaj; ves ta čas CPE ne
 *   more delati ničesar drugega.
 *
 *   Z DMA CPE samo NASTAVI prenos (od kod, kam, koliko) in ga zažene.
 *   Krmilnik DMA sam bere in piše prek vodila, CPE pa medtem dela naprej.
 *   Ob koncu DMA postavi zastavico TCIF in (če je TCIE = 1) sproži prekinitev.
 *
 *        CPE                     DMA2 (tok 0)                    pomnilnik
 *         │  PAR, M0AR, NDTR, CR   │                                 │
 *         ├───────────────────────►│                                 │
 *         │  EN = 1                │  beri  src[0]  ◄────────────────┤
 *         │                        │  piši  dst[0]  ────────────────►│
 *         │  (prosta za drugo delo)│  beri  src[1]  ◄────────────────┤
 *         │                        │  ...           NDTR se zmanjšuje│
 *         │◄───────────────────────┤  NDTR = 0: TCIF -> prekinitev   │
 *
 *   PB4:  ──┐ EN=1                                        IRQ ┌──
 *           └─────────────────────────────────────────────────┘
 *   (v kodi PB4 = 1 ob začetku in PB4 = 0 ob koncu, torej impulz navzgor)
 *
 *   V načinu memory-to-memory:
 *     - "periferna" vrata (PAR) so IZVOR, pomnilniška vrata (M0AR) so CILJ,
 *     - prenos se začne takoj, ko postavimo EN (ni zahtev periferne naprave,
 *       zato DMAMUX tu ne izbira nobene zahteve),
 *     - neposredni način (direct mode) in krožni način NISTA dovoljena:
 *       podatki gredo vedno skozi FIFO (4 besede = 16 B).
 *
 * ===========================================================================
 *  KJE SO LAHKO PODATKI (STM32H753, RM0433, poglavje o arhitekturi vodil)
 * ===========================================================================
 *
 *     0x2000 0000  DTCM  128 KB   SAMO CPE (in MDMA)    <- .data, .bss, sklad
 *     0x2400 0000  AXI SRAM 512 KB  DMA1/DMA2 DA           <- naši medpomnilniki
 *     0x3000 0000  SRAM1..3 288 KB  DMA1/DMA2 DA
 *     0x3800 0000  SRAM4     64 KB  BDMA (in DMA1/DMA2)
 *
 *   Če bi bil src ali dst v DTCM, bi DMA2 javil napako prenosa (TEIF0).
 *   POSKUS: odstrani DMA_BUFFER pri enem od medpomnilnikov v main.c in
 *   opazuj dma_m2m_error.
 *
 *   PREDPOMNILNIK: v tem projektu D-cache ni vklopljen. Če bi ga vklopili,
 *   bi morali pred prenosom izvor zapisati iz predpomnilnika v pomnilnik
 *   (SCB_CleanDCache_by_Addr), po prenosu pa cilj razveljaviti
 *   (SCB_InvalidateDCache_by_Addr), sicer CPE bere stare podatke.
 *
 * ===========================================================================
 *  UPORABLJENI REGISTRI DMA2 (RM0433, poglavje DMA controller)
 * ===========================================================================
 *
 *   S0CR  (konfiguracija toka 0)
 *         bit 0      EN      vklop toka (HW ga pobriše ob koncu prenosa)
 *         bit 1      DMEIE   prekinitev ob napaki direktnega načina
 *         bit 2      TEIE    prekinitev ob napaki prenosa
 *         bit 3      HTIE    prekinitev ob polovici prenosa
 *         bit 4      TCIE    prekinitev ob koncu prenosa
 *         bit 5      PFCTRL  periferija kot krmilnik pretoka (0 = DMA)
 *         bita 7:6   DIR     smer: 00 P->M, 01 M->P, 10 M->M
 *         bit 8      CIRC    krožni način
 *         bit 9      PINC    povečevanje naslova na strani PAR
 *         bit 10     MINC    povečevanje naslova na strani M0AR
 *         bita 12:11 PSIZE   velikost podatka na strani PAR (00 B, 01 HW, 10 W)
 *         bita 14:13 MSIZE   velikost podatka na strani M0AR
 *         bita 17:16 PL      prioriteta toka
 *         bita 22:21 PBURST  burst na strani PAR (00 single, 01 INCR4, ...)
 *         bita 24:23 MBURST  burst na strani M0AR
 *   S0NDTR bita 15:0  število podatkov (v enotah PSIZE), se med prenosom zmanjšuje
 *   S0PAR            izvorni naslov (pri M->M)
 *   S0M0AR           ciljni naslov  (pri M->M)
 *   S0FCR bita 1:0   FTH     prag FIFO (11 = poln FIFO, 4 besede)
 *         bit 2      DMDIS   1 = FIFO način (direktni način izklopljen)
 *   LISR  bit 5      TCIF0   prenos toka 0 končan
 *         bit 3      TEIF0   napaka prenosa toka 0
 *   LIFCR            brisanje zastavic (pisanje 1 pobriše isto mesto v LISR)
 *
 *   DMAMUX1 kanal 8 pripada DMA2 toku 0 (kanali 0..7 = DMA1, 8..15 = DMA2).
 *
 * ===========================================================================
 *  DWT (Data Watchpoint and Trace) - ŠTEVEC URINIH CIKLOV
 * ===========================================================================
 *
 *   DEMCR bit 24     TRCENA     vklop enot DWT/ITM (sicer DWT "spi")
 *   DWT_LAR          0xC5ACCE55 odklep registrov DWT (zahteva Cortex-M7)
 *   DWT_CTRL bit 0   CYCCNTENA  vklop števca ciklov
 *   DWT_CYCCNT                  32-bitni števec, +1 vsak cikel ure CPE
 *
 *   Pri f_CPE = 64 MHz (HSI) je en cikel 15,625 ns:
 *        t [us] = cycles / 64
 */

#include "DMA_M2M.h"

/* ---------------------------------------------------------------------------
 * Zastavice toka 0 v LISR / LIFCR
 *
 *   DMA2->LISR (Low Interrupt Status Register, toki 0..3) lahko samo BEREMO.
 *   Zastavico pobrišemo tako, da na ISTI bit v DMA2->LIFCR zapišemo 1
 *   (pisanje 0 nima učinka, zato drugih zastavic ne pokvarimo).
 *   Zastavica se postavi ne glede na to, ali je prekinitev omogočena;
 *   prekinitev sproži samo, če je v SxCR vklopljen pripadajoči bit ...IE.
 *   Če zastavice v ISR ne pobrišemo, se prekinitev ponavlja v nedogled,
 *   nov prenos pa se ne zažene.
 * ------------------------------------------------------------------------- */
#define DMA_S0_FEIF   (1u << 0)   /* FIFO error: pre-/podkoračitev FIFO      */
#define DMA_S0_DMEIF  (1u << 2)   /* napaka direktnega načina                */

/* TEIF0 - Transfer Error: napaka na vodilu med prenosom. DMA je dostopal do
 *   naslova, ki ga ne doseže ali ne obstaja (npr. src/dst v DTCM
 *   0x20000000). Strojna oprema tok TAKOJ ustavi (pobriše EN), prenos
 *   ostane nedokončan. Prekinitev sproži, ker je v CR vklopljen TEIE. */
#define DMA_S0_TEIF   (1u << 3)

#define DMA_S0_HTIF   (1u << 4)   /* Half Transfer: prenesena polovica NDTR  */

/* TCIF0 - Transfer Complete: prenos je končan, NDTR je padel na 0.
 *   Strojna oprema ob tem sama pobriše EN (pri M->M ni krožnega načina).
 *   Prekinitev sproži, ker je v CR vklopljen TCIE. */
#define DMA_S0_TCIF   (1u << 5)
#define DMA_S0_ALL    (DMA_S0_FEIF | DMA_S0_DMEIF | DMA_S0_TEIF | DMA_S0_HTIF | DMA_S0_TCIF)

/* PB4: postavi / pobriši prek BSRR (atomarno, brez branja-spremembe-pisanja) */
#define PB4_HIGH()    (GPIOB->BSRR = (1u << 4))
#define PB4_LOW()     (GPIOB->BSRR = (1u << (4 + 16)))

volatile uint32_t dma_m2m_done   = 1u;   /* na začetku ni prenosa v teku */
volatile uint32_t dma_m2m_error  = 0u;
volatile uint32_t dma_m2m_cycles = 0u;

static volatile uint32_t t_start;        /* DWT->CYCCNT ob zagonu prenosa */

static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   /* vklop DWT */
    DWT->LAR = 0xC5ACCE55u;                           /* odklep (Cortex-M7) */
    DWT->CYCCNT = 0u;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;              /* števec teče */
}

static void pb4_init(void)
{
    /* -----------------------------------------------------------------------
     * PB4 kot navaden izhod (v prejšnjem projektu je bil AF2 = TIM3_CH1).
     *
     *   MODER   bita 9:8  = 01  splošni izhod
     *   OTYPER  bit 4     = 0   push-pull
     *   OSPEEDR bita 9:8  = 11  zelo hitri fronti (lepše na osciloskopu)
     *   PUPDR   bita 9:8  = 00  brez uporov
     * --------------------------------------------------------------------- */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
    (void)RCC->AHB4ENR;                     /* počakamo, da ura res steče */

    PB4_LOW();
    GPIOB->MODER   = (GPIOB->MODER   & ~(3u << 8)) | (1u << 8);
    GPIOB->OTYPER &= ~(1u << 4);
    GPIOB->OSPEEDR |=  (3u << 8);
    GPIOB->PUPDR  &= ~(3u << 8);
}

void dma_m2m_init(void)
{
    /* -----------------------------------------------------------------------
     * 1) URA IN RESET DMA2
     * --------------------------------------------------------------------- */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    (void)RCC->AHB1ENR;
    RCC->AHB1RSTR |=  RCC_AHB1RSTR_DMA2RST;
    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_DMA2RST;

    /* -----------------------------------------------------------------------
     * 2) DMAMUX1 KANAL 8 (= DMA2 tok 0): brez zahteve periferije
     *    DMAREQ_ID = 0. Pri M->M prenos začne že bit EN, zahteva ni potrebna.
     * --------------------------------------------------------------------- */
    DMAMUX1_Channel8->CCR = 0u;

    pb4_init();
    dwt_init();

    /* -----------------------------------------------------------------------
     * 3) PREKINITEV DMA2_Stream0_IRQn v NVIC
     * --------------------------------------------------------------------- */
    NVIC_SetPriority(DMA2_Stream0_IRQn, 5u);
    NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

void dma_m2m_start(const uint32_t *src, uint32_t *dst, uint32_t n_words)
{
    if (n_words == 0u || n_words > DMA_M2M_MAX_ITEMS)
    {
        return;
    }

    /* -----------------------------------------------------------------------
     * 1) TOK MORA BITI IZKLOPLJEN
     *    Registre toka lahko pišemo samo, ko je EN = 0. Če je bil tok
     *    vklopljen, EN pobrišemo in POČAKAMO, da ga strojna oprema res
     *    izklopi (konča trenutni podatek).
     * --------------------------------------------------------------------- */
    DMA2_Stream0->CR &= ~DMA_SxCR_EN;
    while (DMA2_Stream0->CR & DMA_SxCR_EN) { }

    /* -----------------------------------------------------------------------
     * 2) POBRIŠEMO VSE STARE ZASTAVICE TOKA 0
     *    Če bi ostala postavljena npr. TCIF0 iz prejšnjega prenosa, se tok
     *    ne bi zagnal.
     * --------------------------------------------------------------------- */
    DMA2->LIFCR = DMA_S0_ALL;

    /* -----------------------------------------------------------------------
     * 3) NASLOVA IN ŠTEVILO PODATKOV
     *    Pri M->M: PAR = izvor, M0AR = cilj. NDTR šteje v enotah PSIZE
     *    (tu 32-bitne besede).
     * --------------------------------------------------------------------- */
    DMA2_Stream0->PAR  = (uint32_t)src;
    DMA2_Stream0->M0AR = (uint32_t)dst;
    DMA2_Stream0->NDTR = n_words;

    /* -----------------------------------------------------------------------
     * 4) FCR: FIFO NAČIN
     *    bit 2      DMDIS = 1   direktni način izklopljen (pri M->M obvezno)
     *    bita 1:0   FTH   = 11  prag = poln FIFO (4 besede); združljivo z
     *                           INCR4 pri MSIZE = 32 bitov
     * --------------------------------------------------------------------- */
    DMA2_Stream0->FCR = DMA_SxFCR_DMDIS | (3u << DMA_SxFCR_FTH_Pos);

    /* -----------------------------------------------------------------------
     * 5) CR: NAČIN PRENOSA (EN še vedno 0)
     *
     *    b 7:6   DIR    = 10   memory-to-memory
     *    bit 9   PINC   = 1    izvorni naslov se povečuje
     *    bit 10  MINC   = 1    ciljni naslov se povečuje
     *    b 12:11 PSIZE  = 10   32 bitov
     *    b 14:13 MSIZE  = 10   32 bitov
     *    b 17:16 PL            prioriteta (DMA_M2M_PRIORITY)
     *    b 22:21 PBURST        single ali INCR4 (DMA_M2M_BURST_INCR4)
     *    b 24:23 MBURST        single ali INCR4
     *    bit 4   TCIE   = 1    prekinitev ob koncu prenosa
     *    bit 2   TEIE   = 1    prekinitev ob napaki prenosa
     * --------------------------------------------------------------------- */
    uint32_t cr = (2u << DMA_SxCR_DIR_Pos)
                | DMA_SxCR_PINC
                | DMA_SxCR_MINC
                | (2u << DMA_SxCR_PSIZE_Pos)
                | (2u << DMA_SxCR_MSIZE_Pos)
                | ((DMA_M2M_PRIORITY & 3u) << DMA_SxCR_PL_Pos)
                | DMA_SxCR_TCIE
                | DMA_SxCR_TEIE;
#if DMA_M2M_BURST_INCR4
    cr |= (1u << DMA_SxCR_PBURST_Pos) | (1u << DMA_SxCR_MBURST_Pos);
#endif
    DMA2_Stream0->CR = cr;

    /* -----------------------------------------------------------------------
     * 6) ZAGON: PB4 = 1, zapomnimo si CYCCNT, EN = 1
     *    Od tu naprej dela DMA sam; funkcija se takoj vrne.
     * --------------------------------------------------------------------- */
    dma_m2m_done  = 0u;
    dma_m2m_error = 0u;

    PB4_HIGH();
    t_start = DWT->CYCCNT;
    DMA2_Stream0->CR = cr | DMA_SxCR_EN;
}

/* ===========================================================================
 *  PREKINITVENA RUTINA DMA2 TOK 0
 *  Ime se ujema z vnosom v vektorski tabeli (startup_stm32h753xx.s), zato jo
 *  povezovalnik uporabi namesto privzete (__weak) Default_Handler.
 * =========================================================================== */
void DMA2_Stream0_IRQHandler(void)
{
    uint32_t t_end = DWT->CYCCNT;   /* čim prej, da je meritev čim točnejša */
    uint32_t isr   = DMA2->LISR;

    /* LISR preberemo ENKRAT in nato preverjamo kopijo: tako vidimo stanje
     * vseh zastavic v istem trenutku. */

    /* TCIF0: uspešen konec prenosa (NDTR = 0). */
    if (isr & DMA_S0_TCIF)
    {
        PB4_LOW();                          /* konec impulza na osciloskopu */
        DMA2->LIFCR = DMA_S0_TCIF;
        dma_m2m_cycles = t_end - t_start;   /* deluje tudi ob preteku CYCCNT */
        dma_m2m_done = 1u;
    }

    /* TEIF0: napaka na vodilu, tok je že ustavljen, prenos nedokončan.
     * Tudi tu postavimo dma_m2m_done, sicer bi glavna zanka čakala večno. */
    if (isr & DMA_S0_TEIF)
    {
        PB4_LOW();
        DMA2->LIFCR = DMA_S0_TEIF;
        dma_m2m_error = 1u;
        dma_m2m_done  = 1u;
    }

    /* Ostale zastavice (FEIF, DMEIF, HTIF) nimajo omogočene prekinitve,
     * a jih vseeno pobrišemo, da ne ostanejo postavljene. */
    DMA2->LIFCR = DMA_S0_FEIF | DMA_S0_DMEIF | DMA_S0_HTIF;

    __DSB();   /* brisanje zastavice mora priti do DMA pred izhodom iz IRQ */
}
