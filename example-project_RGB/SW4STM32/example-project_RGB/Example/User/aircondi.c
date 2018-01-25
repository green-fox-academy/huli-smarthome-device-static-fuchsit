/*
 * aircondi.c

 *
 *  Created on: Jan 25, 2018
 *      Author: Ádám
 */
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include "aircondi.h"

void temp_set(int user_min, int user_max) {

	get_temperatura();
	printf("----------\n");
	HAL_Delay(1000);
	if (temp >= user_max){
		TIM2 -> CCR3 = 100;
	} else if (temp <= user_min) {
		TIM2 -> CCR3 = 0;
	}
}

void Fan_Init(void) {

	__HAL_RCC_GPIOA_CLK_ENABLE();

	PWM_FAN.Alternate 	= GPIO_AF1_TIM2;
	PWM_FAN.Mode 		= GPIO_MODE_IT_RISING;
	PWM_FAN.Pin 		= GPIO_PIN_2;
	PWM_FAN.Pull 		= GPIO_NOPULL;
	PWM_FAN.Speed 		= GPIO_SPEED_HIGH;

	HAL_GPIO_Init(GPIOA, &PWM_FAN);

}

void temp_sensor_init(void) {

	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_I2C1_CLK_ENABLE();

	temp_sensor_config.Pin 				= GPIO_PIN_9 | GPIO_PIN_8;
	temp_sensor_config.Mode           	= GPIO_MODE_AF_OD;
	temp_sensor_config.Speed 			= GPIO_SPEED_HIGH;
	temp_sensor_config.Pull 			= GPIO_PULLUP;
	temp_sensor_config.Alternate      	= GPIO_AF4_I2C1;

	HAL_GPIO_Init(GPIOB, &temp_sensor_config);

	// I2C init
	I2cHandle.Instance             = I2C1;
	I2cHandle.Init.Timing          = 0x40912732;
	I2cHandle.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;

	HAL_I2C_Init(&I2cHandle);
}

void get_temperatura() {

	//trigger temperature measurements
	HAL_I2C_Master_Transmit(&I2cHandle , 0b1001000<<1, &reg , 1 , 100);
	HAL_I2C_Master_Receive(&I2cHandle , 0b1001000<<1, &temp , 1 , 100);

	printf("%i Celsius\n", temp);
}

void timer_pwm_config() {

	__HAL_RCC_TIM2_CLK_ENABLE();

	TimHandle.Instance               = TIM2;
	TimHandle.Init.Prescaler         = 1000;
	TimHandle.Init.Period            = 50;
	TimHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;

	TimerOCConfig.OCMode = TIM_OCMODE_PWM1;
	TimerOCConfig.Pulse = 0;								//MIN 55 to start

	HAL_TIM_PWM_Init(&TimHandle);
	HAL_TIM_PWM_ConfigChannel(&TimHandle, &TimerOCConfig, TIM_CHANNEL_3);

	HAL_TIM_PWM_Start(&TimHandle , TIM_CHANNEL_3);
}

void Peripherals_Init(void) {

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
	timer_pwm_config();
	Fan_Init();
	temp_sensor_init();
	UART_Config();

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
void SystemClock_Config(void) {

	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/* MSI is enabled after System reset, activate PLL with MSI as source */
	RCC_OscInitStruct.OscillatorType 		= RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.MSIState 				= RCC_MSI_ON;
	RCC_OscInitStruct.MSIClockRange 		= RCC_MSIRANGE_6;
	RCC_OscInitStruct.MSICalibrationValue 	= RCC_MSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState 			= RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource 		= RCC_PLLSOURCE_MSI;
	RCC_OscInitStruct.PLL.PLLM 				= 1;
	RCC_OscInitStruct.PLL.PLLN 				= 40;
	RCC_OscInitStruct.PLL.PLLR 				= 2;
	RCC_OscInitStruct.PLL.PLLP 				= 7;
	RCC_OscInitStruct.PLL.PLLQ 				= 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		/* Initialization Error */
		while (1)
			;
	}

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	 clocks dividers */
	RCC_ClkInitStruct.ClockType 		= (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource 		= RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider 	= RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider 	= RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider 	= RCC_HCLK_DIV1;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
		/* Initialization Error */
		while (1)
			;
	}
}

