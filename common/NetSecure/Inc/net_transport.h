#ifndef NET_TRANSPORT_H
#define NET_TRANSPORT_H

#include <stdint.h>
#include "wolfssl/ssl.h"

typedef enum SocketType {
	SOCKET_UDP,
	SOCKET_TCP
} SocketType;

typedef enum NetTransportRC {
	RC_SUCCESS = 0,
	RC_ERROR = -1,
	RC_TIMEOUT = -2
} NetTransportRC;

/**
 * The NetTransportContext in this project
 */
typedef struct NetTransportContext {
	uint32_t connectionId;
	uint8_t macAddr[6];
	WOLFSSL *ssl;
	WOLFSSL_CTX *sslCtx;
} NetTransportContext;

/*! \brief      Platform-dependent low level net transport initialization
 *  \discussion E.g.: on the IOT board initialize the WiFi, on F7 board
 *  			initialize the network with LwIP
 *  \param      netTransportContext      a pointer to the user-defined context
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_Init(void *netTransportContext);

/*! \brief      Platform-dependent connection setup
 *  \discussion Use the platform-specific tools to create a connection
 *  \param      netTransportContext     a pointer to the user-defined context
 *  \param		sockType				can be SCOKET_UDP or SOCKET_TCP
 *  \param		host					the host to connect to
 *  \param		port					the port to connect to
 *  \param		timeoutMs				timeout in milliseconds (optional)
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_Connect(void *netTransportContext, SocketType sockType, const char* host, uint16_t port, uint32_t timeoutMs);

/*! \brief      Platform-dependent connection close
 *  \discussion Use the platform-specific tools to create a connection
 *  \param      netTransportContext      a pointer to the user-defined context
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_Disconnect(void *netTransportContext);

/*! \brief      Platform-dependent data sending implementation
 *  \discussion Use the platform-specific tools to send data to the peer
 *  \param      netTransportContext     a pointer to the user-defined context
 *  \param		buffer					the buffer containing the data to send
 *  \param		bufferSz				the size of the data in the buffer to send
 *  \param		timeoutMs				timeout in milliseconds (optional)
 *
 *  \return     negative result codes for error, otherwise the length of the sent data
 */
int net_Send(void *netTransportContext, const char* buffer, const uint32_t bufferSz, uint32_t timeoutMs);

/*! \brief      Platform-dependent data sending implementation
 *  \discussion Use the platform-specific tools to receive data from the peer
 *  \param      netTransportContext     a pointer to the user-defined context
 *  \param		buffer					the buffer in which we need to read the data
 *  \param		bufferSz				the size of the data to be read
 *  \param		timeoutMs				timeout in milliseconds (optional)
 *
 *  \return     negative result codes for error, otherwise the length of the received data
 */
int net_Receive(void *netTransportContext, const char* buffer, const uint32_t bufferSz, uint32_t timeoutMs);

/*! \brief      Platform-dependent low level net transport deinitialization
 *  \discussion E.g.: free memory if it's necessary, etc.
 *  \param      netTransportcontext      a pointer to the user-defined context
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_Destroy(void *netTransportContext);

#endif //NET_TRANSPORT_H
