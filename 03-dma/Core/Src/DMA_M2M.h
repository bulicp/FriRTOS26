/**
 * @file    DMA_M2M.h
 * @brief   Prenos pomnilnik -> pomnilnik (memory-to-memory) z DMA2, tok 0.
 *
 *          Nastavitev registrov DMA2 je napisana "bare-metal".
 *
 *          Ob ZAČETKU prenosa se postavi pin PB4 (logična 1), ob KONCU
 *          prenosa ga prekinitvena rutina pobriše. Širina impulza na PB4
 *          (osciloskop) je torej čas prenosa (+ zakasnitev prekinitve).
 *
 *          Hkrati čas izmeri tudi števec urinih ciklov DWT->CYCCNT:
 *          rezultat je v spremenljivki dma_m2m_cycles.
 *
 *          VRSTNI RED KLICEV:
 *              dma_m2m_init();                   // enkrat: ure, PB4, DWT, NVIC
 *              dma_m2m_start(src, dst, n);       // zažene prenos in se TAKOJ vrne
 *              while (!dma_m2m_done) { ... }     // CPE je med prenosom prosta
 *
 *          POZOR: src in dst morata biti v pomnilniku, do katerega ima DMA2
 *          dostop (AXI SRAM 0x24000000 ali SRAM1..3 0x30000000). DTCM
 *          (0x20000000), kjer so privzeto .data, .bss in sklad, NI dosegljiv!
 *          Zato medpomnilnike postavimo v sekcijo DMA_BUFFER (glej spodaj).
 */
#ifndef DMA_M2M_H
#define DMA_M2M_H

#include "main.h"
#include <stdint.h>

/* ------------------------------------------------------------------------- */
/*  Parametri                                                                 */
/* ------------------------------------------------------------------------- */

/** Dolžina eksplozijskega prenosa (burst):
 *    0 = single (vsak prenos posebej, en podatek na arbitražo vodila)
 *    1 = INCR4  (4 zaporedni podatki na eno arbitražo vodila)
 *  Preizkusi oboje in primerjaj čas na osciloskopu / v dma_m2m_cycles. */
#define DMA_M2M_BURST_INCR4   0

/** Prioriteta toka (PL): 0 = nizka, 1 = srednja, 2 = visoka, 3 = zelo visoka. */
#define DMA_M2M_PRIORITY      2u

/** Atribut za medpomnilnike, ki jih uporablja DMA: AXI SRAM, poravnano na 32 B. */
#define DMA_BUFFER  __attribute__((section(".axi_sram"), aligned(32)))

/** Največje število podatkov v enem prenosu (NDTR je 16-biten). */
#define DMA_M2M_MAX_ITEMS     65535u

/* ------------------------------------------------------------------------- */
/*  Stanje (nastavlja prekinitvena rutina)                                    */
/* ------------------------------------------------------------------------- */

extern volatile uint32_t dma_m2m_done;    /**< 1, ko je prenos končan (ali napaka) */
extern volatile uint32_t dma_m2m_error;   /**< 1, če je prišlo do napake prenosa (TEIF) */
extern volatile uint32_t dma_m2m_cycles;  /**< trajanje zadnjega prenosa v urinih ciklih CPE */

/* ------------------------------------------------------------------------- */
/*  Vmesnik                                                                   */
/* ------------------------------------------------------------------------- */

/**
 * @brief  Vklopi uro DMA2, nastavi PB4 kot izhod, vklopi števec DWT->CYCCNT
 *         in omogoči prekinitev DMA2_Stream0_IRQn.
 */
void dma_m2m_init(void);

/**
 * @brief  Zažene prenos n_words 32-bitnih besed iz src v dst.
 *         Funkcija se vrne takoj; konec prenosa sporoči dma_m2m_done = 1.
 * @param  src      izvorni naslov (poravnan na 4 B)
 * @param  dst      ciljni naslov  (poravnan na 4 B)
 * @param  n_words  število besed, 1 ... DMA_M2M_MAX_ITEMS
 */
void dma_m2m_start(const uint32_t *src, uint32_t *dst, uint32_t n_words);

/* ------------------------------------------------------------------------- */
/*  DWT: števec urinih ciklov                                                 */
/* ------------------------------------------------------------------------- */

/** Trenutna vrednost števca urinih ciklov (32 bitov, pri 64 MHz preteče v ~67 s). */
static inline uint32_t dwt_cycles(void)
{
    return DWT->CYCCNT;
}

#endif /* DMA_M2M_H */
