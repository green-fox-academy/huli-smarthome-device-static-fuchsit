#include "smart_plug.h"
#include "main.h"
#include "device_info.h"


void smart_plug_parsing(char *plug){

	if (strstr(plug, "on")) {
    	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, SET);
    } else if (strstr(plug, "off")) {
    	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, RESET);
    }
}

void plug_init() {
	__HAL_RCC_GPIOB_CLK_ENABLE();

	plug.Pin = GPIO_PIN_0;
	plug.Mode = GPIO_MODE_OUTPUT_PP;
	plug.Pull = GPIO_PULLDOWN;
	plug.Speed = GPIO_SPEED_HIGH;

	HAL_GPIO_Init(GPIOB, &plug);
}


