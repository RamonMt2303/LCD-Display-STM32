/**
  ******************************************************************************
  * @file    Templates/Src/main.c 
  * @author  MCD Application Team
  * @brief   STM32F4xx HAL API Template project 
  *
  * @note    modified by ARM
  *          The modifications allow to use this file as User Code Template
  *          within the Device Family Pack.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#ifdef _RTE_
#include "RTE_Components.h"             // Component selection
#endif
#ifdef RTE_CMSIS_RTOS2                  // when RTE component CMSIS RTOS2 is used
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
#endif

#ifdef RTE_CMSIS_RTOS2_RTX5
/**
  * Override default HAL_GetTick function
  */
uint32_t HAL_GetTick (void) {
  static uint32_t ticks = 0U;
         uint32_t i;

  if (osKernelGetState () == osKernelRunning) {
    return ((uint32_t)osKernelGetTickCount ());
  }

  /* If Kernel is not running wait approximately 1 ms then increment 
     and return auxiliary tick counter value */
  for (i = (SystemCoreClock >> 14U); i > 0U; i--) {
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
  }
  return ++ticks;
}

#endif

/** @addtogroup STM32F4xx_HAL_Examples
  * @{
  */

/** @addtogroup Templates
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
	
TIM_HandleTypeDef htim7;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim2;
uint16_t flag = 0;
uint16_t primera = 0;

static void init_GPIO(void){
	GPIO_InitTypeDef GPIO_InitStruct; //Definicion los GPIO
	
	__HAL_RCC_GPIOA_CLK_ENABLE();	 //Habilitar el reloj de los puertos GPIO A
	__HAL_RCC_GPIOB_CLK_ENABLE();	 //Habilitar el reloj de los puertos GPIO B
	__HAL_RCC_GPIOC_CLK_ENABLE();	 //Habilitar el reloj de los puertos GPIO C
	
	//Pin PC13 en modo interrupcion flanco de subida
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Pin = GPIO_PIN_13;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	
	//LEDS PB0, PB7, PB14
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_7 | GPIO_PIN_14;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

// Encender un led y apagarlo por x cantidad de segundos
void init_Timer7(){
	__HAL_RCC_TIM7_CLK_ENABLE(); //Habilitar reloj del timer 7
	HAL_NVIC_EnableIRQ(TIM7_IRQn);
	//Frecuencia de conteo es el reloj del timer/prescaler
	//Tiempo = periodo/frecuencia de conteo
	//Frecuencia de interrupcion = frecuencia de conteo/periodo
	htim7.Instance = TIM7; 
	htim7.Init.Prescaler = 8399; 
	//htim7.Init.Period = 9999; // Frecuencia de timers de APB1 84 MHz / prescaler 8400 = 10 kHz -> periodo = 10,000 (1 segundo) / 10 kHz = 1 segundo
	htim7.Init.Period = 14999; // 1.5 segundos
	HAL_TIM_Base_Init(&htim7);
	HAL_TIM_Base_Start_IT(&htim7);
}

void init_Timer5(){
	__HAL_RCC_TIM5_CLK_ENABLE(); //Habilitar reloj del timer 5
	HAL_NVIC_EnableIRQ(TIM5_IRQn);
	//Frecuencia de conteo es el reloj del timer/prescaler
	//Tiempo = periodo/frecuencia de conteo
	//Frecuencia de interrupcion = frecuencia de conteo/periodo
	htim5.Instance = TIM5; 
	htim5.Init.Prescaler = 8399; 
	htim5.Init.Period = 59999; // Frecuencia de timers de APB1 84 MHz / prescaler 8400 = 10 kHz -> periodo = 60,000 (6 segundos) / 10 kHz = 6 segundos
	HAL_TIM_Base_Init(&htim5);
	HAL_TIM_Base_Start_IT(&htim5);
}

// Encender un led y apagarlo por x cantidad de segundos
void init_Timer2(){
	__HAL_RCC_TIM2_CLK_ENABLE(); //Habilitar reloj del timer 7
	HAL_NVIC_EnableIRQ(TIM2_IRQn);
	//Frecuencia de conteo es el reloj del timer/prescaler
	//Tiempo = periodo/frecuencia de conteo
	//Frecuencia de interrupcion = frecuencia de conteo/periodo
	htim2.Instance = TIM2; 
	htim2.Init.Prescaler = 8399; 
	htim2.Init.Period = 59999; // Frecuencia de timers de APB1 84 MHz / prescaler 8400 = 10 kHz -> periodo = 60,000 (6 segundos) / 10 kHz = 6 segundos
	HAL_TIM_Base_Init(&htim2);
	HAL_TIM_Base_Start_IT(&htim2);
}

////ISR del Timer 7
void TIM7_IRQHandler(void){ 
	HAL_TIM_IRQHandler(&htim7);
}

////ISR del Timer 5
void TIM5_IRQHandler(void){ 
	HAL_TIM_IRQHandler(&htim5);
}

////ISR del Timer 2
void TIM2_IRQHandler(void){ 
	HAL_TIM_IRQHandler(&htim2);
}

////Callback del timer en modo Interrupcion
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim -> Instance == TIM7){
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
	}
	else if(htim -> Instance == TIM5){
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
		primera ++;
		if(primera == 1){init_Timer2();}
	}
	else if(htim -> Instance == TIM2){
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
	}
}

//Interrupciones Pines 10-15
void EXTI15_10_IRQHandler(void){
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}
	
//Callback de las interrupciones pines 10-15
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_PIN){
	flag ++;
	if(flag == 1){
		init_Timer7();
	}
	else if(flag == 2){
		init_Timer5();
	}
	else{
		HAL_NVIC_DisableIRQ(EXTI15_10_IRQn); //Deshabilitar la interrupcion del los puertos 10-15
	}
}	

int main(void)
{

  /* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, Flash preread and Buffer caches
       - Systick timer is configured by default as source of time base, but user 
             can eventually implement his proper time base source (a general purpose 
             timer for example or other time source), keeping in mind that Time base 
             duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
             handled in milliseconds basis.
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 168 MHz */
  SystemClock_Config();
  SystemCoreClockUpdate();

  init_GPIO();
	
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn); //Habilitar la interrupcion del los puertos 10-15
	
#ifdef RTE_CMSIS_RTOS2
  /* Initialize CMSIS-RTOS2 */
  osKernelInitialize ();

  /* Create thread functions that start executing, 
  Example: osThreadNew(app_main, NULL, NULL); */

  /* Start thread execution */
  osKernelStart();
#endif

  /* Infinite loop */
  while (1)
  {
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 25
  *            PLL_N                          = 336
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	// HSE/M * N/2 = SYSCLK deseado
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported */
  if (HAL_GetREVID() == 0x1001)
  {
    /* Enable the Flash prefetch */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}

#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
