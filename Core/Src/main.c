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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  /* USER CODE BEGIN 2 */

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




  /*
   * DEMONSTRACIJA VSTOPA V PREKINITEV
   * ---------------------------------
   * Registre napolnimo z znanimi vrednostmi (markerji) in nato v neskončni
   * zanki čakamo na pritisk gumba (EXTI13 na PC13). Ob vstopu v
   * EXTI15_10_IRQHandler na skladu prepoznamo, katere registre je
   * shranila strojna oprema (R0-R3, R12, LR, PC, xPSR), r4-r11 pa ostanejo
   * nespremenjeni v registrih. Shranjeni PC kaže na demo_loop.
   *
   * Čakalna zanka je namenoma v asemblerju: v C bi prevajalnik lahko
   * uporabil r0-r3 in markerje prepisal.
   *
   * r7 izpustimo: v Debug buildu (-O0) je kazalec okvirja (frame pointer)
   * funkcije main in ga GCC v inline asm ne dovoli spreminjati.
   *
   * SINTAKSA RAZŠIRJENEGA INLINE ASM (GCC):
   *
   *   __asm volatile ( "koda"
   *       : izhodni operandi     npr. [res] "=r" (x)
   *       : vhodni operandi      npr. [pin] "r" (GPIO_PIN_13)
   *       : clobbers             registri/pomnilnik, ki jih koda pokvari
   *   );
   *
   * Dvopičja so ločila in štejejo po vrsti. Tukaj sta izhodni in vhodni
   * del prazna (vse vrednosti so takojšnje konstante v ukazih), zato
   * stojita prvi dve dvopičji brez vsebine.
   *
   * CLOBBERS:
   * Prevajalnik asm kode ne razčlenjuje, zanj je to črna škatla. S seznamom
   * clobbers mu povemo, katere registre koda spremeni. Prevajalnik zato:
   *   - v teh registrih čez asm blok ne hrani svojih vrednosti,
   *   - callee-saved registre (r4-r11, LR) shrani v prologu funkcije
   *     (push) in jih obnovi v epilogu,
   *   - vhodnih operandov ne postavi v te registre.
   * "memory": koda lahko bere/piše pomnilnik mimo vednosti prevajalnika.
   * Prevajalnik pred blokom zapiše spremenljivke iz registrov v pomnilnik,
   * po bloku jih prebere znova in pomnilniških dostopov ne premika čez
   * blok (compiler barrier).
   *
   * Ker se ta blok nikoli ne vrne, clobbers tukaj praktično niso nujni.
   * Pustimo jih, ker pravilnost ne sme biti odvisna od tega, da se blok
   * slučajno ne vrne (npr. če zanko kasneje zamenjamo s čakanjem na
   * zastavico in nadaljujemo izvajanje).
   *
   * __builtin_unreachable() za blokom: prevajalniku povemo, da izvajanje
   * do te točke nikoli ne pride (ne ve, da je demo_loop neskončna zanka).
   * Ne generira nobenega ukaza. Če bi izvajanje tja vseeno prišlo, je
   * obnašanje nedefinirano.
   */
  __asm volatile (
      "mov   r0,  #0xA0A0A0A0  \n\t"
      "mov   r1,  #0xA1A1A1A1  \n\t"
      "mov   r2,  #0xA2A2A2A2  \n\t"
      "mov   r3,  #0xA3A3A3A3  \n\t"
      "mov   r4,  #0xA4A4A4A4  \n\t"
      "mov   r5,  #0xA5A5A5A5  \n\t"
      "mov   r6,  #0xA6A6A6A6  \n\t"
      "mov   r8,  #0xA8A8A8A8  \n\t"
      "mov   r9,  #0xA9A9A9A9  \n\t"
      "mov   r10, #0xAAAAAAAA  \n\t"
      "mov   r11, #0xABABABAB  \n\t"
      "mov   r12, #0xACACACAC  \n\t"
      "mov   lr,  #0xAEAEAEAE  \n\t"
      ".global demo_loop       \n\t"
      "demo_loop: b demo_loop  \n\t"   // čakamo na pritisk gumba
      :         // izhodi: ni jih 
      :         // vhodi: ni jih
      : "r0","r1","r2","r3","r4","r5","r6","r8","r9",
        "r10","r11","r12","lr","memory"   // clobbers - kaj koda spremeni
  );
  __builtin_unreachable();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    //*(uint32_t *)0x58022414 |= 0x00000004; // Ugasni LED
    //*(uint32_t *)0x58022414 &= ~0x00000004; // Prižgi LED
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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOI, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
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
