#ifndef EXTI_PC13_H
#define EXTI_PC13_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

/* RCC */
#define RCC_APB4ENR     ((volatile uint32_t *) 0x580244F4)

/* SYSCFG */
#define SYSCFG_EXTICR4  ((volatile uint32_t *) 0x58000414)

/* EXTI */
#define EXTI_RTSR1      ((volatile uint32_t *) 0x58000000)
#define EXTI_FTSR1      ((volatile uint32_t *) 0x58000004)
#define EXTI_C1IMR1     ((volatile uint32_t *) 0x58000080)
#define EXTI_C1PR1      ((volatile uint32_t *) 0x58000088)

/* NVIC */
#define NVIC_ISER1      ((volatile uint32_t *) 0xE000E104)
#define NVIC_IPR10      ((volatile uint32_t *) 0xE000E428)

void pc13_exti_init(void);


#endif /* EXTI_PC13_H */