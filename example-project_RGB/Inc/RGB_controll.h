/*
 * RGB_controll.h
 *
 *  Created on: Jan 18, 2018
 *      Author: Ádám
 */

#ifndef RGB_CONTROLL_H_
#define RGB_CONTROLL_H_
#include <stdio.h>
#include <stdlib.h>
#include "main.h"

#define RED 	GPIOA, GPIO_PIN_15
#define GREEN 	GPIOB, GPIO_PIN_1
#define BLUE 	GPIOB, GPIO_PIN_4
#define WriteRed 	TIM3->CCR4
#define WriteGreen 	TIM3->CCR1
#define WriteBlue 	TIM2->CCR1

TIM_HandleTypeDef	Red;
TIM_OC_InitTypeDef 	RedPwmHandle;
TIM_HandleTypeDef	Green;
TIM_OC_InitTypeDef 	GreenPwmHandle;
TIM_HandleTypeDef	Blue;
TIM_OC_InitTypeDef 	BluePwmHandle;

void Peripherals_Init(void);
void LED_Init(void);
void TIMER_Init(void);
void colors_init(int r, int g, int b);

#endif /* RGB_CONTROLL_H_ */
