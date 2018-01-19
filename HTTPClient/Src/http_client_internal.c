#include <string.h>
#include <ctype.h>
#include "http_client_internal.h"
#include "smarthome_log.h"

#define HTTPI_ENTER(fnc)                 SHOME_LogEnter("httpi", fnc)
#define HTTPI_MSG(fmt, ...)              SHOME_LogMsg("httpi", fmt, ##__VA_ARGS__)
#define HTTPI_EXIT(fnc, rc, fail)        SHOME_LogExit("httpi", fnc, rc, fail)

#define HTTP_DEFAULT_TIMEOUT            30000

static const char httpi_RESERVED_CHARS[] = {'!', '#', '$', '&', '\'', '(', ')', '*', '+', ',', ':', ';', '=', '?', '@', '[', ']' };

/**
 * URL decoding state machine helper
 */
typedef enum UrlDecodeState {
    URLD_NONE, URLD_FIRST_NIBBLE, URLD_SECOND_NIBBLE
} UrlDecodeState;

int http_CreateKeyValue(char *key, char *value, HTTP_KeyValue **keyValuePPtr) {
    HTTPI_ENTER("http_CreateKeyValue");
    *keyValuePPtr = malloc(sizeof(HTTP_KeyValue));
    HTTP_KeyValue *keyValue = *keyValuePPtr;
    if (keyValue == NULL) {
        HTTPI_MSG("ERROR: out of memory\r\n");
        HTTPI_EXIT("http_CreateKeyValue", HTTP_RC_OUT_OF_MEMORY, 1);
        return HTTP_RC_OUT_OF_MEMORY;
    }
    keyValue->key = malloc(strlen(key) + 1);
    keyValue->value = malloc(strlen(value) + 1);
    if (keyValue->key == NULL || keyValue->value == NULL) {
        HTTPI_MSG("ERROR: out of memory\r\n");
        HTTPI_EXIT("http_CreateKeyValue", HTTP_RC_OUT_OF_MEMORY, 1);
        return HTTP_RC_OUT_OF_MEMORY;
    }

    strcpy(keyValue->key, key);
    strcpy(keyValue->value, value);

    HTTPI_EXIT("http_CreateKeyValue", 0, 0);
    return HTTP_RC_OK;
}

void http_DeleteKeyValue(HTTP_KeyValue *keyValue) {
    if (keyValue == NULL) {
        return;
    }
    if (keyValue->key != NULL) {
        free(keyValue->key);
    }
    if (keyValue->value != NULL) {
        free(keyValue->value);
    }
    free(keyValue);
    keyValue = NULL;
}

int http_NetConnectInternal(NetTransportContext *ctx, HTTP_Request *request) {
    return request->protocol == HTTP_HTTP ?
            net_Connect(ctx, SOCKET_TCP, request->host, request->port, HTTP_DEFAULT_TIMEOUT) : net_TLSConnect(ctx, SOCKET_TCP, request->host, request->port, HTTP_DEFAULT_TIMEOUT);
}

int http_NetSendInternal(NetTransportContext *ctx, HTTP_Protocol protocol, char* data, uint16_t dataSize) {
    return protocol == HTTP_HTTP ? net_Send(ctx, data, dataSize, HTTP_DEFAULT_TIMEOUT) : net_TLSSend(ctx, data, dataSize, HTTP_DEFAULT_TIMEOUT);
}

int http_NetReceiveInternal(NetTransportContext *ctx, HTTP_Protocol protocol, char* data, uint16_t dataSize) {
    return protocol == HTTP_HTTP ? net_Receive(ctx, data, dataSize, HTTP_DEFAULT_TIMEOUT) : net_TLSReceive(ctx, data, dataSize, HTTP_DEFAULT_TIMEOUT);
}

int http_NetDisconnectInternal(NetTransportContext *ctx, HTTP_Protocol protocol) {
    return protocol == HTTP_HTTP ? net_Disconnect(ctx) : net_TLSDisconnect(ctx);
}

int http_HostPortFromURL(char *url, HTTP_Protocol *protocol, char **host, uint16_t *port, char **uri) {
    HTTPI_ENTER("http_HostPortFromURL");
    char *protocolStr = strtok(url, ":");
    char *hostPortStr = strtok(NULL, "/");
    *uri = strtok(NULL, "");
    *host = strtok(hostPortStr, ":");
    char *portStr = strtok(NULL, ":");
    *port = 0;

    // get the port
    if (portStr != NULL) {
        *port = strtol(portStr, (char**) NULL, 10);
        if (*port == 0) {
            HTTPI_MSG("ERROR: Invalid port: %s\r\n", portStr);
            HTTPI_EXIT("http_HostPortFromURL", HTTP_RC_INVALID_ARGS, 1);
            return HTTP_RC_INVALID_ARGS;
        }
    }

    // get the protocol and if the port is not determined yet,
    // set it to default (http->80, https->443)
    if (strcasecmp("HTTP", protocolStr) == 0) {
        *port = *port > 0 ? *port : 80;
        *protocol = HTTP_HTTP;
    }
    else if (strcasecmp("HTTPS", protocolStr) == 0) {
        *port = *port > 0 ? *port : 443;
        *protocol = HTTP_HTTPS;
    }
    else {
        HTTPI_MSG("ERROR: Unknown protocol: %s\r\n", protocolStr);
        HTTPI_EXIT("http_HostPortFromURL", HTTP_RC_INVALID_ARGS, 1);
        return HTTP_RC_INVALID_ARGS;
    }

    HTTPI_EXIT("http_HostPortFromURL", 0, 0);
    return HTTP_RC_OK;
}

int http_URLEncode(char *result, char *toEncode) {
    if (toEncode == NULL) {
        return 0;
    }
    int len = strlen(toEncode);
    int resPos = 0;
    for (int i = 0; i < len; i++) {
        char c = toEncode[i];
        if (memchr(httpi_RESERVED_CHARS, c, sizeof(httpi_RESERVED_CHARS)) == NULL) {
            result[resPos++] = c;
            continue;
        }
        char *insert = result + resPos;
        sprintf(insert, "%%%02X", c);
        resPos += 3;
    }
    result[resPos] = '\0';
    return resPos;
}

int http_URLDecode(char *result, char *toDecode) {
    if (toDecode == NULL) {
        return 0;
    }
    uint16_t len = strlen(toDecode);
    uint16_t resPos = 0;
    UrlDecodeState state = URLD_NONE;
    char hexChar = 0;
    for (uint16_t i = 0; i < len; i++) {
        char c = toDecode[i];
        switch (state) {
            case URLD_NONE:
                if (c == '%') {
                    state = URLD_FIRST_NIBBLE;
                    break;
                }
                result[resPos++] = c;
                break;
            case URLD_FIRST_NIBBLE:
                if (!isxdigit(c)) {
                    return HTTP_RC_INVALID_ARGS;
                    break;
                }
                hexChar = (c - (c > 96 ? 87 : c > 64 ? 55 : 48)) * 16;
                state = URLD_SECOND_NIBBLE;
                break;
            case URLD_SECOND_NIBBLE:
                if (!isxdigit(c)) {
                    return HTTP_RC_INVALID_ARGS;
                    break;
                }
                hexChar += c - (c > 96 ? 87 : c > 64 ? 55 : 48);
                result[resPos++] = hexChar;
                state = URLD_NONE;
        }
    }
    result[resPos] = '\0';
    return resPos;
}


int http_AssembleRequest(HTTP_Request *request, char *buffer) {
    HTTPI_ENTER("http_AssembleRequest");
    char *wptr = buffer;
    uint16_t contentLength = http_CreateHttpBody(request);
    if (contentLength < 0) {
        HTTPI_MSG("ERROR: assembling body failed (%d)!\r\n", contentLength);
        HTTPI_EXIT("http_AssembleRequest", contentLength, 1);
        return contentLength;
    }
    char contentLengthStr[10];
    itoa(contentLength, contentLengthStr, 10);

    int rc = http_AddKeyValueToList(&request->headers, "Content-Length", contentLengthStr);
    if (rc != HTTP_RC_OK) {
        HTTPI_MSG("ERROR: adding content-length header failed (%d)!\r\n", rc);
        HTTPI_EXIT("http_AssembleRequest", rc, 1);
        return rc;
    }
    // when writing GET request with parameters, we need to append the parameters to the URI
    if (request->method == HTTP_GET && request->params.first != NULL) {
        wptr += sprintf(wptr, "%s /%s?%s", HTTP_MethodStrs[request->method], request->uri, request->body);
    }
    else {
        // not a GET request or there's no param, the body will come in the end
        wptr += sprintf(wptr, "%s /%s HTTP/1.1\r\n", HTTP_MethodStrs[request->method], request->uri);
    }

    // writing the HTTP headers
    HTTP_KeyValue *header = request->headers.first;
    while (header != NULL) {
        wptr += sprintf(wptr, "%s: %s\r\n", header->key, header->value);
        header = header->_next;
    }
    // writing the second \r\n to mark that the body comes
    wptr += sprintf(wptr, "\r\n");
    if (contentLength > 0) {
        wptr += sprintf(wptr, "%s", request->body);
    }
    HTTPI_EXIT("http_AssembleRequest", 0, 0);
    return wptr - buffer;
}

int http_CreateHttpBody(HTTP_Request *request) {
    if (request->contentType != HTTP_CONTENT_TYPE_FORM) {
        if (request->body == NULL) {
            return HTTP_RC_OK;
        }
        return strlen(request->body);
    }

    if (request->params.first == NULL) {
        return HTTP_RC_OK;
    }
    uint16_t bodyLength = 0;
    HTTP_KeyValue *p = request->params.first;
    while (p != NULL) {
        if (p->value == NULL) {
            p->value = "";
        }
        // pessimistic estimation for the current param length
        char tmpP[strlen(p->key) + 2 + strlen(p->value) * 3];
        char *tmpPWriter = tmpP;
        if (p != request->params.first) {
            tmpPWriter[0] = '&';
            tmpPWriter++;
        }

        tmpPWriter += sprintf(tmpPWriter, "%s=", p->key);
        tmpPWriter += http_URLEncode(tmpPWriter, p->value);

        uint16_t prevBodyEnd = bodyLength;
        bodyLength += strlen(tmpP);

        request->body = realloc(request->body, bodyLength + 1);
        if (request->body == NULL) {
            return HTTP_RC_OUT_OF_MEMORY;
        }
        strcpy((request->body + prevBodyEnd), tmpP);
        p = p->_next;
    }
    return bodyLength;
}

int http_AddKeyValueToList(HTTP_KeyValueList *list, char *key, char *value) {
    HTTP_KeyValue *kv;
    int rc = http_CreateKeyValue(key, value, &kv);
    if (rc != HTTP_RC_OK) {
        return rc;
    }

    kv->_next = NULL;
    if (list->first == NULL) {
        list->first = list->last = kv;
    }
    else {
        list->last->_next = kv;
        list->last = kv;
    }
    return HTTP_RC_OK;
}

int http_CleanupKeyValueList(HTTP_KeyValueList *kvList) {
    if (kvList->first == NULL) {
        return HTTP_RC_OK;
    }

    HTTP_KeyValue *kv = kvList->first;
    while (kv != NULL) {
        HTTP_KeyValue *nextTmp = kv->_next;
        free(kv->key);
        free(kv->value);
        free(kv);
        kv = nextTmp;
    }
    kvList->first = kvList->last = NULL;
    return HTTP_RC_OK;
}
