#include "aircondi.h"
#include "main.h"

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

void airconditioner_temperature_range_parsing(char temperature_range[]){


    if (strcmp(temperature_range, "on") == 1) {
    	fan_state = FAN_ON;
    } else if (strcmp(temperature_range, "off") == 1) {
    	fan_state = FAN_OFF;
    } else {
    	user_max = strtol(temperature_range , NULL , 10);
    }
}
