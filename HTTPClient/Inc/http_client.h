#ifndef HTTP_CLIENT
#define HTTP_CLIENT

#include <stdint.h>
#include "http_client_internal.h"
#include "net_secure.h"

/**
 * @brief   Creates the HTTP_Request structure based on the parameters
 *
 * @param   url             the URL of the service to call
 * @param   method          the HTTP_Method to use (GET/POST/etc.)
 * @param   contentType     the HTTP_ContentType in which the request should be encoded (JSON/form/etc.)
 * @param   request         a pointer to a HTTP_Request pointer to where the request will be created
 *
 * @retval  HTTP_RC_OK (0) on success, negative values on error
 */
int http_CreateRequest(char *url, HTTP_Method method, HTTP_ContentType contentType, HTTP_Request **request);

/**
 * @brief   Adds a HTTP header to the request
 *
 * @param   request         the request structure pointer in which the header should be created
 * @param   name            the name of the header
 * @param   value           the value of the header
 *
 * @retval  HTTP_RC_OK (0) on success, negative values on error
 */
int http_AddHeader(HTTP_Request *request, char *name, char *value);

/**
 * @brief   Adds a HTTP parameter to the request. This is allowed on GET requests or FORM encoded
 *          requests, otherwise will return error.
 *
 * @param   request         the request structure pointer to which the parameter should be added
 * @param   name            the name of the parameter
 * @param   value           the value of the parameter
 *
 * @retval  HTTP_RC_OK (0) on success, negative values on error
 */
int http_AddBodyParam(HTTP_Request *request, char *name, char *value);

/**
 * @brief   Sets the body of the request. This can be used only for non-GET methods with different
 *          encoding than FORM
 *
 * @param   request         the request structure pointer in which the body should be set
 * @param   body            the contents to add to the body
 *
 * @retval  HTTP_RC_OK (0) on success, negative values on error
 */
int http_SetBody(HTTP_Request *request, const char *body);

/**
 * @brief   Executes the HTTP request
 *
 * @param   ctx             the NetTransportContext to use with NetSecure API
 * @param   request         the request structure pointer to use
 * @param   response        a pointer to a response pointer on which the parsed
 *                          response will be stored
 *
 * @retval  HTTP_RC_OK (0) on success, negative values on error
 */
int http_Execute(NetTransportContext *ctx, HTTP_Request *request, HTTP_Response **response);

/**
 * @brief   Frees all the memory reserved during a call
 *
 * @param   request         the request structure
 * @param   response        the response structure
 *
 * @retval  HTTP_RC_OK (0) on success, negative values on error
 */
int http_Cleanup(HTTP_Request *request, HTTP_Response *response);

#endif //HTTP_CLIENT
