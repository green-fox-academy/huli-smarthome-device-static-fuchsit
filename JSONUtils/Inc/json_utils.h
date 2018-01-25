#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "jsmn.h"

int json_IsStrEqual(const char *json, jsmntok_t *tok, const char *s);

char** json_GetValuesForKeys(const char *json, char** keys, size_t keysSize, jsmntok_t *tokens, size_t tokensSize);

#endif // JSON_UTILS_H