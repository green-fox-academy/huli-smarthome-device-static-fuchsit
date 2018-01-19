#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "http_client.h"
#include "smarthome_log.h"

#define	HTTP_REQUEST_MAX_SIZE			2048

#define HTTP_ENTER(fnc)					SHOME_LogEnter("http", fnc)
#define HTTP_MSG(fmt, ...)				SHOME_LogMsg("http", fmt, ##__VA_ARGS__)
#define HTTP_EXIT(fnc, rc, fail)		SHOME_LogExit("http", fnc, rc, fail)

int http_ReadResponse(NetTransportContext *ctx, HTTP_Request *request, HTTP_Response **responsePptr);

typedef enum HTTP_ResponseParsingState {
    HTTP_RP_STATUS_LINE, HTTP_RP_HEADER, HTTP_RP_BODY
} HTTP_ResponseParsingState;

int http_CreateRequest(char *url, HTTP_Method method, HTTP_ContentType contentType, HTTP_Request **requestPPtr) {
    HTTP_ENTER("http_CreateRequest");
    *requestPPtr = malloc(sizeof(HTTP_Request));
    HTTP_Request *request = *requestPPtr;
    if (request == NULL) {
        HTTP_MSG("ERROR: out of memory\r\n");
        HTTP_EXIT("http_CreateRequest", HTTP_RC_OUT_OF_MEMORY, 1);
        return HTTP_RC_OUT_OF_MEMORY;
    }
    request->url = url;
    request->method = method;
    request->contentType = contentType;
    request->body = NULL;
    request->headers.first = request->headers.last = NULL;
    request->params.first = request->params.last = NULL;

    char *urlCpy = malloc(strlen(url) + 1);
    strcpy(urlCpy, url);
    int rc = http_HostPortFromURL(urlCpy, &request->protocol, &request->host, &request->port, &request->uri);
    if (rc != HTTP_RC_OK) {
        HTTP_MSG("ERROR: could not create url: %d\r\n", rc);
        HTTP_EXIT("http_CreateRequest", rc, 1);
        return rc;
    }

    rc = http_AddHeader(request, "Host", request->host);
    if (rc != HTTP_RC_OK) {
        HTTP_MSG("ERROR: could not create header: %d\r\n", rc);
        HTTP_EXIT("http_CreateRequest", rc, 1);
        return rc;
    }

    rc = http_AddHeader(request, "Content-Type", (char*) HTTP_ContentTypeStrs[request->contentType]);
    if (rc != HTTP_RC_OK) {
        HTTP_MSG("ERROR: could not create header: %d\r\n", rc);
        HTTP_EXIT("http_CreateRequest", rc, 1);
        return rc;
    }

    rc = http_AddHeader(request, "Accept-Encoding", "identity");
    if (rc != HTTP_RC_OK) {
        HTTP_MSG("ERROR: could not create header: %d\r\n", rc);
        HTTP_EXIT("http_CreateRequest", rc, 1);
        return rc;
    }

    rc = http_AddHeader(request, "Connection", "close");
    if (rc != HTTP_RC_OK) {
        HTTP_MSG("ERROR: could not create header: %d\r\n", rc);
        HTTP_EXIT("http_CreateRequest", rc, 1);
        return rc;
    }

    HTTP_EXIT("http_CreateRequest", HTTP_RC_OK, 0);
    return HTTP_RC_OK;
}

int http_AddHeader(HTTP_Request *request, char *name, char *value) {
    HTTP_ENTER("http_AddRequestHeader");
    int rc = http_AddKeyValueToList(&request->headers, name, value);
    if (rc != HTTP_RC_OK) {
        HTTP_MSG("ERROR: could not create header: %d\r\n", rc);
        HTTP_EXIT("http_AddRequestHeader", rc, 1);
        return rc;
    }
    HTTP_EXIT("http_AddRequestHeader", HTTP_RC_OK, 0);
    return HTTP_RC_OK;
}

int http_AddBodyParam(HTTP_Request *request, char *name, char *value) {
    HTTP_ENTER("http_AddBodyParam");
    if (request->contentType != HTTP_CONTENT_TYPE_FORM) {
        HTTP_MSG("ERROR: adding body param is allowed only for HTTP_CONTENT_TYPE_FORM content type!\r\n");
        HTTP_EXIT("http_AddBodyParam", HTTP_RC_ILLEGAL_STATE, 0);
        return HTTP_RC_ILLEGAL_STATE;
    }
    int rc = http_AddKeyValueToList(&request->params, name, value);
    if (rc != HTTP_RC_OK) {
        HTTP_MSG("ERROR: could not create param: %d\r\n", rc);
        HTTP_EXIT("http_AddBodyParam", rc, 1);
        return rc;
    }
    HTTP_EXIT("http_AddBodyParam", HTTP_RC_OK, 0);
    return HTTP_RC_OK;
}

int http_SetBody(HTTP_Request *request, const char *body) {
    HTTP_ENTER("http_SetBody");
    request->body = malloc(strlen(body) + 1);
    if (request->body == NULL) {
        HTTP_MSG("ERROR: out of memory\r\n");
        HTTP_EXIT("http_SetBody", HTTP_RC_OUT_OF_MEMORY, 1);
        return HTTP_RC_OUT_OF_MEMORY;
    }
    strcpy(request->body, body);
    HTTP_EXIT("http_SetBody", HTTP_RC_OK, 0);
    return HTTP_RC_OK;
}

int http_Execute(NetTransportContext *ctx, HTTP_Request *request, HTTP_Response **response) {
    HTTP_ENTER("http_Execute");
    char rq[HTTP_REQUEST_MAX_SIZE];
    uint16_t rqSize = http_AssembleRequest(request, rq);
    if (rqSize > HTTP_REQUEST_MAX_SIZE - 1) {
        HTTP_MSG("ERROR: buffer overflow, the request size(%d) is bigger than the buffer(%d)\r\n", rqSize, HTTP_REQUEST_MAX_SIZE - 1);
        HTTP_EXIT("http_Execute", -1, 1);
        return HTTP_RC_OUT_OF_MEMORY;
    }

    int rc = http_NetConnectInternal(ctx, request);
    if (rc != 0) {
        HTTP_MSG("ERROR: HTTP connection failed (%d)!\r\n", rc);
        HTTP_EXIT("http_Execute", rc, 1);
        return rc;
    }

    rc = http_NetSendInternal(ctx, request->protocol, rq, rqSize);
    if (rc < 0) {
        HTTP_MSG("ERROR: HTTP send failed (%d)!\r\n", rc);
        http_NetDisconnectInternal(ctx, request->protocol);
        HTTP_EXIT("http_Execute", rc, 1);
        return rc;
    }

    rc = http_ReadResponse(ctx, request, response);
    if (rc < 0) {
        HTTP_MSG("ERROR: http_ReadResponse (%d)!\r\n", rc);
        http_NetDisconnectInternal(ctx, request->protocol);
        HTTP_EXIT("http_Execute", rc, 1);
        return rc;
    }

    rc = http_NetDisconnectInternal(ctx, request->protocol);
    if (rc != 0) {
        HTTP_MSG("ERROR: HTTP disconnect failed (%d)!\r\n", rc);
        HTTP_EXIT("http_Execute", rc, 1);
        return rc;
    }

    HTTP_EXIT("http_Execute", HTTP_RC_OK, 0);
    return HTTP_RC_OK;
}

int http_Cleanup(HTTP_Request *request, HTTP_Response *response) {
    HTTP_ENTER("http_Cleanup");

    // cleanup request
    if (request->body != NULL) {
        free(request->body);
    }
    free(request->host);
    free(request->uri);
    free(request->url);
    http_CleanupKeyValueList(&request->headers);
    http_CleanupKeyValueList(&request->params);
    free(request);

    // cleanup response
    if (response->body != NULL) {
        free(response->body);
    }
    free(response->statusMsg);
    http_CleanupKeyValueList(&response->headers);
    free(response);
    HTTP_EXIT("http_Cleanup", HTTP_RC_OK, 0);
    return HTTP_RC_OK;
}

int http_ReadResponse(NetTransportContext *ctx, HTTP_Request *request, HTTP_Response **responsePptr) {
    HTTP_ENTER("http_ReadResponse");

    *responsePptr = malloc(sizeof(HTTP_Response));
    if (*responsePptr == NULL) {
        HTTP_MSG("ERROR: out of memory!\r\n");
        HTTP_EXIT("http_ReadResponse", HTTP_RC_OUT_OF_MEMORY, 1);
        return HTTP_RC_OUT_OF_MEMORY;

    }
    HTTP_Response *response = *responsePptr;
    response->headers.first = response->headers.last = NULL;
    response->body = NULL;

    char rsp[HTTP_REQUEST_MAX_SIZE];
    int rc = http_NetReceiveInternal(ctx, request->protocol, rsp, HTTP_REQUEST_MAX_SIZE - 1);
    if (rc < 0) {
        HTTP_MSG("ERROR: HTTP receive failed (%d)!\r\n", rc);
        HTTP_EXIT("http_Execute", rc, 1);
        return rc;
    }

    char *lineBegin = rsp;
    char *lineEnd = strstr(rsp, "\r\n");
    HTTP_ResponseParsingState state = HTTP_RP_STATUS_LINE;
    while (lineEnd != NULL) {
        lineEnd[0] = '\0';
        char *sl_code, *sl_msg, *h_name, *h_value;
        switch (state) {
            case HTTP_RP_STATUS_LINE:
                strtok(lineBegin, " ");
                sl_code = strtok(NULL, " ");
                sl_msg = strtok(NULL, " ");

                response->statusCode = atoi(sl_code);
                response->statusMsg = strdup(sl_msg);

                state = HTTP_RP_HEADER;
                break;
            case HTTP_RP_HEADER:
                if (!strlen(lineBegin)) {
                    state = HTTP_RP_BODY;
                    break;
                }

                h_name = strtok(lineBegin, ": ");
                h_value = strtok(NULL, "");
                http_AddKeyValueToList(&response->headers, h_name, h_value);
                break;
            default:
                break;
        }
        lineEnd[0] = '\r';
        lineBegin = lineEnd + 2;
        if (state == HTTP_RP_BODY) {
            lineEnd = NULL;
            break;
        }
        lineEnd = strstr(lineBegin, "\r\n");
    }
    uint16_t bodyLength = strlen(lineBegin);
    if (bodyLength) {
        response->body = malloc(bodyLength + 1);
        if (response->body == NULL) {
            HTTP_MSG("ERROR: out of memory!\r\n");
            HTTP_EXIT("http_ReadResponse", HTTP_RC_OUT_OF_MEMORY, 1);
            return HTTP_RC_OUT_OF_MEMORY;
        }
        strcpy(response->body, lineBegin);
    }
    HTTP_EXIT("http_ReadResponse", HTTP_RC_OK, 0);
    return HTTP_RC_OK;
}
