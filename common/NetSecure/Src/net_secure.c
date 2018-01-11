#include <stdint.h>
#include <time.h>

#include "user_settings.h"
#include "net_transport.h"
#include "net_secure.h"
#include "rtc_utils.h"
#include "smarthome_log.h"

#define NET_SECURE_DEFAULT_TIMEOUT		5000

#define NETS_ENTER(fnc)					SHOME_LogEnter("nets", fnc)
#define NETS_MSG(fnc, ...)				SHOME_LogMsg(fnc, ##__VA_ARGS__)
#define NETS_EXIT(fnc, rc, fail)		SHOME_LogExit("nets", fnc, rc, fail)

#define CTX(CTX)	((NetTransportContext*)CTX)

int WolfSSL_IORecvCallback(WOLFSSL *ssl, char *buf, int sz, void *ctx);
int WolfSSL_IOSendCallback(WOLFSSL *ssl, char *buf, int sz, void *ctx);

RNG_HandleTypeDef *net_secure_rngHandle;

void net_SecureInit(NetSecure_InitTypeDef *netSecureInit) {
	net_secure_rngHandle = netSecureInit->rngHandle;
}

time_t net_CustomTimestampCallback(time_t x) {
	return RTCUtils_GetEpochTimestamp();
}

uint32_t net_CustomRandomCallback(void) {
	return HAL_RNG_GetRandomNumber(net_secure_rngHandle);
}

int net_TLSConnect(void *netTransportContext, SocketType sockType,
		const char* host, uint16_t port, uint32_t timeoutMs) {
	NETS_ENTER("net_TLSConnect");
	NetTransportContext *ctx = CTX(netTransportContext);

	if ((ctx->sslCtx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
		NETS_MSG("wolfSSL_CTX_new failed\r\n");
		NETS_EXIT("net_TLSConnect", -1, 1);
		return -1;
	}

	wolfSSL_SetIORecv(ctx->sslCtx, WolfSSL_IORecvCallback);
	wolfSSL_SetIOSend(ctx->sslCtx, WolfSSL_IOSendCallback);

	if ((ctx->ssl = wolfSSL_new(ctx->sslCtx)) == NULL) {
		NETS_MSG("wolfSSL_new failed\r\n");
		NETS_EXIT("net_TLSConnect", -2, 1);
		return -2;
	}

	wolfSSL_SetIOReadCtx(ctx->ssl, netTransportContext);
	wolfSSL_SetIOWriteCtx(ctx->ssl, netTransportContext);

	int rc = net_Connect(netTransportContext, sockType, host, port, timeoutMs);
	if (rc != RC_SUCCESS) {
		NETS_MSG("net_Connect failed %d\r\n", rc);
		NETS_EXIT("net_TLSConnect", rc, 1);
		return rc;
	}

	rc = wolfSSL_connect(ctx->ssl);
	if (rc != WOLFSSL_SUCCESS) {
		NETS_MSG("wolfSSL_connect failed %d\r\n", rc);
		NETS_EXIT("net_TLSConnect", rc, 1);
		return rc;
	}

	NETS_EXIT("net_TLSConnect", 0, 0);
	return 0;
}

int net_TLSDisconnect(void *netTransportContext) {
	NETS_ENTER("net_TLSDisconnect");
	NetTransportContext *ctx = CTX(netTransportContext);
	wolfSSL_free(ctx->ssl);
	wolfSSL_CTX_free(ctx->sslCtx);
	int rc = wolfSSL_Cleanup();
	if (rc != WOLFSSL_SUCCESS) {
		NETS_MSG("wolfSSL_Cleanup failed %d\r\n", rc);
		NETS_EXIT("net_TLSDisconnect", rc, 1);
		return rc;

	}
	rc = net_Disconnect(netTransportContext);
	if (rc != 0) {
		NETS_MSG("net_Disconnect failed %d\r\n", rc);
		NETS_EXIT("net_TLSDisconnect", rc, 1);
		return rc;

	}
	NETS_EXIT("net_TLSDisconnect", 0, 0);
	return 0;
}

int net_TLSSend(void *netTransportContext, const char* buffer,
		const uint32_t bufferSz, uint32_t timeoutMs) {
	NETS_ENTER("net_TLSSend");
	NetTransportContext *ctx = CTX(netTransportContext);
	int rc = wolfSSL_write(ctx->ssl, buffer, bufferSz);
	if (rc <= 0) {
		NETS_MSG("wolfSSL_write failed %d\r\n", rc);
		NETS_EXIT("net_TLSSend", rc, 1);
		return rc;
	}
	NETS_EXIT("net_TLSSend", 0, 0);
	return -1;
}

int net_TLSReceive(void *netTransportContext, const char* buffer,
		const uint32_t bufferSz, uint32_t timeoutMs) {
	NETS_ENTER("net_TLSReceive");
	NetTransportContext *ctx = CTX(netTransportContext);
	int rc = wolfSSL_read(ctx->ssl, (char*)buffer, bufferSz);
	if (rc <= 0) {
		NETS_MSG("wolfSSL_read failed %d\r\n", rc);
		NETS_EXIT("net_TLSReceive", rc, 1);
		return rc;
	}
	NETS_EXIT("net_TLSReceive", 0, 0);
	return -1;
}

int net_TLSDestroy(void *netTransportContext) {
	NETS_ENTER("net_TLSDestroy");
	int rc;
	if ((rc = net_Destroy(netTransportContext)) != 0) {
		NETS_MSG("net_Destroy failed %d\r\n", rc);
		NETS_EXIT("net_TLSDestroy", rc, 1);
	}
	NETS_EXIT("net_TLSDestroy", 0, 0);
	return 0;
}

int WolfSSL_IORecvCallback(WOLFSSL *ssl, char *buf, int sz, void *ctx) {
	return net_Receive(ctx, buf, sz, NET_SECURE_DEFAULT_TIMEOUT);
}

int WolfSSL_IOSendCallback(WOLFSSL *ssl, char *buf, int sz, void *ctx) {
	return net_Send(ctx, buf, sz, NET_SECURE_DEFAULT_TIMEOUT);
}
