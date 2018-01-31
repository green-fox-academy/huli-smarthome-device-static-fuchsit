/**
This file is an extension for the original JSMN parser with 3 additional custom functions.
 */

/*
 * this is a simple parser for JSON formatted string.
 * we use it for parsing incoming HTTPS, or MQTT bodies
 * and to updated the config structure of conf_t
 */

#ifndef __JSMN_H_
#define __JSMN_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "main.h"
#include "device_info.h"
#include "jsmn.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * not standard JSMN library elements.
 * were created for the smart home project
 */

/*
 * checks if the jsmn token's key value is equal to the searches key value
 * json: the original json string
 * tok: jsmn token
 * s: looked up key value
 */
int jsoneq(const char *json, jsmntok_t *tok, const char *s);
/*
 * returns a copy of a string (pointer) from the memory location of s1
 * up to n bytes long
 */
char *strndup(const char *s1, size_t n);
/*
 * paprses a JSON_STRING, and copies its value to the corresponding
 * members of the conf_struct.
 */
int parse_JSON(device_config_t *conf_struct, char *JSON_STRING);


#ifdef __cplusplus
}
#endif

#endif /* __JSMN_H_ */
