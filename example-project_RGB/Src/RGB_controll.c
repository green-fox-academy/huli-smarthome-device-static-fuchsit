/*
 * RGB_controll.c
 *
 *  Created on: Jan 18, 2018
 *      Author: Ádám
 */
#include "RGB_controll.h"

void colors_init(int r, int g, int b) {
	WriteRed = r;
	WriteGreen = g;
	WriteBlue = b;
}

void LED_Init(void) {

	__HAL_RCC_GPIOB_CLK_ENABLE();    // we need to enable the GPIO* port's clock first
	__HAL_RCC_GPIOA_CLK_ENABLE();    // we need to enable the GPIO* port's clock first

	GPIO_InitTypeDef LEDRED;
	LEDRED.Pin 			= GPIO_PIN_15;            // this is about PIN 1
	LEDRED.Mode 		= GPIO_MODE_AF_PP; // Configure as output with push-up-down enabled
	LEDRED.Pull 		= GPIO_NOPULL;      // the push-up-down should work as pulldown
	LEDRED.Speed 		= GPIO_SPEED_HIGH;     // we need a high-speed output
	LEDRED.Alternate 	= GPIO_AF1_TIM2;   //Alterante function to set PWM timer

	HAL_GPIO_Init(GPIOA, &LEDRED);   // initialize the pin on GPIO* port with HAL

	GPIO_InitTypeDef LEDGREEN;
	LEDGREEN.Pin 		= GPIO_PIN_1;            // this is about PIN 1
	LEDGREEN.Mode 		= GPIO_MODE_AF_PP; // Configure as output with push-up-down enabled
	LEDGREEN.Pull 		= GPIO_NOPULL;      // the push-up-down should work as pulldown
	LEDGREEN.Speed 		= GPIO_SPEED_HIGH;     // we need a high-speed output
	LEDGREEN.Alternate 	= GPIO_AF2_TIM3;   //Alterante function to set PWM timer

	HAL_GPIO_Init(GPIOB, &LEDGREEN);   // initialize the pin on GPIO* port with HAL

	GPIO_InitTypeDef LEDBLUE;
	LEDBLUE.Pin 		= GPIO_PIN_4;            // this is about PIN 1
	LEDBLUE.Mode 		= GPIO_MODE_AF_PP; // Configure as output with push-up-down enabled
	LEDBLUE.Pull 		= GPIO_NOPULL;      // the push-up-down should work as pulldown
	LEDBLUE.Speed 		= GPIO_SPEED_HIGH;     // we need a high-speed output
	LEDBLUE.Alternate 	= GPIO_AF2_TIM3;   //Alterante function to set PWM timer

	HAL_GPIO_Init(GPIOB, &LEDBLUE);   // initialize the pin on GPIO* port with HAL

}

void TIMER_Init(void) {

	__HAL_RCC_TIM2_CLK_ENABLE();
	__HAL_RCC_TIM3_CLK_ENABLE();

	Red.Instance               = TIM2;
	Red.Init.Period            = 255; //max value 0xFF
	Red.Init.Prescaler         = 0;
	Red.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	Red.Init.CounterMode       = TIM_COUNTERMODE_UP;

	RedPwmHandle.OCMode 	= TIM_OCMODE_PWM1;
	RedPwmHandle.Pulse 		= 0;

	HAL_TIM_PWM_ConfigChannel(&Red, &RedPwmHandle, TIM_CHANNEL_1);
	HAL_TIM_PWM_Init(&Red);
	HAL_TIM_PWM_Start(&Red, TIM_CHANNEL_1);


	Green.Instance               = TIM3;
	Green.Init.Period            = 255; //16bit number max value 0xFF
	Green.Init.Prescaler         = 0;
	Green.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	Green.Init.CounterMode       = TIM_COUNTERMODE_UP;

	GreenPwmHandle.OCMode 	= TIM_OCMODE_PWM1;
	GreenPwmHandle.Pulse 	= 0;

	HAL_TIM_PWM_ConfigChannel(&Green, &GreenPwmHandle, TIM_CHANNEL_4);
	HAL_TIM_PWM_Init(&Green); //Configure the timer
	HAL_TIM_PWM_Start(&Green, TIM_CHANNEL_4);

	Blue.Instance               = TIM3;
	Blue.Init.Period            = 255; //16bit number max value 0xFF
	Blue.Init.Prescaler         = 0;
	Blue.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	Blue.Init.CounterMode       = TIM_COUNTERMODE_UP;

	BluePwmHandle.OCMode = TIM_OCMODE_PWM1;
	BluePwmHandle.Pulse = 0;

	HAL_TIM_PWM_ConfigChannel(&Blue, &BluePwmHandle, TIM_CHANNEL_1);
	HAL_TIM_PWM_Init(&Blue);
	HAL_TIM_PWM_Start(&Blue, TIM_CHANNEL_1);
}


