/*
 * aircondi.h
 *
 *  Created on: Jan 25, 2018
 *      Author: Ádám
 */
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "heartbeat.h"
#include "device_info.h"

#ifndef EXAMPLE_USER_AIRCONDI_H_
#define EXAMPLE_USER_AIRCONDI_H_

/* Private typedef -----------------------------------------------------------*/
GPIO_InitTypeDef PWM_FAN;
GPIO_InitTypeDef temp_sensor_config;                              // configure GPIOs for I2C data and clock lines
TIM_HandleTypeDef TimHandle;
I2C_HandleTypeDef I2cHandle;
TIM_OC_InitTypeDef TimerOCConfig;

int reg;
int air_temperature;
int user_max;
int user_min;
int should_GGL_publish;


void get_temperatura();
void temp_set(int user_min, int user_max);
void temp_range_set_and_fan_controll(int user_min, int user_max);
void Fan_Init(void);
void temp_sensor_init(void);
void timer_pwm_config();

void airconditioner_temperature_range_parsing(char temperature_range[]);
void Project_Airconditioner (char *Temperature);

#endif /* EXAMPLE_USER_AIRCONDI_H_ */
