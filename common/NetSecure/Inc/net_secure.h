#ifndef NET_SECURE_H
#define NET_SECURE_H

#if defined(STM32L475xx)
#include "stm32l4xx_hal.h"
#elif defined(STM32F746xx)
#include "stm32f7xx_hal.h"
#endif

typedef struct __NetSecure_InitTypeDef {
	RNG_HandleTypeDef *rngHandle;
} NetSecure_InitTypeDef;

/*! \brief		Initializer for ensuring secure communication. WolfSSL needs the
 * 				random number generator
 *
 * 	\param		rngHandle		reference to the random number generator struct
 */
void net_SecureInit(NetSecure_InitTypeDef *rngHandle);

/*! \brief      The timestamp callback for WolfSSL
 *  \discussion This implementation uses the RTCUtils to get the current timestamp
 *  			from RTC. Should be configured in user_settings.h
 *
 *  \return     the unix timestamp
 */
time_t net_CustomTimestampCallback(time_t x);

/*! \brief      Random generation callback for WolfSSL
 *  \discussion This implementation uses the hardware random generator with HAL library.
 *  			Should be configured in user_settings.h
 *
 *  \return     32 bit random number.
 */
uint32_t net_CustomRandomCallback(void);

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
int net_TLSConnect(void *netTransportContext, SocketType sockType, const char* host, uint16_t port, uint32_t timeoutMs);

/*! \brief      Platform-dependent connection close
 *  \discussion Use the platform-specific tools to create a connection
 *  \param      netTransportContext      a pointer to the user-defined context
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_TLSDisconnect(void *netTransportContext);

/*! \brief      Platform-dependent data sending implementation
 *  \discussion Use the platform-specific tools to send data to the peer
 *  \param      netTransportContext     a pointer to the user-defined context
 *  \param		buffer					the buffer containing the data to send
 *  \param		bufferSz				the size of the data in the buffer to send
 *  \param		timeoutMs				timeout in milliseconds (optional)
 *
 *  \return     negative result codes for error, otherwise the length of the sent data
 */
int net_TLSSend(void *netTransportContext, const char* buffer, const uint32_t bufferSz, uint32_t timeoutMs);

/*! \brief      Platform-dependent data sending implementation
 *  \discussion Use the platform-specific tools to receive data from the peer
 *  \param      netTransportContext     a pointer to the user-defined context
 *  \param		buffer					the buffer in which we need to read the data
 *  \param		bufferSz				the size of the data to be read
 *  \param		timeoutMs				timeout in milliseconds (optional)
 *
 *  \return     negative result codes for error, otherwise the length of the received data
 */
int net_TLSReceive(void *netTransportContext, const char* buffer, const uint32_t bufferSz, uint32_t timeoutMs);

/*! \brief      Platform-dependent low level net transport deinitialization
 *  \discussion E.g.: free memory if it's necessary, etc.
 *  \param      netTransportcontext      a pointer to the user-defined context
 *
 *  \return     result code, where the success is RC_SUCCESS
 */
int net_TLSDestroy(void *netTransportContext);

#endif //NET_SECURE_H
