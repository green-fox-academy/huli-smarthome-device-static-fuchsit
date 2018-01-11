#ifndef MQTT_NET_H
#define MQTT_NET_H

#include <stdint.h>
#include "wolfmqtt/mqtt_types.h"

#define MQTT_MAX_BUFFER_SIZE         	512
#define MQTT_DEFAULT_TIMEOUT			5000

int MQTT_Init();

int MQTT_GetNextPacketId();

int MQTT_NetConnectCallback(void *context, const char* host, word16 port, int timeout_ms);

int MQTT_NetWriteCallback(void *context, const byte* buf, int buf_len, int timeout_ms);

int MQTT_NetReadCallback(void *context, byte* buf, int buf_len, int timeout_ms);

int MQTT_NetDisconnectCallback(void *context);

#endif // MQTT_NET_H
