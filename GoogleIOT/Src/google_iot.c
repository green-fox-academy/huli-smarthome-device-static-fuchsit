#include "google_iot.h"
#include "smarthome_log.h"
#include "mqtt_net.h"
#include "wolfmqtt/mqtt_client.h"
#include "wolfssl/ssl.h"
#include "wolfssl/wolfcrypt/coding.h"
#include "wolfssl/wolfcrypt/signature.h"
#include "wolfssl/wolfcrypt/rsa.h"
#include "rtc_utils.h"
#include "http_client.h"
#include "net_settings.h"
#include "json_utils.h"

#define UINT32_STR_LEN              10
#define CRED_INVALIDITY_DIFF        120

#define GGL_ENTER(fnc)				SHOME_LogEnter("ggl", fnc)
#define GGL_MSG(fmt, ...)			SHOME_LogMsg("ggl", fmt, ##__VA_ARGS__)
#define GGL_EXIT(fnc, rc, fail)		SHOME_LogExit("ggl", fnc, rc, fail)

typedef struct GGL_Credential {
    char *token;
    uint32_t validUntilTimestamp;
} GGL_Credential;

typedef struct GGL_Credentials {
    GGL_Credential mqttJwt;
    GGL_Credential saJwt;
    GGL_Credential saAccessToken;
} GGL_Credentials;

static const char GGL_REST_UPDATE_DEVICE_CONFIG_URL_TEMPLATE[] = "%s/projects/%s/locations/%s/registries/%s/devices/%s:modifyCloudToDeviceConfig";
static const char GGL_REST_UPDATE_DEVICE_CONFIG_BODY_TEMPLATE[] = "{\"versionToUpdate\":0,\"binaryData\":\"%s\"}";
static const char GGL_REST_DELETE_DEVICE_URL_TEMPLATE[] = "%s/projects/%s/locations/%s/registries/%s/devices/%s";
static const char GGL_REST_DEVICES_URL_TEMPLATE[] = "%s/projects/%s/locations/%s/registries/%s/devices";
static const char GGL_REST_CREATE_DEVICE_BODY_TEMPLATE[] = "{\"credentials\":[{\"publicKey\":{\"key\":\"%s\",\"format\":\"RSA_PEM\"}}],\"id\":\"%s\",\"metadata\":{\"deviceNature\":\"%s\"}}";

static const char GGL_JWT_HEADER[] = "{\"alg\":\"RS256\",\"typ\":\"JWT\"}";
static const char GGL_JWT_MQTT_BODY_TEMPLATE[] = "{\"iat\":%lu,\"exp\":%lu,\"aud\":\"%s\"}";
static const char GGL_JWT_SA_BODY_TEMPLATE[] = "{\"iss\":\"%s\",\"scope\":\"%s\",\"aud\":\"https://www.googleapis.com/oauth2/v4/token\",\"exp\":%lu,\"iat\":%lu}";

static GGL_InitDef *GGL_Config;
static GGL_Credentials GGL_Creds;

extern MqttClient mqttClient;

MQTT_NetInitTypeDef mqttNetInit;

int GGL_JWT_Create(const char* header, const char* body, const char *rsaPvtKey, size_t rsaPvtKeySize, GGL_Credential *credential);
int GGL_JWT_RS256_Sign(const char *jwtToSign, const char *rsaPvtKey, size_t rsaPvtKeySize, GGL_Credential *credential);
int GGL_JWT_MQTT_Create();
int GGL_JWT_SA_Create();
int GGL_SA_CreateAccessToken();
int GGL_SizeOfInB64(size_t dataSize);

void GGL_IOT_Init(GGL_InitDef *config) {
    GGL_ENTER("GGL_IOT_Init");
    GGL_Config = config;
    GGL_Creds.mqttJwt.token = NULL;
    GGL_Creds.mqttJwt.validUntilTimestamp = 0;
    GGL_Creds.saJwt.token = NULL;
    GGL_Creds.saJwt.validUntilTimestamp = 0;
    GGL_Creds.saAccessToken.token = NULL;
    GGL_Creds.saAccessToken.validUntilTimestamp = 0;
    GGL_EXIT("GGL_IOT_Init", 0, 0);
}

int GGL_IOT_ListDevices(GGL_DeviceList **deviceListPPtr) {
    GGL_ENTER("GGL_IOT_ListDevices");

    int rc = GGL_SA_CreateAccessToken();
    if (rc != 0) {
        GGL_MSG("ERROR: GGL_SA_CreateAccessToken %d\r\n", rc);
        GGL_EXIT("GGL_IOT_ListDevices", rc, 1);
        return rc;
    }

    char requestUrl[256];
    sprintf(requestUrl, GGL_REST_DEVICES_URL_TEMPLATE, GGL_Config->network.restApiBasePath, GGL_Config->device.projectId, GGL_Config->device.region, GGL_Config->device.deviceRegistry);

    HTTP_Request *listDeviceReq = NULL;
    rc = http_CreateRequest(requestUrl, HTTP_GET, HTTP_CONTENT_TYPE_FORM, &listDeviceReq);
    if (rc != 0) {
        GGL_MSG("ERROR: http_CreateRequest %d\r\n", rc);
        GGL_EXIT("GGL_IOT_ListDevices", rc, 1);
        return rc;
    }

    char authHeaderValue[strlen(GGL_Creds.saAccessToken.token) + 9];
    sprintf(authHeaderValue, "Bearer %s", GGL_Creds.saAccessToken.token);
    http_AddHeader(listDeviceReq, "Authorization", authHeaderValue);
    http_AddBodyParam(listDeviceReq, "fieldMask", "metadata,lastHeartbeatTime,state");

    NetTransportContext ctx;
    HTTP_Response *listDeviceResponse;

    rc = http_Execute(&ctx, listDeviceReq, &listDeviceResponse);

    if (rc != 0) {
        GGL_MSG("ERROR: http_Execute %d\r\n", rc);
        GGL_EXIT("GGL_IOT_ListDevices", rc, 1);
        return rc;
    }

    if (listDeviceResponse->statusCode != 200) {
        GGL_MSG("Request body: %s\r\n", listDeviceReq->body);
        GGL_MSG("ERROR: HTTP response returned status code: %d\r\n", listDeviceResponse->statusCode);
        GGL_MSG("ERROR: HTTP response content is: %s\r\n", listDeviceResponse->body);
        GGL_EXIT("GGL_IOT_ListDevices", -200, 1);
        return -200;
    }

    jsmn_parser p;
    jsmntok_t t[128];
    jsmn_init(&p);
    rc = jsmn_parse(&p, listDeviceResponse->body, strlen(listDeviceResponse->body), t, sizeof(t) / sizeof(t[0]));
    if (rc < 0) {
        GGL_MSG("ERROR: Could not parse response body (rc=%d, body=%s)\r\n", rc, listDeviceResponse->body);
        GGL_EXIT("GGL_IOT_ListDevices", -201, 1);
        return -201;
    }

    if (t[0].type != JSMN_OBJECT || t[1].type != JSMN_STRING || 0 != json_IsStrEqual(listDeviceResponse->body, &t[1], "devices")) {
        GGL_MSG("ERROR: Could not parse response body (rc=%d, body=%s)\r\n", rc, listDeviceResponse->body);
        GGL_EXIT("GGL_IOT_ListDevices", -201, 1);
        return -201;
    }

    GGL_DeviceList *response = *deviceListPPtr = malloc(sizeof(GGL_DeviceList));
    if (response == NULL) {
        GGL_MSG("ERROR: Out of memory\r\n");
        GGL_EXIT("GGL_IOT_ListDevices", -202, 1);
        return -202;
    }

    response->deviceList = NULL;
    response->deviceCount = 0;
    if (t[2].type != JSMN_ARRAY || t[2].size == 0) {
        GGL_MSG("WARNING: no device was returned, the list is empty\r\n");
        GGL_EXIT("GGL_IOT_ListDevices", 0, 0);
        return 0;
    }

    response->deviceCount = t[2].size;
    response->deviceList = malloc(response->deviceCount * sizeof(GGL_Device));
    if (response->deviceList == NULL) {
        GGL_MSG("ERROR: Out of memory\r\n");
        GGL_EXIT("GGL_IOT_ListDevices", -202, 1);
        return -202;
    }

    uint16_t startTokenIdx = 3;
    for (uint16_t i = 0; i < t[2].size; i++) {
        uint16_t len = t[startTokenIdx].end - t[startTokenIdx].start;
        char aDeviceJson[len + 1];
        memcpy(aDeviceJson, listDeviceResponse->body + t[startTokenIdx].start, len);
        aDeviceJson[len] = '\0';
        jsmn_parser dp;
        jsmntok_t dt[128];
        jsmn_init(&dp);
        rc = jsmn_parse(&dp, aDeviceJson, len, dt, sizeof(dt) / sizeof(dt[0]));
        char *props[] = { "id", "numId", "lastHeartbeatTime", "deviceNature" };
        char **values = json_GetValuesForKeys(aDeviceJson, props, 4, dt, rc);

        GGL_Device *aDevice = &response->deviceList[i];
        aDevice->id = values[0];
        aDevice->numId = values[1];
        aDevice->lastHeartbeatTime = values[2] == NULL ? NULL : values[3];
        aDevice->deviceNature = values[3] == NULL ? NULL : values[3];
        aDevice->publicKey = NULL;

        free(values);

        // now find the next device, which is the first JSMN_OBJECT type after the current's end
        uint16_t nextStartTokenIdx = startTokenIdx;
        while (t[nextStartTokenIdx].type != JSMN_OBJECT || t[nextStartTokenIdx].start < t[startTokenIdx].end) {
            nextStartTokenIdx++;
        }
        startTokenIdx = nextStartTokenIdx;
    }

    http_Cleanup(listDeviceReq, listDeviceResponse);

    GGL_EXIT("GGL_IOT_ListDevices", 0, 0);
    return 0;
}

int GGL_IOT_CreateDevice(GGL_Device *device) {
    GGL_ENTER("GGL_IOT_CreateDevice");

    if (device->deviceNature == NULL || device->id == NULL || device->publicKey == NULL) {
        GGL_MSG("ERROR: invalid arguments: deviceNature, id, publicKey are required!\r\n");
        GGL_EXIT("GGL_IOT_CreateDevice", -5, 1);
        return -5;
    }

    int rc = GGL_SA_CreateAccessToken();
    if (rc != 0) {
        GGL_MSG("ERROR: GGL_SA_CreateAccessToken %d\r\n", rc);
        GGL_EXIT("GGL_IOT_CreateDevice", rc, 1);
        return rc;
    }

    char requestUrl[256];
    sprintf(requestUrl, GGL_REST_DEVICES_URL_TEMPLATE, GGL_Config->network.restApiBasePath, GGL_Config->device.projectId, GGL_Config->device.region, GGL_Config->device.deviceRegistry);

    HTTP_Request *createDeviceReq = NULL;
    rc = http_CreateRequest(requestUrl, HTTP_POST, HTTP_CONTENT_TYPE_JSON, &createDeviceReq);
    if (rc != 0) {
        GGL_MSG("ERROR: http_CreateRequest %d\r\n", rc);
        GGL_EXIT("GGL_IOT_CreateDevice", rc, 1);
        return rc;
    }

    char authHeaderValue[strlen(GGL_Creds.saAccessToken.token) + 9];
    sprintf(authHeaderValue, "Bearer %s", GGL_Creds.saAccessToken.token);
    http_AddHeader(createDeviceReq, "Authorization", authHeaderValue);

    char createDeviceBody[1024];
    sprintf(createDeviceBody, GGL_REST_CREATE_DEVICE_BODY_TEMPLATE, device->publicKey, device->id, device->deviceNature);
    http_SetBody(createDeviceReq, createDeviceBody);

    NetTransportContext ctx;
    HTTP_Response *createDeviceResp;

    rc = http_Execute(&ctx, createDeviceReq, &createDeviceResp);

    if (rc != 0) {
        GGL_MSG("ERROR: http_Execute %d\r\n", rc);
        GGL_EXIT("GGL_IOT_CreateDevice", rc, 1);
        return rc;
    }

    if (createDeviceResp->statusCode != 200) {
        GGL_MSG("ERROR: HTTP response returned status code: %d\r\n", createDeviceResp->statusCode);
        GGL_MSG("ERROR: HTTP response content is: %s\r\n", createDeviceResp->body);
        GGL_EXIT("GGL_IOT_CreateDevice", -200, 1);
        return -200;
    }

    jsmn_parser p;
    jsmntok_t t[128];
    jsmn_init(&p);
    rc = jsmn_parse(&p, createDeviceResp->body, strlen(createDeviceResp->body), t, sizeof(t) / sizeof(t[0]));
    if (rc < 0) {
        GGL_MSG("ERROR: Could not parse response body (rc=%d, body=%s)\r\n", rc, createDeviceResp->body);
        GGL_EXIT("GGL_IOT_CreateDevice", -201, 1);
        return -201;
    }

    if (t[0].type != JSMN_OBJECT) {
        GGL_MSG("ERROR: Could not parse response body (rc=%d, body=%s)\r\n", rc, createDeviceResp->body);
        GGL_EXIT("GGL_IOT_CreateDevice", -201, 1);
        return -201;
    }

    char *numIdKey[] = { "numId" };
    char **numIdValue = json_GetValuesForKeys(createDeviceResp->body, numIdKey, 1, t, rc);
    device->numId = numIdValue[0];
    free(numIdValue);

    http_Cleanup(createDeviceReq, createDeviceResp);
    GGL_EXIT("GGL_IOT_CreateDevice", 0, 0);
    return 0;
}

int GGL_IOT_DeleteDevice(char *deviceNumId) {
    GGL_ENTER("GGL_IOT_DeleteDevice");

    if (deviceNumId == NULL) {
        GGL_MSG("ERROR: invalid arguments: deviceNumId is required!\r\n");
        GGL_EXIT("GGL_IOT_DeleteDevice", -5, 1);
        return -5;
    }

    int rc = GGL_SA_CreateAccessToken();
    if (rc != 0) {
        GGL_MSG("ERROR: GGL_SA_CreateAccessToken %d\r\n", rc);
        GGL_EXIT("GGL_IOT_DeleteDevice", rc, 1);
        return rc;
    }

    char requestUrl[256];
    sprintf(requestUrl, GGL_REST_DELETE_DEVICE_URL_TEMPLATE, GGL_Config->network.restApiBasePath, GGL_Config->device.projectId, GGL_Config->device.region, GGL_Config->device.deviceRegistry, deviceNumId);

    HTTP_Request *deleteDeviceReq = NULL;
    rc = http_CreateRequest(requestUrl, HTTP_DELETE, HTTP_CONTENT_TYPE_JSON, &deleteDeviceReq);
    if (rc != 0) {
        GGL_MSG("ERROR: http_CreateRequest %d\r\n", rc);
        GGL_EXIT("GGL_IOT_DeleteDevice", rc, 1);
        return rc;
    }

    char authHeaderValue[strlen(GGL_Creds.saAccessToken.token) + 9];
    sprintf(authHeaderValue, "Bearer %s", GGL_Creds.saAccessToken.token);
    http_AddHeader(deleteDeviceReq, "Authorization", authHeaderValue);

    NetTransportContext ctx;
    HTTP_Response *deleteDeviceResp;

    rc = http_Execute(&ctx, deleteDeviceReq, &deleteDeviceResp);

    if (rc != 0) {
        GGL_MSG("ERROR: http_Execute %d\r\n", rc);
        GGL_EXIT("GGL_IOT_DeleteDevice", rc, 1);
        return rc;
    }

    if (deleteDeviceResp->statusCode != 200) {
        GGL_MSG("ERROR: HTTP response returned status code: %d\r\n", deleteDeviceResp->statusCode);
        GGL_EXIT("GGL_IOT_DeleteDevice", -200, 1);
        return -200;
    }

    http_Cleanup(deleteDeviceReq, deleteDeviceResp);
    GGL_EXIT("GGL_IOT_DeleteDevice", 0, 0);
    return 0;
}

int GGL_IOT_UpdateDeviceConfig(char *deviceId, char *deviceConfig) {
    GGL_ENTER("GGL_IOT_UpdateDeviceConfig");

    if (deviceId == NULL || deviceConfig == NULL) {
        GGL_MSG("ERROR: invalid arguments: deviceId and deviceConfig are required!\r\n");
        GGL_EXIT("GGL_IOT_UpdateDeviceConfig", -5, 1);
        return -5;
    }

    int rc = GGL_SA_CreateAccessToken();
    if (rc != 0) {
        GGL_MSG("ERROR: GGL_SA_CreateAccessToken %d\r\n", rc);
        GGL_EXIT("GGL_IOT_UpdateDeviceConfig", rc, 1);
        return rc;
    }

    char requestUrl[256];
    sprintf(requestUrl, GGL_REST_UPDATE_DEVICE_CONFIG_URL_TEMPLATE, GGL_Config->network.restApiBasePath, GGL_Config->device.projectId, GGL_Config->device.region, GGL_Config->device.deviceRegistry, deviceId);
    HTTP_Request *updateRequest = NULL;
    rc = http_CreateRequest(requestUrl, HTTP_POST, HTTP_CONTENT_TYPE_JSON, &updateRequest);
    if (rc != 0) {
        GGL_MSG("ERROR: http_CreateRequest %d\r\n", rc);
        GGL_EXIT("GGL_IOT_UpdateDeviceConfig", rc, 1);
        return rc;
    }

    char authHeaderValue[strlen(GGL_Creds.saAccessToken.token) + 9];
    sprintf(authHeaderValue, "Bearer %s", GGL_Creds.saAccessToken.token);
    http_AddHeader(updateRequest, "Authorization", authHeaderValue);


    // create the body
    word32 confB64Len = GGL_SizeOfInB64(strlen(deviceConfig));
    char confB64[confB64Len + 1];
    rc = Base64_Encode_NoNl(deviceConfig, strlen(deviceConfig), (byte*)confB64, &confB64Len);
    if (rc != 0) {
        GGL_MSG("ERROR: Base64_Encode_NoNl - could not encode configuration%d\r\n", rc);
        GGL_EXIT("GGL_IOT_UpdateDeviceConfig", rc, 1);
        return rc;
    }
    confB64[confB64Len] = '\0';

    uint16_t bodyLen = strlen(GGL_REST_UPDATE_DEVICE_CONFIG_BODY_TEMPLATE) - 2 + confB64Len;
    char body[bodyLen + 1];
    sprintf(body, GGL_REST_UPDATE_DEVICE_CONFIG_BODY_TEMPLATE, confB64);
    http_SetBody(updateRequest, body);

    NetTransportContext ctx;
    HTTP_Response *updateResp;
    rc = http_Execute(&ctx, updateRequest, &updateResp);
    if (rc != 0) {
        GGL_MSG("ERROR: http_Execute %d\r\n", rc);
        GGL_EXIT("GGL_IOT_UpdateDeviceConfig", rc, 1);
        return rc;
    }

    if (updateResp->statusCode != 200) {
        GGL_MSG("ERROR: HTTP response returned status code: %d\r\n", updateResp->statusCode);
        GGL_EXIT("GGL_IOT_UpdateDeviceConfig", -200, 1);
        return -200;
    }

    http_Cleanup(updateRequest, updateResp);
    GGL_EXIT("GGL_IOT_UpdateDeviceConfig", 0, 0);
    return 0;
}

int GGL_MQTT_WaitForMessage(uint32_t timeout) {
    return MqttClient_WaitMessage(&mqttClient, timeout);
}

int GGL_MQTT_Connect() {
    GGL_ENTER("GGL_MQTT_Connect");

    mqttNetInit.callback = GGL_Config->callback;
    mqttNetInit.ctx = GGL_Config->network.mqttConnectionContext;

    int rc = MQTT_Init(&mqttNetInit);

    if (rc != MQTT_CODE_SUCCESS) {
        GGL_MSG("ERROR: unsuccessful MQTT_Init %d=%s\r\n", rc, MqttClient_ReturnCodeToString(rc));
        GGL_EXIT("GGL_MQTT_Connect", rc, 1);
        return rc;
    }

    rc = MqttClient_NetConnect(&mqttClient, GGL_Config->network.mqttHost, GGL_Config->network.mqttPort, MQTT_DEFAULT_TIMEOUT, 1, NULL);
    if (rc != MQTT_CODE_SUCCESS) {
        GGL_MSG("ERROR: unsuccessful MqttClient_NetConnect %d=%s\r\n", rc, MqttClient_ReturnCodeToString(rc));
        GGL_EXIT("GGL_MQTT_Connect", rc, 1);
        return rc;
    }

    MqttConnect conn;
    XMEMSET(&conn, 0, sizeof(MqttConnect));

    char clientIdBuf[256];
    sprintf(clientIdBuf, "projects/%s/locations/%s/registries/%s/devices/%s", GGL_Config->device.projectId, GGL_Config->device.region, GGL_Config->device.deviceRegistry, GGL_Config->device.deviceId);

    conn.clean_session = 1;
    conn.keep_alive_sec = 60;
    conn.client_id = clientIdBuf;
    conn.enable_lwt = 0;
    rc = GGL_JWT_MQTT_Create();
    if (rc != 0) {
        GGL_MSG("ERROR: couldn't create JWT password %rc\r\n", rc);
        GGL_EXIT("GGL_MQTT_Connect", rc, 1);
        return rc;
    }

    conn.username = "unused";
    conn.password = GGL_Creds.mqttJwt.token;

    rc = MqttClient_Connect(&mqttClient, &conn);
    if (rc != 0) {
        GGL_MSG("ERROR: unsuccessful MqttClient_Connect %d=%s\r\n", rc, MqttClient_ReturnCodeToString(rc));
        GGL_EXIT("GGL_MQTT_Connect", rc, 1);
        return rc;
    }
    if (conn.ack.return_code != 0) {
        GGL_MSG("ERROR: unsuccessful MqttClient_Connect, ack RC: %d\r\n", conn.ack.return_code);
        GGL_EXIT("GGL_MQTT_Connect", -3, 1);
        return -3;
    }
    GGL_EXIT("GGL_MQTT_Connect", 0, 0);
    return 0;
}

int GGL_MQTT_Disconnect() {
    GGL_ENTER("GGL_MQTT_Disconnect");
    int rc = MqttClient_Disconnect(&mqttClient);
    if (rc != 0) {
        GGL_EXIT("GGL_MQTT_Disconnect", rc, 1);
        return rc;
    }
    GGL_EXIT("GGL_MQTT_Disconnect", 0, 0);
    return 0;

}

void GGL_MQTT_CreateTopicName(const char* topicName, const char* topicNameBuf) {
    sprintf((char*) topicNameBuf, "/devices/%s/%s", GGL_Config->device.deviceId, topicName);
}

int GGL_MQTT_Subscribe(const char* topicName, MqttQoS qos) {
    GGL_ENTER("GGL_MQTT_Subscribe");
    MqttSubscribe sub;
    XMEMSET(&sub, 0, sizeof(MqttSubscribe));

    char topicNameBuf[256];
    GGL_MQTT_CreateTopicName(topicName, topicNameBuf);

    MqttTopic top[] = { { topicNameBuf, qos } };
    sub.topics = top;
    sub.topic_count = 1;

    int rc = MqttClient_Subscribe(&mqttClient, &sub);
    if (rc != 0) {
        GGL_EXIT("GGL_MQTT_Subscribe", rc, 1);
        return rc;
    }
    GGL_EXIT("GGL_MQTT_Subscribe", 0, 0);
    return 0;
}

int GGL_MQTT_Publish(const char* topicName, const char* message) {
    GGL_ENTER("GGL_MQTT_Publish");

    char topicNameBuf[256];
    GGL_MQTT_CreateTopicName(topicName, topicNameBuf);

    MqttPublish publish;
    XMEMSET(&publish, 0, sizeof(MqttPublish));
    publish.retain = 0;
    publish.qos = MQTT_QOS_0;
    publish.duplicate = 0;
    publish.topic_name = topicNameBuf;
    publish.packet_id = MQTT_GetNextPacketId();
    publish.buffer = (byte*) message;
    publish.total_len = (word16) XSTRLEN(message);
    int rc = MqttClient_Publish(&mqttClient, &publish);
    if (rc != 0) {
        GGL_EXIT("GGL_MQTT_Publish", rc, 1);
        return rc;
    }

    GGL_EXIT("GGL_MQTT_Publish", 0, 0);
    return 0;
}

int GGL_MQTT_Ping() {
    return MqttClient_Ping(&mqttClient);
}

int GGL_SizeOfInB64(size_t dataSize) {
    return 4 * ((dataSize + 2) / 3);
}

int GGL_JWT_RS256_Sign(const char *jwtToSign, const char *rsaPvtKey, size_t rsaPvtKeySize, GGL_Credential *credential) {
    GGL_ENTER("GGL_RS256_Sign");
    uint8_t sigType = WC_SIGNATURE_TYPE_RSA_W_ENC;
    uint8_t hashType = WC_HASH_TYPE_SHA256;

    WC_RNG rng;
    RsaKey privateKey;
    RsaKey publicKey;
    int rc;

    if ((rc = wc_InitRng(&rng)) != 0) {
        GGL_MSG("ERROR: wc_InitRng %d\r\n", rc);
        GGL_EXIT("GGL_RS256_Sign", rc, 1);
        return rc;
    }
    if ((rc = wc_InitRsaKey(&privateKey, NULL)) != 0) {
        GGL_MSG("ERROR: wc_InitRsaKey (private) %d\r\n", rc);
        GGL_EXIT("GGL_RS256_Sign", rc, 1);
        return rc;
    }
    if ((rc = wc_InitRsaKey(&publicKey, NULL)) != 0) {
        GGL_MSG("ERROR: wc_InitRsaKey (public) %d\r\n", rc);
        GGL_EXIT("GGL_RS256_Sign", rc, 1);
        return rc;
    }

    word32 idx = 0;
    byte privateKeyDer[rsaPvtKeySize];
    XMEMCPY(privateKeyDer, rsaPvtKey, rsaPvtKeySize);
    word32 privateKeyDerSize = rsaPvtKeySize;
    if ((rc = wc_RsaPrivateKeyDecode(privateKeyDer, &idx, &privateKey, privateKeyDerSize)) != 0) {
        GGL_MSG("ERROR: wc_RsaPrivateKeyDecode %d\r\n", rc);
        GGL_EXIT("GGL_RS256_Sign", rc, 1);
        return rc;
    }

    int sigLen = wc_SignatureGetSize(sigType, &privateKey, sizeof(privateKey));
    if (sigLen <= 0) {
        GGL_MSG("ERROR: wc_SignatureGetSize %d\r\n", rc);
        GGL_EXIT("GGL_RS256_Sign", rc, 1);
        return rc;
    }

    byte sigBuf[sigLen];
    uint16_t jwtToSignSize = strlen(jwtToSign);
    rc = wc_SignatureGenerate(hashType, sigType, (byte*) jwtToSign, jwtToSignSize, sigBuf, (word32*) &sigLen, &privateKey, sizeof(privateKey), &rng);
    if (rc != 0) {
        GGL_MSG("ERROR: wc_SignatureGenerate %d\r\n", rc);
        GGL_EXIT("GGL_RS256_Sign", rc, 1);
        return rc;
    }

    // calculate the final length of the JWT
    word32 sigB64Len = GGL_SizeOfInB64(sigLen);
    uint16_t jwtFullLength = jwtToSignSize + sigB64Len + 2;

    credential->token = malloc(jwtFullLength);
    if (credential->token == NULL) {
        GGL_MSG("ERROR: out of memory while allocating buffer for JWT\r\n", -100);
        GGL_EXIT("GGL_RS256_Sign", -100, 1);
        return -100;
    }
    uint32_t len = sprintf(credential->token, "%s.", jwtToSign);
    char *encodedSignaturePos = &credential->token[len];

    if ((rc = Base64_Encode_NoNl(sigBuf, sigLen, (byte*) encodedSignaturePos, &sigB64Len)) != 0) {
        GGL_MSG("ERROR: Base64_Encode_NoNl %d\r\n", rc);
        GGL_EXIT("GGL_RS256_Sign", rc, 1);
        return rc;
    }

    encodedSignaturePos[sigB64Len] = '\0';
    GGL_EXIT("GGL_RS256_Sign", 0, 0);
    return 0;
}

int GGL_JWT_Create(const char* header, const char* body, const char *rsaPvtKey, size_t rsaPvtKeySize, GGL_Credential *credential) {
    GGL_ENTER("GGL_JWT_Create");

    // the total size of the Base64 encoded head.body is the sum of:
    //  - the b64 length of the head
    //  - the b64 length of the body
    //  - one dot and the string terminating zero
    word32 b64EncodedLength = GGL_SizeOfInB64(strlen(GGL_JWT_HEADER));
    b64EncodedLength += GGL_SizeOfInB64(strlen(body)) + 2;

    int rc;
    char b64EncodedBuffer[b64EncodedLength];
    char* buffPos = b64EncodedBuffer;
    word32 buffSizeIo = b64EncodedLength;
    if ((rc = Base64_Encode_NoNl((byte*) header, strlen(header), (byte*) buffPos, &buffSizeIo)) != 0) {
        GGL_MSG("ERROR: Base64_Encode_NoNl header %d\r\n", rc);
        GGL_EXIT("GGL_JWT_Create", rc, 1);
        return rc;
    }

    b64EncodedBuffer[buffSizeIo] = '.';
    buffPos = &b64EncodedBuffer[++buffSizeIo];
    buffSizeIo = b64EncodedLength - buffSizeIo;
    if ((rc = Base64_Encode_NoNl((byte*) body, strlen(body), (byte*) buffPos, &buffSizeIo)) != 0) {
        GGL_MSG("ERROR: Base64_Encode_NoNl claims %d\r\n", rc);
        GGL_EXIT("GGL_JWT_Create", rc, 1);
        return rc;
    }
    buffPos[buffSizeIo] = '\0';

    rc = GGL_JWT_RS256_Sign(b64EncodedBuffer, rsaPvtKey, rsaPvtKeySize, credential);
    if (rc != 0) {
        GGL_MSG("ERROR: GGL_RS256_Sign %d\r\n", rc);
        GGL_EXIT("GGL_JWT_Create", rc, 1);
        return rc;
    }

    GGL_EXIT("GGL_JWT_Create", rc, 0);
    return rc;
}

int GGL_SA_CreateAccessToken() {
    GGL_ENTER("GGL_SA_CreateAccessToken");

    GGL_Credential *cred = &GGL_Creds.saAccessToken;

    // if the current access token is valid, we don't need a new one
    uint32_t currentTimestamp = RTCUtils_GetEpochTimestamp();
    if (cred->validUntilTimestamp > 0 && (cred->validUntilTimestamp - CRED_INVALIDITY_DIFF) > currentTimestamp) {
        GGL_MSG("INFO: we've already a valid access token for SA\r\n");
        GGL_EXIT("GGL_SA_CreateAccessToken", 0, 0);
        return 0;
    }

    // we need an SA JWT for access token
    int rc = GGL_JWT_SA_Create();
    if (rc != 0) {
        GGL_MSG("ERROR: GGL_JWT_SA_Create - %d\r\n", rc);
        GGL_EXIT("GGL_SA_CreateAccessToken", rc, 1);
        return rc;
    }

    if (cred->token != NULL) {
        free(cred->token);
        cred->validUntilTimestamp = 0;
    }

    // now we have a valid JWT for getting access token
    HTTP_Request *atReq;
    rc = http_CreateRequest(GGL_Config->network.oAuthApiBasePath, HTTP_POST, HTTP_CONTENT_TYPE_FORM, &atReq);
    if (rc != HTTP_RC_OK) {
        GGL_MSG("ERROR: http_CreateRequest - %d\r\n", rc);
        GGL_EXIT("GGL_SA_CreateAccessToken", rc, 1);
        return rc;
    }

    rc = http_AddHeader(atReq, "Accept", "application/json");
    if (rc != HTTP_RC_OK) {
        GGL_MSG("ERROR: http_AddHeader (accept) - %d\r\n", rc);
        GGL_EXIT("GGL_SA_CreateAccessToken", rc, 1);
        return rc;
    }

    rc = http_AddBodyParam(atReq, "grant_type", "urn:ietf:params:oauth:grant-type:jwt-bearer");
    if (rc != HTTP_RC_OK) {
        GGL_MSG("ERROR: http_AddBodyParam (grant_type) - %d\r\n", rc);
        GGL_EXIT("GGL_SA_CreateAccessToken", rc, 1);
        return rc;
    }

    rc = http_AddBodyParam(atReq, "assertion", GGL_Creds.saJwt.token);
    if (rc != HTTP_RC_OK) {
        GGL_MSG("ERROR: http_AddBodyParam (assertion) - %d\r\n", rc);
        GGL_EXIT("GGL_SA_CreateAccessToken", rc, 1);
        return rc;
    }

    HTTP_Response *atResp;
    NetTransportContext transportCtx;
    rc = http_Execute(&transportCtx, atReq, &atResp);
    if (rc != HTTP_RC_OK) {
        GGL_MSG("ERROR: http_Execute - %d\r\n", rc);
        GGL_EXIT("GGL_SA_CreateAccessToken", rc, 1);
        return rc;
    }

    if (atResp->statusCode != 200) {
        GGL_MSG("ERROR: HTTP response returned status code: %d\r\n", atResp->statusCode);
        GGL_MSG("ERROR: HTTP response content is: %s\r\n", atResp->body);
        GGL_EXIT("GGL_SA_CreateAccessToken", -200, 1);
        return -200;
    }

    jsmn_parser p;
    jsmntok_t t[128];
    jsmn_init(&p);
    rc = jsmn_parse(&p, atResp->body, strlen(atResp->body), t, sizeof(t) / sizeof(t[0]));
    if (rc < 0) {
        GGL_MSG("ERROR: Could not parse response body (rc=%d, body=%s)\r\n", rc, atResp->body);
        GGL_EXIT("GGL_SA_CreateAccessToken", -201, 1);
        return -201;
    }

    char *keys[] = { "access_token", "expires_in" };
    char **values = json_GetValuesForKeys(atResp->body, keys, 2, t, rc);
    GGL_Creds.saAccessToken.token = values[0];
    GGL_Creds.saAccessToken.validUntilTimestamp = RTCUtils_GetEpochTimestamp() + atoi(values[1]);
    free(values[1]);
    free(values);
    http_Cleanup(atReq, atResp);
    GGL_EXIT("GGL_SA_CreateAccessToken", 0, 0);
    return 0;
}

int GGL_JWT_SA_Create() {
    GGL_ENTER("GGL_JWT_SA_Create");

    GGL_Credential *cred = &GGL_Creds.saJwt;

    if (cred->token != NULL) {
        free(cred->token);
        cred->validUntilTimestamp = 0;
    }

    uint16_t jwtBodySize = strlen(GGL_JWT_SA_BODY_TEMPLATE) - 10;
    jwtBodySize += 2 * UINT32_STR_LEN + strlen(GGL_Config->network.saClientEmail) + strlen(GGL_Config->network.saScope);
    char jwtBody[jwtBodySize + 1];

    uint32_t iat = RTCUtils_GetEpochTimestamp();
    uint32_t exp = iat + 3600;
    sprintf(jwtBody, GGL_JWT_SA_BODY_TEMPLATE, GGL_Config->network.saClientEmail, GGL_Config->network.saScope, exp, iat);

    int rc = GGL_JWT_Create(GGL_JWT_HEADER, jwtBody, GGL_Config->network.saPrivateKey, GGL_Config->network.saPrivateKeySize, cred);
    if (rc != 0) {
        GGL_MSG("ERROR: GGL_JWT_Create %d\r\n", rc);
        GGL_EXIT("GGL_JWT_SA_Create", rc, 1);
        return rc;
    }
    cred->validUntilTimestamp = exp;
    GGL_EXIT("GGL_JWT_SA_Create", 0, 0);
    return 0;
}

int GGL_JWT_MQTT_Create() {
    GGL_ENTER("GGL_JWT_MQTT_Create");

    GGL_Credential *cred = &GGL_Creds.mqttJwt;

    if (cred->token != NULL) {
        free(cred->token);
        cred->validUntilTimestamp = 0;
    }

    uint16_t jwtBodySize = strlen(GGL_JWT_MQTT_BODY_TEMPLATE) - 8;
    jwtBodySize += 2 * UINT32_STR_LEN + strlen(GGL_Config->device.projectId);
    char jwtBody[jwtBodySize + 1];

    uint32_t iat = RTCUtils_GetEpochTimestamp();
    uint32_t exp = iat + 24 * 3600;
    sprintf(jwtBody, GGL_JWT_MQTT_BODY_TEMPLATE, iat, exp, GGL_Config->device.projectId);

    int rc = GGL_JWT_Create(GGL_JWT_HEADER, jwtBody, GGL_Config->network.mqttPrivateKey, GGL_Config->network.mqttPrivateKeySize, cred);
    if (rc != 0) {
        GGL_MSG("ERROR: GGL_JWT_Create %d\r\n", rc);
        GGL_EXIT("GGL_JWT_MQTT_Create", rc, 1);
        return rc;
    }
    cred->validUntilTimestamp = exp;
    GGL_EXIT("GGL_JWT_MQTT_Create", 0, 0);
    return 0;
}
