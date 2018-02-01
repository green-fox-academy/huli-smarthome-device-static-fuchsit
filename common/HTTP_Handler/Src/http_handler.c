#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "http_hanlder.h"
#include "device_info.h"
#include "jsmn.h"

device_config_t device;

int set_device_params (device_config_t *device, char in_body[])
{
    //parse_JSON(device, in_body);

    return 1;
}

int separate_http_head_body(char in_buffer[], char in_head[], char in_body[])
{
    char *first = NULL;
    char *last = NULL;

    first = strchr(in_buffer, '{');
    last = strrchr(in_buffer, '}');

    printf("first: %s\n", first);
    printf("last: %s\n", last);

    if (first != last) { // there is a JSON body
		size_t first_pos =  first - &in_buffer[0];
		size_t last_pos =  last - &in_buffer[0];

		printf("first_pos: %d\n", first_pos);
		printf("last_pos: %d\n", last_pos);

		if (first_pos != 0) { // if there is only JSON body - ie in HTTPS state
			strncpy(in_head, in_buffer, first_pos);
			printf("in sprt head and body 1\n");
			in_head[first_pos] = '\0';

			printf("in sprt head and body 2\n");
		}

		strncpy(in_body, first, last_pos - first_pos + 1);
		in_body[last_pos - first_pos + 1] = '\0';

		printf("in sprt head and body 3\n");
    } else {			// there is NO JSON body - ie in SSDP state
    	strcpy(in_head, in_buffer);
    }

    return 0;
}

int prepare_http_response(device_config_t *device, HTTP_Response_Eval response_code, char snd[])
{
    switch (response_code) {
        case SSDP_FUCHSIT_ANSWER:
            strcpy(snd, "HTTP/1.0 200 OK\r\nContent-Type: \"application/json\"\r\n\r\n");
            break;
         case SSDP_NON_FUCHSIT_ANSWER:
             strcpy(snd, "HTTP/1.0 400 OK\r\nContent-Type: \"application/json\"\r\n\r\n");
            break;
         case HTTPS_SEND_DEVICE_PARAMS:
             strcpy(snd, "invalid request\n");
            break;
         case HTTPS_SUCCESSFUL_DEVICE_CONFIG:
             strcpy(snd, "device params set\n");
            break;
         case HTTPS_FAILED_TO_CONFIG_DEVICE:
            strcpy(snd, "device params coudnt be set\n");
            break;
         case HTTPS_INVALID_REQUEST:
             strcpy(snd, "invalid request\n");
            break;
    }

    return 0;
}

int evaluate_http(device_config_t *device, char in_buffer[], char in_head[], char in_body[], char snd[])
{
    int state_success = 0;

    separate_http_head_body(in_buffer, in_head, in_body);

    printf("in evaluate http\n");

    switch (device->state_of_device) {
        case STATE_SSDP_DISCOVERY:
            if (strstr(in_head, "fuchsit")) {
            	printf("in evaluate http fuchsit answer\n");
                prepare_http_response(device, SSDP_FUCHSIT_ANSWER, snd);
                state_success = 1;
            } else {
                prepare_http_response(device, SSDP_NON_FUCHSIT_ANSWER, snd);
            }
            break;
        case STATE_HTTPS_SERVER:
            if (strstr(in_head, "getDeviceParams")) {
                prepare_http_response(device, HTTPS_SEND_DEVICE_PARAMS, snd);
            } else if (strstr(in_head, "setDeviceParams")){
                int dev_pars_configed = set_device_params(device, in_body);
                if (dev_pars_configed) {
                	// connect to mqtt - válasz, h ok
                    prepare_http_response(device, HTTPS_SUCCESSFUL_DEVICE_CONFIG, snd);
                    state_success = 1;
                }
                else {
                    prepare_http_response(device, HTTPS_FAILED_TO_CONFIG_DEVICE, snd);
                }

            } else {
                prepare_http_response(device, HTTPS_INVALID_REQUEST, snd);
            }
            break;

    }

    return state_success;
}
