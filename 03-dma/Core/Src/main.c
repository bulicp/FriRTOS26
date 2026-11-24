/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "DMA_M2M.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */

/* Število 32-bitnih besed v enem prenosu (4096 besed = 16 KB). */
//#define N_WORDS   4096u
#define N_WORDS   2048u

/* Medpomnilnika MORATA biti v AXI SRAM (DMA_BUFFER), ker DMA2 ne doseže
 * DTCM, kamor povezovalnik privzeto postavi globalne spremenljivke. */
static uint32_t src[N_WORDS] DMA_BUFFER;
static uint32_t dst[N_WORDS] DMA_BUFFER;

/* Rezultati: opazuj v Live Watch / Watch oknu razhroščevalnika. */
volatile uint32_t cycles_dma;       /* DMA prenos (izmeri ISR)            */
volatile uint32_t cycles_cpu;       /* isti prenos z zanko na CPE          */
volatile uint32_t cpu_free_loops;   /* koliko iteracij je CPE naredil MED DMA */
volatile uint32_t napake_dma;       /* število napačnih besed po DMA       */
volatile uint32_t napake_cpu;       /* število napačnih besed po CPE       */
volatile uint32_t stevec_prenosov;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* PB15: impulz za CPE prenos (PB4 je impulz za DMA prenos).
 * Na osciloskopu primerjaj širini obeh impulzov. Pin postavljamo prek BSRR:
 * zgornjih 16 bitov pin pobriše, spodnjih 16 ga postavi (atomarno). */
#define PB15_HIGH()   (GPIOB->BSRR = (1u << 15))
#define PB15_LOW()    (GPIOB->BSRR = (1u << (15 + 16)))

static void pb15_init(void)
{
    /*   MODER   bita 31:30 = 01  splošni izhod
     *   OTYPER  bit 15     = 0   push-pull
     *   OSPEEDR bita 31:30 = 11  zelo hitri fronti
     *   PUPDR   bita 31:30 = 00  brez uporov                                 */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
    (void)RCC->AHB4ENR;

    PB15_LOW();
    GPIOB->MODER   = (GPIOB->MODER & ~(3u << 30)) | (1u << 30);
    GPIOB->OTYPER &= ~(1u << 15);
    GPIOB->OSPEEDR |=  (3u << 30);
    GPIOB->PUPDR  &= ~(3u << 30);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  //MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  dma_m2m_init();   /* DMA2 ura, PB4 izhod, DWT->CYCCNT, NVIC DMA2_Stream0 */
  pb15_init();      /* PB15 izhod: impulz med CPE prenosom */





/*
 * ============================================================================
 *  DEBUG CONSOLE: kratka navodila (STM32Cube for VS Code, GDB)
 * ============================================================================
 *  Ukazi delujejo samo, ko je jedro ustavljeno (breakpoint ali korak).
 *  GDB ukaz vpišeš v Debug Console s predpono '>'.
 *  Pri nekaterih verzijah adapterja je predpona '-exec '.
 *
 *  REGISTRI
 *    >info registers        vsi jedrni registri (r0-r12, sp, lr, pc, xpsr)
 *    >info all-registers    vsi registri, tudi posebni in FPU (s0-s31, fpscr)
 *    >p/x $r0               en register v hex (tudi $sp, $lr, $pc, $xpsr)
 *    >p/t $r0               en register v binarnem zapisu

 *
 *  POMNILNIK:  >x/<N><format><velikost> <naslov>
 *    format:    x = hex, d = desetiško, u = nepredznačeno, t = binarno,
 *               c = znak, s = niz, i = strojni ukaz (disassembly)
 *    velikost:  b = 8 bit, h = 16 bit, w = 32 bit, g = 64 bit
 *
 *    >x/16xb $sp            16 bajtov od sp naprej
 *    >x/32xb $sp            32 bajtov od sp naprej
 *    >x/8xw  $sp            8 besed od sp (osnovni okvir sklada ob izjemi:
 *                           r0, r1, r2, r3, r12, lr, pc, xpsr)
 *    >x/8xw  0x20000000     8 besed s poljubnega naslova (npr. začetek RAM)
 *    >x/xw   0x080000E0     ena beseda, npr. vnos v vektorski tabeli za IRQ 40
 *                           (naslov = 0x08000000 + (16 + 40) * 4)
 *
 *  DISASSEMBLY
 *    >x/4i  $pc             4 ukazi od pc naprej
 *    >x/10i $pc             10 ukazov od pc naprej
 *    >disassemble           celotna trenutna funkcija
 *    >disassemble /s        celotna funkcija, prepletena z izvorno kodo
 * ============================================================================
 */


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t i;

    /* 1) Pripravimo podatke: izvor z vzorcem, cilj pobrišemo.
     *
     *    Vzorec je psevdonaključen, da preverjanje ujame čim več napak:
     *    - 0x9E3779B9 = 2^32 / φ (φ = 1,618... zlati rez). To je konstanta
     *      Knuthovega multiplikativnega ("Fibonaccijevega") zgoščevanja
     *      (uporablja jo tudi šifrirni algoritem TEA). Množenje z njo
     *      razprši zaporedne indekse i po vseh 32 bitih, zato so sosednje
     *      besede povsem različne. Pri preprostem vzorcu (src[i] = i) bi
     *      zamik za eno besedo, podvojena beseda ali napačen naslov lahko
     *      ostali neopaženi, tu pa jih napake_dma takoj ujame.
     *    - Pripona u: nepredznačeno množenje, preliv se definirano ovije
     *      po modulu 2^32 (pri predznačenih tipih bi bil nedefiniran).
     *    - XOR s stevec_prenosov: v vsaki ponovitvi zanke so podatki
     *      drugačni, zato v dst ne morejo ostati "pravilni" podatki iz
     *      prejšnjega kroga, če DMA česa ne bi prepisal. */
    for (i = 0; i < N_WORDS; i++)
    {
      src[i] = (i * 0x9E3779B9u) ^ stevec_prenosov;
      dst[i] = 0u;
    }

    /* 2) DMA prenos: PB4 gre na 1, ISR ga ob koncu postavi na 0.
     *    Medtem CPE NI blokiran: šteje iteracije čakalne zanke. To je
     *    čas, ki bi ga lahko porabil za drugo delo. */
    cpu_free_loops = 0u;
    dma_m2m_start(src, dst, N_WORDS);
    while (!dma_m2m_done)
    {
      cpu_free_loops++;
    }
    cycles_dma = dma_m2m_cycles;

    /* 3) Preverimo rezultat DMA. */
    napake_dma = 0u;
    for (i = 0; i < N_WORDS; i++)
    {
      if (dst[i] != src[i]) napake_dma++;
    }

    /* 4) Za primerjavo: isti prenos s CPE, izmerjen z DWT->CYCCNT.
     *    (V Debug prevodu brez optimizacije je zanka precej počasnejša;
     *    preizkusi tudi Release ali memcpy().) */
    /*    PB15 = 1 med CPE prenosom: širino impulza na PB15 primerjaj
     *    z impulzom na PB4 (DMA). */
    for (i = 0; i < N_WORDS; i++) dst[i] = 0u;
    PB15_HIGH();
    uint32_t t0 = dwt_cycles();
    for (i = 0; i < N_WORDS; i++)
    {
      dst[i] = src[i];
    }
    cycles_cpu = dwt_cycles() - t0;
    PB15_LOW();

    napake_cpu = 0u;
    for (i = 0; i < N_WORDS; i++)
    {
      if (dst[i] != src[i]) napake_cpu++;
    }

    stevec_prenosov++;

    /* 5) Premor, da je impulz na PB4 na osciloskopu lepo periodičen. */
    HAL_Delay(10);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOI, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  //GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PI13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pin : PJ2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
