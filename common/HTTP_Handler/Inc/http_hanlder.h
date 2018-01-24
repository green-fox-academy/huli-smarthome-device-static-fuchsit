#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "main.h"
#include "device_info.h"

#define HTTPS_SUCCESS -5
#define SSPD_DISCOVERY_SUCCESS		-5

typedef enum HTTP_Response_Eval {
	SSDP_FUCHSIT_ANSWER,
	SSDP_NON_FUCHSIT_ANSWER,
	HTTPS_SEND_DEVICE_PARAMS,
	HTTPS_SUCCESSFUL_DEVICE_CONFIG,
	HTTPS_FAILED_TO_CONFIG_DEVICE,
	HTTPS_INVALID_REQUEST
} HTTP_Response_Eval;

int set_device_params (device_config_t *device, char in_body[]);

int separate_http_head_body(char in_buffer[], char in_head[], char in_body[]);

int prepare_http_response(device_config_t *device, HTTP_Response_Eval response_code, char snd[]);

int evaluate_http(device_config_t *device, char in_buffer[], char in_head[], char in_body[], char snd[]);

#endif //HTTP_HANDLER_H
