#ifndef HTTP_CLIENT_INTERNAL
#define HTTP_CLIENT_INTERNAL

#include <stdint.h>
#include "net_secure.h"

typedef enum HTTP_ResultCode {
    HTTP_RC_OK = 0, HTTP_RC_ILLEGAL_STATE = -1, HTTP_RC_INVALID_ARGS = -2, HTTP_RC_OUT_OF_MEMORY = -100,
} HTTP_ResultCode;

/**
 * HTTP request methods
 */
typedef enum HTTP_Method {
    HTTP_GET = 0, HTTP_POST, HTTP_DELETE, HTTP_PUT, HTTP_OPTIONS, HTTP_CONNECT
} HTTP_Method;

/**
 * String representation of methods
 */
static const char *HTTP_MethodStrs[6] = { "GET", "POST", "DELETE", "PUT", "OPTIONS", "CONNECT" };

/**
 * The supported connection modes
 */
typedef enum HTTP_ConnectionMode {
    HTTP_CONN_CLOSE = 0, HTTP_CONN_KEEPALIVE,
} HTTP_ConnectionMode;

/**
 * The Connection header's supported values
 */
static const char *HTTP_ConnectionModeStrs[2] = { "close", "keep-alive" };

/**
 * Supported content types
 */
typedef enum HTTP_ContentType {
    HTTP_CONTENT_TYPE_FORM = 0, HTTP_CONTENT_TYPE_TEXT_HTML, HTTP_CONTENT_TYPE_JSON
} HTTP_ContentType;

/**
 * Content type strings
 */
static const char *HTTP_ContentTypeStrs[3] = { "application/x-www-form-urlencoded", "text/html", "application/json" };

/**
 * The HTTP protocol
 */
typedef enum HTTP_Protocol {
    HTTP_HTTP, HTTP_HTTPS
} HTTP_Protocol;

typedef struct HTTP_KeyValue HTTP_KeyValue;

/**
 * Struct for a HTTP header or body params
 */
typedef struct HTTP_KeyValue {
    char *key;
    char *value;
    HTTP_KeyValue *_next;
} HTTP_KeyValue;

/**
 * Common structure for storing list of key-value pairs
 */
typedef struct HTTP_KeyValueList {
    HTTP_KeyValue *first;
    HTTP_KeyValue *last;
} HTTP_KeyValueList;

/**
 * Struct for storing HTTP request data
 */
typedef struct HTTP_Request {
    char *url;
    char *uri;
    char *host;
    uint16_t port;
    HTTP_Method method;
    HTTP_ContentType contentType;
    HTTP_Protocol protocol;
    HTTP_KeyValueList headers;
    HTTP_KeyValueList params;
    char *body;
} HTTP_Request;

/**
 * Struct for storing HTTP response data
 */
typedef struct HTTP_Response {
    uint16_t statusCode;
    char *statusMsg;
    HTTP_KeyValueList headers;
    char *body;
} HTTP_Response;

int http_CreateKeyValue(char *key, char *value, HTTP_KeyValue **keyValue);

void http_DeleteKeyValue(HTTP_KeyValue *keyValue);

int http_HostPortFromURL(char *url, HTTP_Protocol *protocol, char **host, uint16_t *port, char **uri);

int http_AddKeyValueToList(HTTP_KeyValueList *list, char *key, char *value);

int http_CleanupKeyValueList(HTTP_KeyValueList *kvList);

int http_WriteParams(HTTP_Request *request, char *buffer);

int http_URLDecode(char *result, char *toDecode);

int http_URLEncode(char *result, char *toEncode);

int http_AssembleRequest(HTTP_Request *request, char *buffer);

int http_CreateHttpBody(HTTP_Request *request);

int http_NetConnectInternal(NetTransportContext *ctx, HTTP_Request *request);

int http_NetSendInternal(NetTransportContext *ctx, HTTP_Protocol protocol, char* data, uint16_t dataSize);

int http_NetReceiveInternal(NetTransportContext *ctx, HTTP_Protocol protocol, char* data, uint16_t dataSize);

int http_NetDisconnectInternal(NetTransportContext *ctx, HTTP_Protocol protocol);

#endif // HTTP_CLIENT_INTERNAL
