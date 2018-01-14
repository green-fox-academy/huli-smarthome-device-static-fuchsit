#ifndef GOOGLE_IOT_H
#define GOOGLE_IOT_H

#include <stdint.h>
#include "mqtt_net.h"
#include "net_transport.h"
#include "smarthome_log.h"

/**
 *  @brief      This struct should contain all the network preferences which
 * 				is necessary to connect
 */
typedef struct __GGL_NetworkDef {
	char* mqttHost;
	uint16_t mqttPort;
	NetConnectionContext mqttConnectionContext;
} GGL_NetworkDef;

/**
 * @brief      This struct should contain all the device information
 */
typedef struct __GGL_DeviceDef {
	char* projectId;
	char* region;
	char* deviceRegistry;
	char* deviceId;
} GGL_DeviceDef;

/**
 * @brief		This struct should contain all the network and device
 * 				information and the message callback as well.
 */
typedef struct __GGL_InitDef {
	GGL_NetworkDef network;
	GGL_DeviceDef device;
	MQTT_MesssageArrivedCallback callback;
} GGL_InitDef;

/**
 *  @brief      Initializes the Google IOT library
 *  @discussion Similarly to the hardware peripherals, this method should be
 *  			invoked with all the init data specified in the structure before
 *  			being able to use the library.
 *  @param      config     the connection and device configuration
 */
void GGL_IOT_Init(GGL_InitDef *config);

/**
 *  @brief      Connects and authenticates the device to the google MQTT broker on TLS
 *  @discussion This method is the composite of the WolfMQTT's NetConnect and Connect
 *  			methods. Establishes a session with TLS and authenticates to the MQTT
 *  			broker.
 *
 *  @retval     result code, where the success is 0
 */
int GGL_MQTT_Connect();

/** @brief      Disconnects from the google MQTT broker
 *
 *  @retval     result code, where the success is 0
 */
int GGL_MQTT_Disconnect();

/**
 *  @brief      Subscribes to the specified topic with the Google broker
 *  @discussion Only the last part of the topic name should be specified, the
 *  			function assembles the full-qualified name for the topic before
 *  			subscription
 *
 *  @param		topicName	The name of the topic to subscribe
 *  @param		qos			QoS of the subscription
 *
 *  @retval     result code, where the success is 0
 */
int GGL_MQTT_Subscribe(const char* topicName, MqttQoS qos);

/**
 *  @brief      Waits for incoming message from the topics the client subscribed to
 *  @discussion
 *
 *  @param		timeout		The time amount in millisecons to wait
 *
 *  @retval     result code, where the success is 0
 */
int GGL_MQTT_WaitForMessage(uint32_t timeout);

/**
 *  @brief      Publishes the message to the specified topic.
 *  @discussion Only the last part of the topic name should be specified, the
 *  			function assembles the full-qualified name for the topic before
 *  			subscription
 *
 *  @param		topicName	The name of the topic to publish
 *  @param		message		The message string to publish in the topic
 *
 *  @retval     result code, where the success is 0
 */
int GGL_MQTT_Publish(const char* topicName, const char* message);

/**
 *  @brief      Keep-alive heartbeat sending method
 *  @discussion It's recommended to setup a timer, which calls this method
 *  			regularly when the device is connected in order to keep the
 *  			connection alive with the Google MQTT Broker
 *
 *  @retval     result code, where the success is 0
 */
int GGL_MQTT_Ping();


#endif // GOOGLE_IOT_H
