#include "user_settings.h"
#include "net_transport.h"
#include "smarthome_log.h"
#include "wifi.h"

#define NET_ENTER(fnc)				SHOME_LogEnter("net", fnc)
#define NET_MSG(fnc, ...)			SHOME_LogMsg(fnc, ##__VA_ARGS__)
#define NET_EXIT(fnc, rc, fail)		SHOME_LogExit("net", fnc, rc, fail)

#define CTX(CTX)	((NetTransportContext*)CTX)

typedef int (*NetReadWriteCb)(uint32_t connId, const char* buffer,
		const uint32_t bufferSz, uint16_t *processed, uint32_t timeoutMs);

int net_InnerSendUnsafe(uint32_t connId, const char* buffer,
		const uint32_t bufferSz, uint16_t *sent, uint32_t timeoutMs);
int net_InnerReceiveUnsafe(uint32_t connId, const char* buffer,
		const uint32_t bufferSz, uint16_t *received, uint32_t timeoutMs);

int net_Init(void *netTransportContext) {
	NET_ENTER("net_Init");
	NetTransportContext *ctx = CTX(ctx);
	int rc;
	if ((rc = WIFI_Init()) != WIFI_STATUS_OK) {
		NET_MSG("WIFI_Init() failed\r\n");
		NET_EXIT("net_Init", rc, 1);
		return rc;
	}
	if ((rc = WIFI_GetMAC_Address(ctx->macAddr)) != WIFI_STATUS_OK) {
		NET_MSG("WIFI_GetMAC_Address() failed\r\n");
		NET_EXIT("net_Init", rc, 1);
		return rc;
	}
	NET_EXIT("net_Init", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_Connect(void *netTransportContext, SocketType sockType,
		const char* host, uint16_t port, uint32_t timeoutMs) {
	NetTransportContext *ctx = CTX(ctx);
	NET_ENTER("net_Connect");

	uint8_t destIp[4];
	int rc;
	if ((rc = WIFI_GetHostAddress((char*) host, destIp)) != WIFI_STATUS_OK) {
		NET_MSG("DNS lookup failed!\r\n");
		NET_EXIT("net_Connect", rc, 1);
		return rc;
	}

	NET_MSG("DNS lookup result: %s => %d.%d.%d.%d\r\n", host, destIp[0],
			destIp[1], destIp[2], destIp[3]);
	WIFI_Protocol_t protocol =
			sockType == SOCKET_UDP ? WIFI_UDP_PROTOCOL : WIFI_TCP_PROTOCOL;
	if ((rc = WIFI_OpenClientConnection(ctx->connectionId, protocol, "client",
			destIp, port, timeoutMs)) != WIFI_STATUS_OK) {
		NET_MSG("Could not open connection\r\n");
		NET_EXIT("net_Connect", rc, 1);
		return rc;
	}

	NET_EXIT("net_Connect", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_Disconnect(void *netTransportContext) {
	NetTransportContext *ctx = CTX(ctx);
	NET_ENTER("net_Disconnect");
	int rc;
	if ((rc = WIFI_CloseClientConnection(ctx->connectionId))
			!= WIFI_STATUS_OK) {
		NET_MSG("Could not close connection\r\n");
		NET_EXIT("net_Disconnect", rc, 1);
	}
	NET_EXIT("net_Connect", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_ProcessRecoursively(uint32_t connId, const char *buffer,
		uint32_t bufferSz, uint16_t *totalSent, NetReadWriteCb callback,
		uint32_t timeoutMs, uint32_t spentTime) {
	NET_ENTER("net_ProcessRecoursively");
	// check the timeout
	if (spentTime > timeoutMs) {
		NET_MSG("ERROR: no more data, so timeout\r\n");
		NET_EXIT("net_ProcessRecoursively", RC_TIMEOUT, 1);
		return RC_TIMEOUT;
	}
	uint32_t iterationStart = HAL_GetTick();

	uint32_t toSend = MIN(ES_WIFI_PAYLOAD_SIZE, bufferSz-*totalSent);
	uint16_t currentProcessed = 0;
	char *remBufferPtr = (char*)buffer + *totalSent;
	int rc = callback(connId, remBufferPtr, toSend, &currentProcessed, timeoutMs);
	if (rc != WIFI_STATUS_OK) {
		NET_EXIT("net_ProcessRecoursively", rc, 1);
		return rc;
	}

	spentTime += (HAL_GetTick() - iterationStart);

	*totalSent += currentProcessed;
	if (*totalSent == bufferSz) {
		NET_EXIT("net_ProcessRecoursively", RC_SUCCESS, 0);
		return RC_SUCCESS;
	}

	rc = net_ProcessRecoursively(connId, buffer, bufferSz,
			totalSent, callback, timeoutMs, spentTime);

	if (rc != WIFI_STATUS_OK) {
		NET_EXIT("net_ProcessRecoursively", rc, 1);
		return rc;
	}
	NET_EXIT("net_ProcessRecoursively", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_Send(void *netTransportContext, const char* buffer,
		const uint32_t bufferSz, uint32_t timeoutMs) {
	NetTransportContext *ctx = CTX(ctx);
	NET_ENTER("net_Send");
	uint16_t totalSent = 0;
	int rc = net_ProcessRecoursively(ctx->connectionId, buffer, bufferSz,
			&totalSent, net_InnerSendUnsafe, timeoutMs, 0);
	if (rc != RC_SUCCESS) {
		NET_MSG("the send was failed with rc=%d\r\n", rc);
		NET_EXIT("net_Send", RC_ERROR, 1);
		return RC_ERROR;
	}
	NET_EXIT("net_Send", RC_SUCCESS, 0);
	return totalSent;
}

int net_Receive(void *netTransportContext, const char* buffer,
		const uint32_t bufferSz, uint32_t timeoutMs) {
	NetTransportContext *ctx = CTX(ctx);
	NET_ENTER("net_Receive");
	uint16_t totalReceived = 0;
	int rc = net_ProcessRecoursively(ctx->connectionId, buffer, bufferSz,
			&totalReceived, net_InnerReceiveUnsafe, timeoutMs, 0);
	if (rc == RC_SUCCESS) {
		NET_EXIT("net_Receive", RC_SUCCESS, 0);
		return totalReceived;
	}
	if (rc == RC_TIMEOUT && totalReceived > 0) {
		NET_MSG("WARNING: timeout, but already have some data, returning the read amount\r\n");
		NET_EXIT("net_Receive", RC_SUCCESS, 0);
		return totalReceived;
	}

	NET_MSG("the receive was failed with rc=%d\r\n", rc);
	NET_EXIT("net_Receive", RC_ERROR, 1);
	return RC_ERROR;

}


int net_Destroy(void *netTransportContext) {
	NET_ENTER("net_Destroy");
	NET_EXIT("net_Destroy", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_InnerSendUnsafe(uint32_t connId, const char* buffer,
		const uint32_t bufferSz, uint16_t *sent, uint32_t timeoutMs) {
	NET_ENTER("net_InnerSend");

	int rc = WIFI_SendData(connId, (uint8_t*) buffer, bufferSz, sent,
			timeoutMs);
	if (rc) {
		NET_EXIT("net_InnerSend", rc, 1);
		return rc;
	}
	NET_EXIT("net_InnerSend", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_InnerReceiveUnsafe(uint32_t connId, const char* buffer,
		const uint32_t bufferSz, uint16_t *received, uint32_t timeoutMs) {
	NET_ENTER("net_InnerReceive");

	int rc = WIFI_ReceiveData(connId, (uint8_t*) buffer, bufferSz, received,
			timeoutMs);
	if (rc) {
		NET_EXIT("net_InnerReceive", rc, 1);
		return rc;
	}
	NET_EXIT("net_InnerReceive", RC_SUCCESS, 0);
	return RC_SUCCESS;
}
