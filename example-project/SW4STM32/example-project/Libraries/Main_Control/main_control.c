/*
 * main_control.c
 *
 *  Created on: 2018. febr. 5.
 *      Author: sassb
 */
#include "main_control.h"

extern device_config_t device;

int RGB_init_flag = 0;
int Aircondi_init_flag = 0;


void main_control() {
if (strstr(device.device_name, "LED_CONTROLLER")) {
		device.device_type = LED_CONTROLLER;
	} else if (strstr(device.device_name, "COFFEE_MAKER")) {
		device.device_type = COFFEE_MAKER;
	} else if (strstr(device.device_name, "SMART_LIGTH")) {
		device.device_type = SMART_LIGTH;
	} else if (strstr(device.device_name, "AIR_CONDITIONER")) {
		device.device_type = AIR_CONDITIONER;
	}

	switch (device.device_type) { //old was (device.device_type)
    case LED_CONTROLLER:
    	if (RGB_init_flag  == 0) {
    		RGB_Init();
    		RGB_init_flag = 1;
    	}
    	Project_Led_Lights (device.color);
    	report_status_color ();
    	break;
    case COFFEE_MAKER:
    	//call COFFEE_MAKER;
    	break;
    case SMART_LIGTH:
    	//call SMART_LIGTH;
    	break;
    case AIR_CONDITIONER:
    	if (Aircondi_init_flag == 0) {
    		Aircondi_init();
    	    Aircondi_init_flag = 1;
    	}
    	Project_Airconditioner (device.temperature);
    	break;
    }
}

void report_status_color () {

	int rc;
	char buffer[50];
	sprintf (buffer, "{\"state\": \"%s\" }", device.color);
	if ((rc = GGL_MQTT_Publish("state", buffer))
			!= RC_SUCCESS) {
		printf("ERROR: GGL_MQTT_Publish FAILED %d - %s\r\n", rc,
				MqttClient_ReturnCodeToString(rc));
	}
}

void report_fan_state_and_temperature () {

	int rc;
	char buffer[50];
	sprintf (buffer, "{\"Temperature state\": \"%d\" }", air_temperature);
	if ((rc = GGL_MQTT_Publish("state", buffer))
			!= RC_SUCCESS) {
		printf("ERROR: GGL_MQTT_Publish FAILED %d - %s\r\n", rc,
				MqttClient_ReturnCodeToString(rc));
	}
}
