#ifndef NET_TRANSPORT_H
#define NET_TRANSPORT_H

#include <stdint.h>
#include "net_settings.h"
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
 * The NetTransportContext
 */
typedef struct NetTransportContext {
	NetConnectionContext connection;
	uint8_t macAddr[6];
	WOLFSSL *ssl;
	WOLFSSL_CTX *sslCtx;
} NetTransportContext;

/**
 * This method will be called once a new client gets connected in server mode. This should return
 * a negative integer in order to close stop the server mode.
 */
typedef int (*NetHandleClientConnectionCallback)(NetTransportContext *ctx);

/*! \brief      Platform-dependent low level net transport initialization
 *  \discussion E.g.: on the IOT board initialize the WiFi, on F7 board
 *  			initialize the network with LwIP
 *  \param      ctx      a pointer to the user-defined context
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_Init(NetTransportContext *ctx);

/*! \brief      Platform-dependent DNS lookup
 *  \discussion Resolves the IP address of the specified host in the resultIp parameter
 *
 *  \param		host					The host string to lookup
 *  \param		resultIp				the IP address array, which contains the 4 sections of IPv4
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_DNSLookup(const char* host, uint8_t* resultIp);

/*! \brief      Platform-dependent connection setup
 *  \discussion Use the platform-specific tools to create a connection
 *  \param      ctx					    a pointer to the user-defined context
 *  \param		sockType				can be SCOKET_UDP or SOCKET_TCP
 *  \param		host					the host to connect to
 *  \param		port					the port to connect to
 *  \param		timeoutMs				timeout in milliseconds (optional)
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_Connect(NetTransportContext *ctx, SocketType sockType, const char* host, uint16_t port, uint32_t timeoutMs);

/*! \brief      Platform-dependent connection setup withut DNS resolving
 *  \discussion The same as net_Connect, but this expects an IP address instead
 *  			of host name
 *  \param      ctx					    a pointer to the user-defined context
 *  \param		sockType				can be SCOKET_UDP or SOCKET_TCP
 *  \param		targetIp				the IP address array, which contains the 4 sections of IPv4
 *  \param		port					the port to connect to
 *  \param		timeoutMs				timeout in milliseconds (optional)
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_ConnectIp(NetTransportContext *ctx, SocketType sockType, uint8_t targetIp[4], uint16_t port, uint32_t timeoutMs);

/*! \brief      The callback function setter for new client connections in server mode
 *  \discussion The client handling callback must be set BEFORE starting the server mode
 *  			because the net_StartServerConnection will block the execution until the
 *  			first client comes, then invokes this callback method
 *  \param      callback    the callback function, which should be invoked when a new
 *  						client gets connected
 */
void net_SetHandleClientConnectionCallback(NetHandleClientConnectionCallback callback);

/*! \brief      Platform-dependent server setup (start listening on a specific port
 * 				and wait for incoming connection)
 *  \discussion This function opens the specified ports and is waiting for client
 *  			connection. Once a new client gets connected, the callback function will be
 *  			invoked. Thus, the net_SetHandleClientConnectionCallback function should be
 *  			invoked first to set the callback function! This method will be stopped once
 *  			the callback function returns a negative number.
 *
 *  \param      ctx					    a pointer to the user-defined context
 *  \param		sockType				can be SCOKET_UDP or SOCKET_TCP
 *  \param		port					the port to open
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_StartServerConnection(NetTransportContext *ctx, SocketType sockType, uint16_t port);

/*! \brief      Platform-dependent server stop (stops listening)
 *
 *  \param      ctx     a pointer to the user-defined context
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_StopServerConnection(NetTransportContext *ctx);

/*! \brief      Platform-dependent connection close
 *  \discussion Use the platform-specific tools to create a connection
 *  \param      ctx      a pointer to the user-defined context
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_Disconnect(NetTransportContext *ctx);

/*! \brief      Platform-dependent data sending implementation
 *  \discussion Use the platform-specific tools to send data to the peer
 *  \param      ctx     				a pointer to the user-defined context
 *  \param		buffer					the buffer containing the data to send
 *  \param		bufferSz				the size of the data in the buffer to send
 *  \param		timeoutMs				timeout in milliseconds (optional)
 *
 *  \return     negative result codes for error, otherwise the length of the sent data
 */
int net_Send(NetTransportContext *ctx, const char* buffer, const uint32_t bufferSz, uint32_t timeoutMs);

/*! \brief      Platform-dependent data sending implementation
 *  \discussion Use the platform-specific tools to receive data from the peer
 *  \param      ctx     				a pointer to the user-defined context
 *  \param		buffer					the buffer in which we need to read the data
 *  \param		bufferSz				the size of the data to be read
 *  \param		timeoutMs				timeout in milliseconds (optional)
 *
 *  \return     negative result codes for error, otherwise the length of the received data
 */
int net_Receive(NetTransportContext *ctx, const char* buffer, const uint32_t bufferSz, uint32_t timeoutMs);

/*! \brief      Platform-dependent low level net transport deinitialization
 *  \discussion E.g.: free memory if it's necessary, etc.
 *  \param      netTransportcontext      a pointer to the user-defined context
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_Destroy(NetTransportContext *ctx);

#endif //NET_TRANSPORT_H
