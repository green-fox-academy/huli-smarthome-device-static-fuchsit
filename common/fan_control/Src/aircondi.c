/*
 * aircondi.c

 *
 *  Created on: Jan 25, 2018
 *      Author: Ádám
 */

#include "main.h"
#include "aircondi.h"

int is_fan_working = FALSE;
int air_temperature = 0;
int fan_state = FAN_OFF;

void temp_range_set_and_fan_controll(int user_min, int user_max) {

	get_temperatura();  // updates temp

	switch(fan_state) {
		case FAN_OFF :
			TIM2 -> CCR3 = 0;
			should_GGL_publish = TRUE;
			//update_message_buffer(&message_buffer);
			break;

		case FAN_ON :
			TIM2 -> CCR3 = 99;
			should_GGL_publish = TRUE;
			//update_message_buffer(&message_buffer);
			break;

		case THERMOSTAT :
			if (air_temperature >= user_max + 1) {
				if (is_fan_working == FALSE) {
					TIM2 -> CCR3 = 99; // turn on fan
					printf("set flag state to %d\n", is_fan_working);
					should_GGL_publish = TRUE;
					//update_message_buffer(&message_buffer);
				}
				is_fan_working = TRUE;
			} else if (air_temperature <= user_max - 1) {
				if (is_fan_working) {
					TIM2 -> CCR3 = 0;
					printf("set flag state to %d\n", is_fan_working);
					should_GGL_publish = TRUE;
					//update_message_buffer(&message_buffer);
				}
				is_fan_working = FALSE;
			}
			break;
	}
}



void Fan_Init(void) {

	__HAL_RCC_GPIOA_CLK_ENABLE();

	PWM_FAN.Alternate 	= GPIO_AF1_TIM2;
	PWM_FAN.Mode 		= GPIO_MODE_AF_PP;
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
	HAL_I2C_Master_Receive(&I2cHandle , 0b1001000<<1, &air_temperature , 1 , 100);

}

void timer_pwm_config() {

	__HAL_RCC_TIM2_CLK_ENABLE();

	TimHandle.Instance               = TIM2;
	TimHandle.Init.Prescaler         = 0;
	TimHandle.Init.Period            = 100;
	TimHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;

	TimerOCConfig.OCMode = TIM_OCMODE_PWM1;
	TimerOCConfig.Pulse = 0;								//MIN 55 to start

	HAL_TIM_PWM_Init(&TimHandle);
	HAL_TIM_PWM_ConfigChannel(&TimHandle, &TimerOCConfig, TIM_CHANNEL_3);

	HAL_TIM_PWM_Start(&TimHandle , TIM_CHANNEL_3);
}

void airconditioner_temperature_range_parsing(char temperature_range[]){

   /* char *text_one;
    char *text_two;

    const char ch[2] = " ";
    char *token;
    token = strtok(temperature_range, ch);
    text_one = token;

    while(token != NULL){
        text_two = token;
        token = strtok(NULL, ch);
    }
    user_min = strtol(text_two , NULL , 10);
    user_max = strtol(text_one , NULL , 10);
    int temp_buff = 0;*/

    if (strcmp(temperature_range, "on") == 1) {
    	fan_state = FAN_ON;
    }
    else if (strcmp(temperature_range, "off") == 1) {
    	fan_state = FAN_OFF;
    }
    else {
    	user_max = strtol(temperature_range , NULL , 10);
    }
        int temp_buff = 0;

}

void Project_Airconditioner (char *Temperature) {
	timer_pwm_config();
	Fan_Init();
	temp_sensor_init();
	airconditioner_temperature_range_parsing(Temperature);
}

