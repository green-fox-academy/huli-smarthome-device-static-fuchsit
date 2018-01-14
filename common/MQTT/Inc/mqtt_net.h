#ifndef MQTT_NET_H
#define MQTT_NET_H

#include <stdint.h>
#include "wolfmqtt/mqtt_types.h"

#define MQTT_MAX_BUFFER_SIZE         	512
#define MQTT_DEFAULT_TIMEOUT			5000

/**
 * @brief		Initializes the WolfMQTT library
 *
 * @retval		WOLFMQTT_SUCCESS (0) or error code
 */
int MQTT_Init();

/**
 * @brief		Generates a packet ID for QoS > 0. This function assumes that
 * 				the application is single threaded.
 *
 * @retval		the next packet id
 */
int MQTT_GetNextPacketId();

/**
 * @brief		create network connection callback
 *
 * @retval		WOLFMQTT_SUCCESS (0) or error code
 */
int MQTT_NetConnectCallback(void *context, const char* host, word16 port, int timeout_ms);

/**
 * @brief		send data on network callback
 *
 * @retval		negative error code on failure, the number of sent bytes otherwise
 */
int MQTT_NetWriteCallback(void *context, const byte* buf, int buf_len, int timeout_ms);

/**
 * @brief		read data from network callback
 *
 * @retval		negative error code on failure, the number of read bytes otherwise
 */
int MQTT_NetReadCallback(void *context, byte* buf, int buf_len, int timeout_ms);


/**
 * @brief		close network connection callback
 *
 * @retval		WOLFMQTT_SUCCESS (0) or error code
 */
int MQTT_NetDisconnectCallback(void *context);

#endif // MQTT_NET_H