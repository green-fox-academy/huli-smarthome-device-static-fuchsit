#ifndef GOOGLE_IOT_H
#define GOOGLE_IOT_H

#include <stdint.h>
#include "mqtt_net.h"
#include "net_transport.h"
#include "smarthome_log.h"

typedef struct GGL_Device {
    char *id;
    char *numId;
    char *lastHeartbeatTime;
    char *deviceNature;
    char *publicKey;
} GGL_Device;

typedef struct GGL_DeviceList {
    GGL_Device *deviceList;
    uint16_t deviceCount;
} GGL_DeviceList;

/**
 *  @brief      This struct should contain all the network preferences which
 * 				is necessary to connect
 */
typedef struct __GGL_NetworkDef {
	char* mqttHost;
	uint16_t mqttPort;
	char* restApiBasePath;
	char* oAuthApiBasePath;
	char* saClientEmail;
	char* saScope;
	char* saPrivateKey;
	uint16_t saPrivateKeySize;
	char* mqttPrivateKey;
	uint16_t mqttPrivateKeySize;
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
 * @brief		Holds the access token contents
 */
typedef struct __GGL_AccessTokenDef {
	char* accessToken;
	char* tokenType;
	uint16_t expiresIn;
} GGL_AccessTokenDef;

/**
 *  @brief      Initializes the Google IOT library
 *  @discussion Similarly to the hardware peripherals, this method should be
 *  			invoked with all the init data specified in the structure before
 *  			being able to use the library.
 *  @param      config     the connection and device configuration
 */
void GGL_IOT_Init(GGL_InitDef *config);

/**
 * @brief       Lists the devices of the configured device registry
 * @discussion  The devices will be returned in the parameter, which is pointer to
 *              a pointer. The contents of this is dynamically allocated, so after you
 *              don't need the results anymore, you need to explicitly free that!
 *
 *              WARNING: it's important that the credentials won't be returned (publicKey),
 *              it will be NULL in the result (lowering memory footprint).
 *
 * @param       A pointer to a GGL_DeviceListPointer in which the devices will be loaded
 * @retval      result code, where the success is 0
 */
int GGL_IOT_ListDevices(GGL_DeviceList **deviceListPPtr);

/**
 * @brief       Deletes a device from the configured device registry
 * @discussion  The Google IoT Core REST API expects the numeric ID of the device
 *
 * @param       deviceNumId the numeric ID of the device
 * @retval      result code, where the success is 0
 */
int GGL_IOT_DeleteDevice(char *deviceNumId);

/**
 * @brief       Creates the specified device in the configured device registry
 * @discussion  You should specify only the ID, the deviceKind and the publicKey of
 *              the device, and this function will register the device into the cloud.
 *
 * @param       device      a pointer to the device to create
 * @retval      result code, where the success is 0
 */
int GGL_IOT_CreateDevice(GGL_Device *device);

/**
 * @brief       Updates the configuration of the specified device to the specified
 *              config
 *
 * @param       deviceId        the string ID of the device (not the numId)
 * @param       deviceConfig    the configuration for the device
 *
 * @retval      result code, where the success is 0
 */
int GGL_IOT_UpdateDeviceConfig(char *deviceId, char *deviceConfig);

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
