#include "exti_pc13.h"

void pc13_exti_init(void)
{
    /* 1) Vklopi uro za SYSCFG (bit 1). Brez ure vpis v SYSCFG nima učinka. */
    //*RCC_APB4ENR |= (1 << 1);
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    /* 2) Povežemo linijo EXTI13 s portom C.
     *
     *    Vsaka linija EXTIn (n = 0..15) lahko posluša pin n na ENEM SAMEM
     *    portu (PAn, PBn, PCn, ...). Kateri port, izberemo v SYSCFG_EXTICR1..4:
     *
     *      EXTICR1 -> EXTI0..3,  EXTICR2 -> EXTI4..7,
     *      EXTICR3 -> EXTI8..11, EXTICR4 -> EXTI12..15
     *
     *    Vsaka linija ima v registru 4 bite:
     *
     *      bit:  15..12   11..8    7..4     3..0
     *           [EXTI15] [EXTI14] [EXTI13] [EXTI12]
     *
     *    EXTI13 je torej na bitih [7:4].
     *    Koda porta: 0 = PA, 1 = PB, 2 = PC, 3 = PD, ...
     *    Za PC13 v bite [7:4] vpišemo 2 (0b0010).
     *
     */
    *SYSCFG_EXTICR4 = (*SYSCFG_EXTICR4 & ~(0xF << 4)) | (0x2 << 4);

    /* 3) Proženje na naraščajočo fronto (tipka na DK je aktivno visoka) */
    *EXTI_RTSR1 |=  (1 << 13);     /* rising:  vklopi  */
    *EXTI_FTSR1 &= ~(1 << 13);     /* falling: izklopi */

    /* 4) Odmaskiraj linijo 13 -> EXTI sme poslati zahtevo v NVIC */
    *EXTI_C1IMR1 |= (1 << 13);

    /* 5) NVIC: EXTI10..15 si delijo prekinitev št. 40.
     *
     *    Prioriteta: vsak register IPR ima 4 prekinitve po 8 bitov.
     *    40 / 4 = 10 -> IPR10, 40 % 4 = 0 -> biti [7:0].
     *    STM32H7 uporablja le zgornje 4 bite -> prioriteta gre na bite [7:4].
     */
    *NVIC_IPR10 = (*NVIC_IPR10 & ~(0xFF << 0)) | (5 << 4);   /* prioriteta 5 */

    /*    Omogočanje: vsak register ISER pokrije 32 prekinitev.
     *    40 / 32 = 1 -> ISER1, 40 % 32 = 8 -> bit 8.
     */
    *NVIC_ISER1 = (1 << 8);
}