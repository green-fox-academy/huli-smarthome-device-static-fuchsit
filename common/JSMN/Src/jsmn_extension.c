#include "jsmn.h"
#include "jsmn_extension.h"
#include "device_info.h"

/*
 * A small example of jsmn parsing when JSON structure is known and number of
 * tokens is predictable.
 */

int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

/*
 * returns a copy of  a string to the destination memory location
 */

char *strndup(const char *s1, size_t n)
{
    char *copy= (char*)malloc(n + 1);
    memcpy(copy, s1, n);
    copy[n] = '\0';
    return copy;
};

/*
 * parse up a JSON string, which has a known structure and the token num can be determined
 */

int parse_JSON(device_config_t *conf_struct, char *JSON_STRING) {
	int i;
	int r;
	jsmn_parser p;
	jsmntok_t t[10]; /* We expect no more than 10 tokens */

	jsmn_init(&p);
	r = jsmn_parse(&p, JSON_STRING, strlen(JSON_STRING), t, sizeof(t)/sizeof(t[0]));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		printf("Object expected\n");
		return 1;
	}

	/* Loop over all keys of the root object */

	for (i = 1; i < r; i++) {
		if (jsoneq(JSON_STRING, &t[i], "Device") == 0) {
			/* We may use strndup() to fetch string value */
			printf("- Device: %.*s\n", t[i+1].end-t[i+1].start,
					JSON_STRING + t[i+1].start);
			conf_struct->device_name = strndup(JSON_STRING + t[i+1].start,t[i+1].end-t[i+1].start);
			i++;
		} else if (strstr(conf_struct->device_name, "LED_CONTROLLER") && jsoneq(JSON_STRING, &t[i], "Color") == 0) {
			 //We may additionally check if the value is either "true" or "false"
			printf("- Color: %.*s\n", t[i+1].end-t[i+1].start,
					JSON_STRING + t[i+1].start);
			conf_struct->color = strndup(JSON_STRING + t[i+1].start,t[i+1].end-t[i+1].start);
			i++;
		} else if (strstr(conf_struct->device_name, "AIR_CONDITIONER") && jsoneq(JSON_STRING, &t[i], "Temperature") == 0) {
			 //We may additionally check if the value is either "true" or "false"
			printf("- Temperature range: %.*s\n", t[i+1].end-t[i+1].start,
					JSON_STRING + t[i+1].start);
			conf_struct->temperature = strndup(JSON_STRING + t[i+1].start,t[i+1].end-t[i+1].start);
			i++;
		} else {
			printf("Unexpected key: %.*s\n", t[i].end-t[i].start,
					JSON_STRING + t[i].start);
		}
	}
	return EXIT_SUCCESS;
};
