#include "main.h"
#include "net_transport.h"
#include "mqtt_net.h"
#include "wolfmqtt/mqtt_client.h"

NetTransportContext mqttNetTransportContext = { 0 };

static uint8_t mqttTxBuf[MQTT_MAX_BUFFER_SIZE];
static uint8_t mqttRxBuf[MQTT_MAX_BUFFER_SIZE];
static word16 mqttPacketId = 0;

MqttClient mqttClient;
MqttNet mqttNetwork;

int MQTT_Init(MqttMsgCb messageCallbackFunction) {

	mqttNetwork.connect = MQTT_NetConnectCallback;
	mqttNetwork.disconnect = MQTT_NetDisconnectCallback;
	mqttNetwork.read = MQTT_NetReadCallback;
	mqttNetwork.write = MQTT_NetWriteCallback;
	mqttNetwork.context = &mqttNetTransportContext;

	return MqttClient_Init(&mqttClient, &mqttNetwork,
			messageCallbackFunction, (byte*)&mqttTxBuf, MQTT_MAX_BUFFER_SIZE,
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
