# MQTT

This library is just a helper to initialize WolfMQTT client with the NetSecure library. This library can be used alone or through Google IoT library. When using with Google IoT library, you shouldn't have to initialize separately the MQTT, because it takes care about the initialization.

# Usage

The example below demonstrates how to use this library together with WolfMQTT, without Google IoT.

```c
#include "mqtt_net.h"


extern MqttClient mqttClient;
extern MqttNet mqttNetwork;
MQTT_NetInitTypeDef mqttConfig;

int MQTT_MessageArrivedCallback(const char* topic, const char* message) {
    printf("Message arrived in topic: %s\r\nMessage:%s\r\n", topic, message);
    return 0;
}

void main() {
    mqttConfig.callback = MQTT_MessageArrivedCallback;
    mqttConfig.ctx.id = 0;
    MQTT_Init(&mqttConfig);

    // create network connection
    int rc = MqttClient_NetConnect(&mqttClient, "iot.eclipse.org", 1883, 2000, 0, NULL);
    if (rc != MQTT_CODE_SUCCESS) {
        printf("ERROR MqttClient_NetConnect rc=%d\r\n", rc);
        return;
    }

    // create MQTT connection
    MqttConnect connect;
    connect.client_id = "test cli ID";
    connect.clean_session = 1;
    connect.keep_alive_sec = 30;
    rc = MqttClient_Connect(&mqttClient, &connect);
    if (rc != MQTT_CODE_SUCCESS || connect.ack.return_code != 0) {
        printf("ERROR MqttClient_Connect rc=%d, ack rc=%d\r\n", rc, connect.ack.return_code);
        return;
    }

    // publish a message to a topic
    MqttPublish pub;
    XMEMSET(&pub, 0, sizeof(MqttPublish));
	pub.buffer = (byte*)"Test message";
	pub.buffer_len = strlen((char*)pub.buffer);
    pub.topic_name = "ptopic";
    pub.qos = MQTT_QOS_0;

    rc = MqttClient_Publish(&mqttClient, &pub);
    if (rc != MQTT_CODE_SUCCESS || connect.ack.return_code != 0) {
        printf("ERROR MqttClient_Publish rc=%d\r\n", rc);
        return;
    }

    // subscribe to a topic
    MqttTopic top = { "stopic", MQTT_QOS_1 };

    MqttSubscribe sub;
    sub.topic_count = 1;
    sub.topics = &top;
    rc = MqttClient_Subscribe(&mqttClient, &sub);
    if (rc != MQTT_CODE_SUCCESS || connect.ack.return_code != 0) {
        printf("ERROR MqttClient_Subscribe rc=%d\r\n", rc);
        return;
    }

    // listening for messages and sending regular pings to keep the connection alive
    while (1) {
        MqttClient_WaitMessage(&mqttClient, 10000);
        MqttClient_Ping(&mqttClient);
    }
    

}
```