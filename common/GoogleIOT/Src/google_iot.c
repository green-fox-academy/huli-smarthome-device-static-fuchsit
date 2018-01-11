#include "google_iot.h"
#include "smarthome_log.h"
#include "mqtt_net.h"
#include "wolfmqtt/mqtt_client.h"
#include "wolfssl/ssl.h"
#include "wolfssl/wolfcrypt/coding.h"
#include "wolfssl/wolfcrypt/signature.h"
#include "wolfssl/wolfcrypt/rsa.h"
#include "rtc_utils.h"
#include "device_keys.h"

#define GGL_ENTER(fnc)				SHOME_LogEnter("ggl", fnc)
#define GGL_MSG(fnc, ...)			SHOME_LogMsg(fnc, ##__VA_ARGS__)
#define GGL_EXIT(fnc, rc, fail)		SHOME_LogExit("ggl", fnc, rc, fail)

static GGL_InitDef *GGL_Config;

extern MqttClient mqttClient;

int GGL_JWT_Create(char *buffer, word32 bufferLen);
int GGL_RS256_Sign(const char *dataBuff, char *signature, word32 signatureSize);

void GGL_IOT_Init(GGL_InitDef *config) {
	GGL_ENTER("GGL_IOT_Init");
	GGL_Config = config;
	GGL_EXIT("GGL_IOT_Init", 0, 0);
}

int GGL_IOT_MQTT_MessageArrivedCallback(struct _MqttClient *client,
		MqttMessage *message, byte msg_new, byte msg_done) {
	GGL_ENTER("GGL_IOT_MQTT_MessageArrivedCallback");

	byte topicBuf[MQTT_MAX_BUFFER_SIZE + 1];
	byte messageBuf[MQTT_MAX_BUFFER_SIZE + 1];

	if (msg_new) {
		XMEMCPY(topicBuf, message->topic_name, message->topic_name_len);
		topicBuf[message->topic_name_len] = '\0';
		GGL_MSG("Got MQTT message to topic: %s\r\n", topicBuf);
	}

	if (!msg_done) {
		GGL_MSG(
				"ERROR: MQTT message exceeds the maximum buffer size, which is not supported in this library.\r\n");
		GGL_EXIT("GGL_IOT_MQTT_MessageArrivedCallback", -1, 1);
		return -1;
	}

	XMEMCPY(messageBuf, message->buffer, message->buffer_len);
	messageBuf[message->buffer_len] = '\0';
	int rc = GGL_Config->callback((char*)topicBuf, (char*)messageBuf);
	if (rc != 0) {
		GGL_EXIT("GGL_IOT_MQTT_MessageArrivedCallback", rc, 1);
		return rc;

	}
	GGL_EXIT("GGL_IOT_MQTT_MessageArrivedCallback", 0, 0);
	return 0;
}

int GGL_MQTT_Connect() {
	GGL_ENTER("GGL_MQTT_Connect");

	int rc = MQTT_Init(GGL_IOT_MQTT_MessageArrivedCallback);

	if (rc != MQTT_CODE_SUCCESS) {
		GGL_MSG("ERROR: unsuccessful MQTT_Init %d=%s\r\n", rc,
				MqttClient_ReturnCodeToString(rc));
		GGL_EXIT("GGL_MQTT_Connect", rc, 1);
		return rc;
	}

	rc = MqttClient_NetConnect(&mqttClient, GGL_Config->network.mqttHost,
			GGL_Config->network.mqttPort, MQTT_DEFAULT_TIMEOUT, 1, NULL);
	if (rc != MQTT_CODE_SUCCESS) {
		GGL_MSG("ERROR: unsuccessful MqttClient_NetConnect %d=%s\r\n", rc,
				MqttClient_ReturnCodeToString(rc));
		GGL_EXIT("GGL_MQTT_Connect", rc, 1);
		return rc;
	}

	MqttConnect conn;
	XMEMSET(&conn, 0, sizeof(MqttConnect));

	char clientIdBuf[256];
	sprintf(clientIdBuf, "projects/%s/locations/%s/registries/%s/devices/%s",
			GGL_Config->device.projectId, GGL_Config->device.region,
			GGL_Config->device.deviceRegistry, GGL_Config->device.deviceId);

	conn.clean_session = 1;
	conn.keep_alive_sec = 60;
	conn.client_id = clientIdBuf;
	conn.enable_lwt = 0;

	char password[512];
	rc = GGL_JWT_Create(password, 512);
	if (rc != 0) {
		GGL_MSG("ERROR: couldn't create JWT password %rc\r\n", rc);
		GGL_EXIT("GGL_MQTT_Connect", rc, 1);
		return rc;
	}

	conn.username = "unused";
	conn.password = password;

	rc = MqttClient_Connect(&mqttClient, &conn);
	if (rc != 0) {
		GGL_MSG("ERROR: unsuccessful MqttClient_Connect %d=%s\r\n", rc,
				MqttClient_ReturnCodeToString(rc));
		GGL_EXIT("GGL_MQTT_Connect", rc, 1);
		return rc;
	}
	if (conn.ack.return_code != 0) {
		GGL_MSG("ERROR: unsuccessful MqttClient_Connect, ack RC: %d\r\n",
				conn.ack.return_code);
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
	sprintf((char*)topicNameBuf, "/devices/%s/%s",
			GGL_Config->device.deviceId, topicName);
}

int GGL_MQTT_Subscribe(const char* topicName) {
	GGL_ENTER("GGL_MQTT_Subscribe");
	MqttSubscribe sub;
	XMEMSET(&sub, 0, sizeof(MqttSubscribe));

	char topicNameBuf[256];
	GGL_MQTT_CreateTopicName(topicName, topicNameBuf);

	MqttTopic top[] = { { topicNameBuf, 0 } };
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


int GGL_RS256_Sign(const char *dataBuff, char *signature, word32 signatureSize) {
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
	byte privateKeyDer[PRIVATE_KEY_SIZE];
	XMEMCPY(privateKeyDer, PRIVATE_KEY, (size_t)PRIVATE_KEY_SIZE);
	word32 privateKeyDerSize = PRIVATE_KEY_SIZE;
	if ((rc = wc_RsaPrivateKeyDecode(privateKeyDer, &idx, &privateKey, privateKeyDerSize))
			!= 0) {
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
	uint16_t dataSize = strlen(dataBuff);
	rc = wc_SignatureGenerate(hashType, sigType, (byte*)dataBuff, dataSize, sigBuf,
			(word32*)&sigLen, &privateKey, sizeof(privateKey), &rng);
	if (rc != 0) {
		GGL_MSG("ERROR: wc_SignatureGenerate %d\r\n", rc);
		GGL_EXIT("GGL_RS256_Sign", rc, 1);
		return rc;
	}

	if ((rc = Base64_Encode_NoNl(sigBuf, sigLen, (byte*)signature, &signatureSize)) != 0) {
		GGL_MSG("ERROR: Base64_Encode_NoNl %d\r\n", rc);
		GGL_EXIT("GGL_RS256_Sign", rc, 1);
		return rc;
	}

	signature[signatureSize] = '\0';
	GGL_EXIT("GGL_RS256_Sign", 0, 0);
	return 0;
}

int GGL_JWT_Create(char *buffer, word32 bufferLen) {
	GGL_ENTER("GGL_JWT_Create");
	const char *head = "{\"alg\":\"RS256\",\"typ\":\"JWT\"}";
	const word32 headSize = strlen(head);
	int rc;

	char* buffPos = &buffer[0];
	word32 buffSizeIo = bufferLen;
	if ((rc = Base64_Encode_NoNl((byte*) head, headSize, (byte*) buffPos,
			&buffSizeIo)) != 0) {
		GGL_MSG("ERROR: Base64_Encode_NoNl header %d\r\n", rc);
		GGL_EXIT("GGL_JWT_Create", rc, 1);
		return rc;
	}

	buffer[buffSizeIo] = '.';
	char claims[128];
	uint32_t iat = RTCUtils_GetEpochTimestamp();
	uint32_t exp = iat + 24*3600;
	sprintf(claims, "{\"iat\":%lu,\"exp\":%lu,\"aud\":\"%s\"}", iat, exp, GGL_Config->device.projectId);
	const word32 claimsSize = strlen(claims);

	buffPos = &buffer[++buffSizeIo];
	buffSizeIo = bufferLen - buffSizeIo;
	if ((rc = Base64_Encode_NoNl((byte*) claims, claimsSize, (byte*) buffPos,
			&buffSizeIo)) != 0) {
		GGL_MSG("ERROR: Base64_Encode_NoNl claims %d\r\n", rc);
		GGL_EXIT("GGL_JWT_Create", rc, 1);
		return rc;
	}
	buffPos[buffSizeIo] = '\0';

	char signature[512];
	rc = GGL_RS256_Sign(buffer, signature, 512);
	if (rc != 0) {
		GGL_MSG("ERROR: GGL_RS256_Sign %d\r\n", rc);
		GGL_EXIT("GGL_JWT_Create", rc, 1);
		return rc;
	}

	buffPos[buffSizeIo++] = '.';
	buffPos = &buffPos[buffSizeIo];

	strcpy(buffPos, signature);

	GGL_EXIT("GGL_JWT_Create", rc, 0);
	return rc;
}
