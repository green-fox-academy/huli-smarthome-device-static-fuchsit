#include "json_utils.h"
#include "smarthome_log.h"

#define JSON_ENTER(fnc)              SHOME_LogEnter("json", fnc)
#define JSON_MSG(fmt, ...)           SHOME_LogMsg("json", fmt, ##__VA_ARGS__)
#define JSON_EXIT(fnc, rc, fail)     SHOME_LogExit("json", fnc, rc, fail)

int json_IsStrEqual(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start && strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

char** json_GetValuesForKeys(const char *json, char** keys, size_t keysSize, jsmntok_t *tokens, size_t tokensSize) {
    char **result = malloc(keysSize * sizeof(char*));
    if (result == NULL) {
        JSON_MSG("ERROR: out of memory\r\n");
        return NULL;
    }

    for (int i = 0; i < keysSize; i++) {
        result[i] = NULL;
    }

    for (int i = 0; i < tokensSize; i++) {
        jsmntok_t tok = tokens[i];
        for (int j = 0; j < keysSize; j++) {
            char* key = keys[j];
            if (json_IsStrEqual(json, &tok, key) == 0) {
                size_t vLen = tokens[i + 1].end - tokens[i + 1].start;
                result[j] = malloc(vLen + 1);
                if (result[j] == NULL) {
                    JSON_MSG("ERROR: out of memory\r\n");
                    return NULL;
                }
                memcpy(result[j], json + tokens[i + 1].start, vLen);
                result[j][vLen] = '\0';
            }
        }
    }
    return result;
}
