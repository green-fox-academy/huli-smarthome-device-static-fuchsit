/**
 ******************************************************************************
 * @file    Templates/Src/main.c
 * @author  MCD Application Team
 * @brief   Main program body
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

#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
 set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/** @addtogroup STM32L4xx_HAL_Examples
 * @{
 */

/** @addtogroup Templates
 * @{
 */

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef uartHandle;
TIM_HandleTypeDef TimHandle;
TIM_OC_InitTypeDef TimPwmHandle;
TIM_HandleTypeDef Red;
TIM_OC_InitTypeDef RedPwmHandle;
TIM_HandleTypeDef Green;
TIM_OC_InitTypeDef GreenPwmHandle;
TIM_HandleTypeDef Blue;
TIM_OC_InitTypeDef BluePwmHandle;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);

/* Private functions ---------------------------------------------------------*/
static void Peripherals_Init(void);
static void UART_Init(void);
static void timer_pwm_init();

#define RED GPIOA, GPIO_PIN_2
#define GREEN GPIOB, GPIO_PIN_0
#define BLUE GPIOB, GPIO_PIN_4
#define PURPLE GPIOB, GPIO_PIN_1 + GPIO_PIN_4
#define CYAN GPIOB, GPIO_PIN_4 + GPIO_PIN_0

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void) {

Peripherals_Init();

HAL_GPIO_WritePin(RED, SET);
HAL_GPIO_WritePin(GREEN, SET);
HAL_GPIO_WritePin(BLUE, SET);

HAL_GPIO_WritePin(PURPLE, RESET);


	while (1) {
			  }

	TIM2->CCR1 = 100;
	TIM8->CCR1 = 100;
	TIM3->CCR1 = 100;
}

static void Peripherals_Init(void) {
	/* STM32L4xx HAL library initialization:
	 - Configure the Flash prefetch, Flash preread and Buffer caches
	 - Systick timer is configured by default as source of time base, but user
	 can eventually implement his proper time base source (a general purpose
	 timer for example or other time source), keeping in mind that Time base
	 duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
	 handled in milliseconds basis.
	 - Low Level Initialization
	 */
	HAL_Init();

	/* Configure the System clock to have a frequency of 80 MHz */
	SystemClock_Config();
	UART_Init();

	timer_pwm_init();

	__HAL_RCC_GPIOA_CLK_ENABLE();    // we need to enable the GPIO* port's clock first

	GPIO_InitTypeDef LEDRED;            // create a config structure
	LEDRED.Pin = GPIO_PIN_2;            // this is about PIN 1
	LEDRED.Mode = GPIO_MODE_OUTPUT_OD; // Configure as output with push-up-down enabled
	LEDRED.Pull = GPIO_NOPULL;      // the push-up-down should work as pulldown
	LEDRED.Speed = GPIO_SPEED_HIGH;     // we need a high-speed output
	LEDRED.Alternate = GPIO_AF1_TIM2;   //Alterante function to set PWM timer

	HAL_GPIO_Init(GPIOA, &LEDRED);   // initialize the pin on GPIO* port with HAL

	__HAL_RCC_GPIOB_CLK_ENABLE();    // we need to enable the GPIO* port's clock first

	GPIO_InitTypeDef LEDGREEN;            // create a config structure
	LEDGREEN.Pin = GPIO_PIN_4;            // this is about PIN 1
	LEDGREEN.Mode = GPIO_MODE_OUTPUT_OD; // Configure as output with push-up-down enabled
	LEDGREEN.Pull = GPIO_NOPULL;      // the push-up-down should work as pulldown
	LEDGREEN.Speed = GPIO_SPEED_HIGH;     // we need a high-speed output
	LEDGREEN.Alternate = GPIO_AF2_TIM3;   //Alterante function to set PWM timer

	HAL_GPIO_Init(GPIOB, &LEDGREEN);   // initialize the pin on GPIO* port with HAL

	GPIO_InitTypeDef LEDBLUE;            // create a config structure
	LEDBLUE.Pin = GPIO_PIN_0;            // this is about PIN 1
	LEDBLUE.Mode = GPIO_MODE_OUTPUT_OD; // Configure as output with push-up-down enabled
	LEDBLUE.Pull = GPIO_NOPULL	;      // the push-up-down should work as pulldown
	LEDBLUE.Speed = GPIO_SPEED_HIGH;     // we need a high-speed output
	LEDBLUE.Alternate = GPIO_AF2_TIM3;   //Alterante function to set PWM timer

	HAL_GPIO_Init(GPIOB, &LEDBLUE);   // initialize the pin on GPIO* port with HAL

	BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
	BSP_LED_Init(LED_GREEN);

}

static void timer_pwm_init() {
	__HAL_RCC_TIM2_CLK_ENABLE();

	/*TimHandle.Instance               = TIM2;
	TimHandle.Init.Period            = 2000;
	TimHandle.Init.Prescaler         = 40000;
	TimHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;

	TimPwmHandle.OCMode = TIM_OCMODE_PWM1;
	TimPwmHandle.Pulse = 0;

	HAL_TIM_PWM_Init(&TimHandle);
	HAL_TIM_PWM_ConfigChannel(&TimHandle, &TimPwmHandle, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_1);*/
	Red.Instance               = TIM2;
	Red.Init.Period            = 2000;
	Red.Init.Prescaler         = 40000;
	Red.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	Red.Init.CounterMode       = TIM_COUNTERMODE_UP;

	RedPwmHandle.OCMode = TIM_OCMODE_PWM1;
	RedPwmHandle.Pulse = 0;

	HAL_TIM_PWM_Init(&Red);
	HAL_TIM_PWM_ConfigChannel(&Red, &RedPwmHandle, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&Red, TIM_CHANNEL_1);

	__HAL_RCC_TIM3_CLK_ENABLE();

	Green.Instance               = TIM3;
	Green.Init.Period            = 2000;
	Green.Init.Prescaler         = 40000;
	Green.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	Green.Init.CounterMode       = TIM_COUNTERMODE_UP;

	GreenPwmHandle.OCMode = TIM_OCMODE_PWM1;
	GreenPwmHandle.Pulse = 0;

	HAL_TIM_PWM_Init(&Green);
	HAL_TIM_PWM_ConfigChannel(&Green, &GreenPwmHandle, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&Green, TIM_CHANNEL_1);

	Blue.Instance               = TIM3;
	Blue.Init.Period            = 2000;
	Blue.Init.Prescaler         = 40000;
	Blue.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	Blue.Init.CounterMode       = TIM_COUNTERMODE_UP;

	BluePwmHandle.OCMode = TIM_OCMODE_PWM1;
	BluePwmHandle.Pulse = 0;

	HAL_TIM_PWM_Init(&Blue);
	HAL_TIM_PWM_ConfigChannel(&Blue, &BluePwmHandle, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start_IT(&Blue, TIM_CHANNEL_3);

}

static void UART_Init(void) {
	uartHandle.Init.BaudRate = 115200;
	uartHandle.Init.WordLength = UART_WORDLENGTH_8B;
	uartHandle.Init.StopBits = UART_STOPBITS_1;
	uartHandle.Init.Parity = UART_PARITY_NONE;
	uartHandle.Init.Mode = UART_MODE_TX_RX;

	BSP_COM_Init(COM1, &uartHandle);
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follow :
 *            System Clock source            = PLL (MSI)
 *            SYSCLK(Hz)                     = 80000000
 *            HCLK(Hz)                       = 80000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 1
 *            APB2 Prescaler                 = 1
 *            MSI Frequency(Hz)              = 4000000
 *            PLL_M                          = 1
 *            PLL_N                          = 40
 *            PLL_R                          = 2
 *            PLL_P                          = 7
 *            PLL_Q                          = 4
 *            Flash Latency(WS)              = 4
 * @param  None
 * @retval None
 */
static void SystemClock_Config(void) {
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/* MSI is enabled after System reset, activate PLL with MSI as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
	RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 40;
	RCC_OscInitStruct.PLL.PLLR = 2;
	RCC_OscInitStruct.PLL.PLLP = 7;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		/* Initialization Error */
		while (1)
			;
	}

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	 clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
		/* Initialization Error */
		while (1)
			;
	}
}

PUTCHAR_PROTOTYPE {
	/* Place your implementation of fputc here */
	/* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
	HAL_UART_Transmit(&uartHandle, (uint8_t *) &ch, 1, 0xFFFF);

	return ch;
}

#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(char* file, uint32_t line)
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
