#include "main.h"
#include "net_transport.h"
#include "smarthome_log.h"
#include "mqtt_net.h"
#include "wolfmqtt/mqtt_client.h"

#define MQTT_ENTER(fnc)					SHOME_LogEnter("mqtt", fnc)
#define MQTT_MSG(fmt, ...)				SHOME_LogMsg("mqtt", fmt, ##__VA_ARGS__)
#define MQTT_EXIT(fnc, rc, fail)		SHOME_LogExit("mqtt", fnc, rc, fail)

NetTransportContext mqttNetTransportContext = { 0 };

static uint8_t mqttTxBuf[MQTT_MAX_BUFFER_SIZE];
static uint8_t mqttRxBuf[MQTT_MAX_BUFFER_SIZE];
static word16 mqttPacketId = 0;

MqttClient mqttClient;
MqttNet mqttNetwork;
MQTT_NetInitTypeDef *mqtt_config;

int MQTT_MessageArrivedCallback(struct _MqttClient *client,
		MqttMessage *message, byte msg_new, byte msg_done) {
	MQTT_ENTER("MQTT_MessageArrivedCallback");

	byte topicBuf[MQTT_MAX_BUFFER_SIZE + 1];
	byte messageBuf[MQTT_MAX_BUFFER_SIZE + 1];

	if (msg_new) {
		XMEMCPY(topicBuf, message->topic_name, message->topic_name_len);
		topicBuf[message->topic_name_len] = '\0';
		MQTT_MSG("Got MQTT message to topic: %s\r\n", topicBuf);
	}

	if (!msg_done) {
		MQTT_MSG("ERROR: MQTT message exceeds the maximum buffer size, which is not supported in this library.\r\n");
		MQTT_EXIT("MQTT_MessageArrivedCallback", -1, 1);
		return -1;
	}

	XMEMCPY(messageBuf, message->buffer, message->buffer_len);
	messageBuf[message->buffer_len] = '\0';
	int rc = mqtt_config->callback((char*)topicBuf, (char*)messageBuf);
	if (rc != 0) {
		MQTT_EXIT("MQTT_MessageArrivedCallback", rc, 1);
		return rc;

	}
	MQTT_EXIT("MQTT_MessageArrivedCallback", 0, 0);
	return 0;
}

int MQTT_Init(MQTT_NetInitTypeDef *config) {
	mqtt_config = config;
	mqttNetwork.connect = MQTT_NetConnectCallback;
	mqttNetwork.disconnect = MQTT_NetDisconnectCallback;
	mqttNetwork.read = MQTT_NetReadCallback;
	mqttNetwork.write = MQTT_NetWriteCallback;
	mqttNetwork.context = &config->ctx;

	return MqttClient_Init(&mqttClient, &mqttNetwork,
			MQTT_MessageArrivedCallback, (byte*)&mqttTxBuf, MQTT_MAX_BUFFER_SIZE,
			(byte*)&mqttRxBuf, MQTT_MAX_BUFFER_SIZE, MQTT_DEFAULT_TIMEOUT);
}

int MQTT_GetNextPacketId() {
	return mqttPacketId++;
}

int MQTT_NetConnectCallback(void *context, const char* host, word16 port, int timeout_ms) {
	return net_Connect(context, SOCKET_TCP, host, port, timeout_ms);
}
int MQTT_NetWriteCallback(void *context, const byte* buf, int buf_len, int timeout_ms) {
	return net_Send(context, (const char*)buf, buf_len, timeout_ms);
}
int MQTT_NetReadCallback(void *context, byte* buf, int buf_len, int timeout_ms) {
	return net_Receive(context, (const char*)buf, buf_len, timeout_ms);
}
int MQTT_NetDisconnectCallback(void *context) {
	return net_Disconnect(context);
}
