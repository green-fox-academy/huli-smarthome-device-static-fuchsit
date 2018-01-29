# Google IoT

Google generally has quite strict requirements in order to create secure connection with their APIs. Meeting this requirements in an embedded system requires some work. The motivation behind this library was to simplify the usage of Google IoT Core API and MQTT broker.

The library contains the creation and signal of the JWT, so if JWT should be used somewhere else this could be a good starting point.

# Usage

```c

#include "google_iot.h"

GGL_InitDef gglConfig;

int MQTT_HandleMessageCallback(const char* topic, const char* message) {
    printf("Message arrived in topic: %s\r\nMessage:%s\r\n", topic, message);
    return 0;
}

void main() {
    // initialize the google library
    GGL_DeviceDef device;
    device.deviceId = "test-iot-device-2";
    device.deviceRegistry = "greenfox-device-registry";
    device.projectId = "static-aventurin-fuchsit";
    device.region = "europe-west1";

    GGL_NetworkDef network;
    network.mqttHost = "mqtt.googleapis.com";
    network.mqttPort = 8883;

    gglConfig.callback = MQTT_HandleMessageCallback;
    gglConfig.device = device;
    gglConfig.network = network;
    GGL_IOT_Init(&gglConfig);

    // use the library
    int rc = GGL_MQTT_Connect();
    if (rc != RC_SUCCESS) {
        printf("ERROR: GGL_MQTT_Connect FAILED %d - %s\r\n", rc, MqttClient_ReturnCodeToString(rc));
        return;
    }
    rc = GGL_MQTT_Publish("events/report", "{\"ledState\": \"off\"}");
    if (rc != RC_SUCCESS) {
        printf("ERROR: GGL_MQTT_Publish FAILED %d - %s\r\n", rc, MqttClient_ReturnCodeToString(rc));
        return;
    }
    rc = GGL_MQTT_Subscribe("config", MQTT_QOS_1);
    if (rc != RC_SUCCESS) {
        printf("ERROR: GGL_MQTT_Subscribe FAILED %d - %s\r\n", rc, MqttClient_ReturnCodeToString(rc));
        return;
    }
    while (1) {
        GGL_MQTT_WaitForMessage(10000);
        GGL_MQTT_Ping();
    }

    GGL_MQTT_Disconnect();
}
```