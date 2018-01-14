#include "user_settings.h"
#include "net_transport.h"
#include "smarthome_log.h"
#include "wifi.h"

#define NET_ENTER(fnc)				SHOME_LogEnter("net", fnc)
#define NET_MSG(fnc, ...)			SHOME_LogMsg(fnc, ##__VA_ARGS__)
#define NET_EXIT(fnc, rc, fail)		SHOME_LogExit("net", fnc, rc, fail)

typedef int (*NetReadWriteCb)(uint32_t connId, const char* buffer,
		const uint32_t bufferSz, uint16_t *processed, uint32_t timeoutMs);

int net_InnerSendUnsafe(uint32_t connId, const char* buffer,
		const uint32_t bufferSz, uint16_t *sent, uint32_t timeoutMs);
int net_InnerReceiveUnsafe(uint32_t connId, const char* buffer,
		const uint32_t bufferSz, uint16_t *received, uint32_t timeoutMs);

typedef enum ServerConnectionStatus {
	SC_INIT, SC_CONNECTED
} ServerConnectionStatus;

NetHandleClientConnectionCallback net_HandleClientCallback;

int net_Init(NetTransportContext *ctx) {
	NET_ENTER("net_Init");
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

int net_DNSLookup(const char* host, uint8_t* resultIp) {
	NET_ENTER("net_DNSLookup");
	int rc;
	if ((rc = WIFI_GetHostAddress((char*) host, resultIp)) != WIFI_STATUS_OK) {
		NET_MSG("DNS lookup failed!\r\n");
		NET_EXIT("net_DNSLookup", rc, 1);
		return rc;
	}

	NET_EXIT("net_DNSLookup", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_Connect(NetTransportContext *ctx, SocketType sockType, const char* host,
		uint16_t port, uint32_t timeoutMs) {
	NET_ENTER("net_Connect");

	uint8_t destIp[4];
	int rc;
	if ((rc = net_DNSLookup(host, destIp)) != WIFI_STATUS_OK) {
		NET_MSG("DNS lookup failed!\r\n");
		NET_EXIT("net_Connect", rc, 1);
		return rc;
	}

	NET_MSG("DNS lookup result: %s => %d.%d.%d.%d\r\n", host, destIp[0],
			destIp[1], destIp[2], destIp[3]);

	rc = net_ConnectIp(ctx, sockType, destIp, port, timeoutMs);
	if (rc != WIFI_STATUS_OK) {
		NET_MSG("Could not open connection\r\n");
		NET_EXIT("net_Connect", rc, 1);
		return rc;
	}

	NET_EXIT("net_Connect", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_ConnectIp(NetTransportContext *ctx, SocketType sockType,
		uint8_t targetIp[4], uint16_t port, uint32_t timeoutMs) {
	NET_ENTER("net_ConnectIp");

	WIFI_Protocol_t protocol =
			sockType == SOCKET_UDP ? WIFI_UDP_PROTOCOL : WIFI_TCP_PROTOCOL;
	int rc;
	if ((rc = WIFI_OpenClientConnection(ctx->connection.id, protocol, "client",
			targetIp, port, timeoutMs)) != WIFI_STATUS_OK) {
		NET_MSG("Could not open connection\r\n");
		NET_EXIT("net_ConnectIp", rc, 1);
		return rc;
	}
	NET_EXIT("net_ConnectIp", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

void net_SetHandleClientConnectionCallback(
		NetHandleClientConnectionCallback callback) {
	net_HandleClientCallback = callback;
}

int net_StartServerConnection(NetTransportContext *netTransportContext,
		SocketType sockType, uint16_t port) {
	NET_ENTER("net_StartServerConnection");

	ServerConnectionStatus status = SC_INIT;
	WIFI_Protocol_t protocol =
			sockType == SOCKET_UDP ? WIFI_UDP_PROTOCOL : WIFI_TCP_PROTOCOL;

	while (1) {
		int rc, cbrc;
		switch (status) {
		case SC_INIT:
			rc = WIFI_StartServer(netTransportContext->connection.id, protocol, "", port);
			if (rc != WIFI_STATUS_OK) {
				NET_MSG("WIFI_StartServer failed %d\r\n", rc);
				NET_EXIT("net_StartServerConnection", rc, 1);
				return rc;
			}
			status = SC_CONNECTED;
			continue;
		case SC_CONNECTED:
			cbrc = net_HandleClientCallback(netTransportContext);
			rc = net_StopServerConnection(netTransportContext);
			if (rc != WIFI_STATUS_OK) {
				NET_MSG("WIFI_StopServer failed %d\r\n", rc);
			}

			if (cbrc < 0) {
				NET_MSG("Callback returned %d -> ending server process\r\n",
						cbrc);
				NET_EXIT("net_StartServerConnection", RC_SUCCESS, 0);
				return 0;
			}
			status = SC_INIT;
			continue;
		}
	}
	NET_EXIT("net_StartServerConnection", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_StopServerConnection(NetTransportContext *netTransportContext) {
	NET_EXIT("net_StopServerConnection", RC_SUCCESS, 0);
	int rc = WIFI_StopServer(netTransportContext->connection.id);
	if (rc != WIFI_STATUS_OK) {
		NET_MSG("WIFI_StopServer failed %d\r\n", rc);
		NET_EXIT("net_StopServerConnection", rc, 1);
		return rc;
	}
	NET_EXIT("net_StopServerConnection", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_Disconnect(NetTransportContext *ctx) {
	NET_ENTER("net_Disconnect");
	int rc;
	if ((rc = WIFI_CloseClientConnection(ctx->connection.id))
			!= WIFI_STATUS_OK) {
		NET_MSG("Could not close connection\r\n");
		NET_EXIT("net_Disconnect", rc, 1);
	}
	NET_EXIT("net_Disconnect", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_ProcessRecoursively(uint32_t connId, const char *buffer,
		uint32_t bufferSz, uint16_t *totalSent, NetReadWriteCb callback,
		uint32_t timeoutMs, uint32_t spentTime) {
	NET_ENTER("net_ProcessRecoursively");

	if (spentTime > timeoutMs) {
		NET_MSG("ERROR: no more data, so timeout\r\n");
		NET_EXIT("net_ProcessRecoursively", RC_TIMEOUT, 1);
		return RC_TIMEOUT;
	}
	uint32_t iterationStart = HAL_GetTick();

	uint32_t toSend = MIN(ES_WIFI_PAYLOAD_SIZE, bufferSz - *totalSent);
	uint16_t currentProcessed = 0;
	char *remBufferPtr = (char*) buffer + *totalSent;
	int rc = callback(connId, remBufferPtr, toSend, &currentProcessed,
			timeoutMs);
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

	rc = net_ProcessRecoursively(connId, buffer, bufferSz, totalSent, callback,
			timeoutMs, spentTime);

	if (rc != WIFI_STATUS_OK) {
		NET_EXIT("net_ProcessRecoursively", rc, 1);
		return rc;
	}
	NET_EXIT("net_ProcessRecoursively", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_Send(NetTransportContext *ctx, const char* buffer,
		const uint32_t bufferSz, uint32_t timeoutMs) {
	NET_ENTER("net_Send");
	uint16_t totalSent = 0;
	int rc = net_ProcessRecoursively(ctx->connection.id, buffer, bufferSz,
			&totalSent, net_InnerSendUnsafe, timeoutMs, 0);
	if (rc != RC_SUCCESS) {
		NET_MSG("the send was failed with rc=%d\r\n", rc);
		NET_EXIT("net_Send", RC_ERROR, 1);
		return RC_ERROR;
	}
	NET_EXIT("net_Send", RC_SUCCESS, 0);
	return totalSent;
}

int net_Receive(NetTransportContext *ctx, const char* buffer,
		const uint32_t bufferSz, uint32_t timeoutMs) {
	NET_ENTER("net_Receive");
	uint16_t totalReceived = 0;
	int rc = net_ProcessRecoursively(ctx->connection.id, buffer, bufferSz,
			&totalReceived, net_InnerReceiveUnsafe, timeoutMs, 0);
	if (rc == RC_SUCCESS) {
		NET_EXIT("net_Receive", RC_SUCCESS, 0);
		return totalReceived;
	}
	if (rc == RC_TIMEOUT && totalReceived >= 0) {
		NET_MSG(
				"WARNING: timeout, data size in read buffer: %d\r\n", totalReceived);
		NET_EXIT("net_Receive", RC_SUCCESS, 0);
		return totalReceived;
	}

	NET_MSG("the receive was failed with rc=%d\r\n", rc);
	NET_EXIT("net_Receive", RC_ERROR, 1);
	return RC_ERROR;

}

int net_Destroy(NetTransportContext *netTransportContext) {
	NET_ENTER("net_Destroy");
	NET_EXIT("net_Destroy", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_InnerSendUnsafe(uint32_t connId, const char* buffer,
		const uint32_t bufferSz, uint16_t *sent, uint32_t timeoutMs) {
	NET_ENTER("net_InnerSendUnsafe");

	int rc = WIFI_SendData(connId, (uint8_t*) buffer, bufferSz, sent,
			timeoutMs);
	if (rc) {
		NET_EXIT("net_InnerSendUnsafe", rc, 1);
		return rc;
	}
	NET_EXIT("net_InnerSendUnsafe", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_InnerReceiveUnsafe(uint32_t connId, const char* buffer,
		const uint32_t bufferSz, uint16_t *received, uint32_t timeoutMs) {
	NET_ENTER("net_InnerReceiveUnsafe");

	int rc = WIFI_ReceiveData(connId, (uint8_t*) buffer, bufferSz, received,
			timeoutMs);
	if (rc) {
		NET_EXIT("net_InnerReceiveUnsafe", rc, 1);
		return rc;
	}
	NET_EXIT("net_InnerReceiveUnsafe", RC_SUCCESS, 0);
	return RC_SUCCESS;
}
