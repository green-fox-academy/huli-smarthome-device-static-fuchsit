#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "main.h"
#include "device_info.h"

/*
 * return codes for HTTPS state and UDP_SSPD discovery state
 * neg value means that we break the given state loop
 */
#define HTTPS_SUCCESS           -5
#define SSPD_DISCOVERY_SUCCESS  -6

/*
 * enum for instructing prepare_http_response
 */
typedef enum HTTP_Response_Eval {
	SSDP_FUCHSIT_ANSWER,
	SSDP_NON_FUCHSIT_ANSWER,
	HTTPS_SEND_DEVICE_PARAMS,
	HTTPS_SUCCESSFUL_DEVICE_CONFIG,
	HTTPS_FAILED_TO_CONFIG_DEVICE,
	HTTPS_INVALID_REQUEST
} HTTP_Response_Eval;

/*
 * sets device parameters based on info received from master board
 * FUT
 * implement synced with master board
 */
int set_device_params (device_config_t *device, char in_body[]);

/*
 * in: in_buffer
 * out: in_head: header of incoming http request
 * out: in_body: body of incoming http request
 */
int separate_http_head_body(char in_buffer[], char in_head[], char in_body[]);

/*
 * prepared the http response answer based on the given state and resp code
 * returns message to snd[]
 */
int prepare_http_response(device_config_t *device, HTTP_Response_Eval response_code, char snd[]);

/*
 * evaluates the incoming http request and prepares the appropriate answer
 * in: in_buffer: incoming http request
 * out: in_head: header of incoming http request
 * out: in_body: body of incoming http request
 * out: snd: http answer to send back to client
 */
int evaluate_http(device_config_t *device, char in_buffer[], char in_head[], char in_body[], char snd[]);

#endif //HTTP_HANDLER_H
