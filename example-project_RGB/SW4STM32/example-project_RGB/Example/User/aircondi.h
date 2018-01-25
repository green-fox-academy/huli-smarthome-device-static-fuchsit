/*
 * aircondi.h
 *
 *  Created on: Jan 25, 2018
 *      Author: Ádám
 */

#ifndef EXAMPLE_USER_AIRCONDI_H_
#define EXAMPLE_USER_AIRCONDI_H_

/* Private typedef -----------------------------------------------------------*/
GPIO_InitTypeDef PWM_FAN;
GPIO_InitTypeDef temp_sensor_config;                              // configure GPIOs for I2C data and clock lines
TIM_HandleTypeDef TimHandle;
I2C_HandleTypeDef I2cHandle;
TIM_OC_InitTypeDef TimerOCConfig;

int reg = 0;
int temp = 0;
int user_max;
int user_min;

void SystemClock_Config(void);
void Peripherals_Init(void);
void get_temperatura();
void temp_set(int user_min, int user_max);

#endif /* EXAMPLE_USER_AIRCONDI_H_ */
