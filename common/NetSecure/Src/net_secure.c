#include <stdint.h>
#include <time.h>

#include "wolfssl/wolfcrypt/logging.h"
#include "user_settings.h"
#include "net_transport.h"
#include "net_secure.h"
#include "rtc_utils.h"
#include "smarthome_log.h"

#define NET_SECURE_DEFAULT_TIMEOUT		5000

#define NETS_ENTER(fnc)					SHOME_LogEnter("nets", fnc)
#define NETS_MSG(fnc, ...)				SHOME_LogMsg(fnc, ##__VA_ARGS__)
#define NETS_EXIT(fnc, rc, fail)		SHOME_LogExit("nets", fnc, rc, fail)

int WolfSSL_IORecvCallback(WOLFSSL *ssl, char *buf, int sz, void *ctx);
int WolfSSL_IOSendCallback(WOLFSSL *ssl, char *buf, int sz, void *ctx);

NetSecure_InitTypeDef *net_secure_config;
NetHandleClientConnectionCallback net_secure_HandleClientConnectionCallback;

int net_TLSNetHandleClientConnectionCallbackWrapper(NetTransportContext *ctx);

void wolfSSL_LoggingCallback(const int logLevel, const char * const logMessage) {
	if (!net_secure_config->debugEnable) {
		return;
	}
	char* logLevelStr;
	switch (logLevel) {
	case (ERROR_LOG):
		logLevelStr = "ERROR";
		break;
	case (INFO_LOG):
		logLevelStr = "INFO";
		break;
	case (ENTER_LOG):
		logLevelStr = "ENTER";
		break;
	case (LEAVE_LOG):
		logLevelStr = "EXIT";
		break;
	case (OTHER_LOG):
		logLevelStr = "OTHER";
		break;
	default:
		logLevelStr = "UNKNOWN";
	}
	NETS_MSG("wolfssl > [%s] %s\r\n", logLevelStr, logMessage);
}

void net_SecureInit(NetSecure_InitTypeDef *netSecureInit) {
	NETS_ENTER("net_SecureInit");
	net_secure_config = netSecureInit;
	wolfSSL_Init();
	if (netSecureInit->debugEnable) {
		wolfSSL_SetLoggingCb(wolfSSL_LoggingCallback);
		wolfSSL_Debugging_ON();
	}
	NETS_EXIT("net_SecureInit", 0, 0);
}

time_t net_CustomTimestampCallback(time_t x) {
	return RTCUtils_GetEpochTimestamp();
}

uint32_t net_CustomRandomCallback(void) {
	return HAL_RNG_GetRandomNumber(net_secure_config->rngHandle);
}

int net_TLSConnect(NetTransportContext *ctx, SocketType sockType,
		const char* host, uint16_t port, uint32_t timeoutMs) {
	NETS_ENTER("net_TLSConnect");

	uint8_t targetIp[4];
	int rc = net_DNSLookup(host, targetIp);
	if (rc != 0) {
		NETS_MSG("net_DNSLookup failed %d\r\n", rc);
		NETS_EXIT("net_TLSConnect", rc, 1);
		return rc;
	}

	rc = net_TLSConnectIp(ctx, sockType, targetIp, port, timeoutMs);
	if (rc != 0) {
		NETS_MSG("net_TLSConnectIp failed %d\r\n", rc);
		NETS_EXIT("net_TLSConnect", rc, 1);
		return rc;
	}

	NETS_EXIT("net_TLSConnect", 0, 0);
	return 0;
}

int net_TLSConnectIp(NetTransportContext *ctx, SocketType sockType,
		uint8_t targetIp[4], uint16_t port, uint32_t timeoutMs) {
	NETS_ENTER("net_TLSConnectIp");

	if ((ctx->sslCtx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
		NETS_MSG("wolfSSL_CTX_new failed\r\n");
		NETS_EXIT("net_TLSConnectIp", -1, 1);
		return -1;
	}

	wolfSSL_SetIORecv(ctx->sslCtx, WolfSSL_IORecvCallback);
	wolfSSL_SetIOSend(ctx->sslCtx, WolfSSL_IOSendCallback);
	wolfSSL_CTX_set_verify(ctx->sslCtx, WOLFSSL_VERIFY_NONE, NULL);

	if ((ctx->ssl = wolfSSL_new(ctx->sslCtx)) == NULL) {
		NETS_MSG("wolfSSL_new failed\r\n");
		NETS_EXIT("net_TLSConnectIp", -2, 1);
		return -2;
	}

	wolfSSL_SetIOReadCtx(ctx->ssl, ctx);
	wolfSSL_SetIOWriteCtx(ctx->ssl, ctx);
	wolfSSL_set_verify(ctx->ssl, WOLFSSL_VERIFY_NONE, NULL);

	int rc = net_ConnectIp(ctx, sockType, targetIp, port, timeoutMs);
	if (rc != RC_SUCCESS) {
		NETS_MSG("net_ConnectIp failed %d\r\n", rc);
		NETS_EXIT("net_TLSConnectIp", rc, 1);
		return rc;
	}

	rc = wolfSSL_connect(ctx->ssl);
	if (rc != WOLFSSL_SUCCESS) {
		NETS_MSG("wolfSSL_connect failed %d\r\n", rc);
		NETS_EXIT("net_TLSConnectIp", rc, 1);
		return rc;
	}

	NETS_EXIT("net_TLSConnectIp", 0, 0);
	return 0;
}

void net_TLSSetHandleClientConnectionCallback(
		NetHandleClientConnectionCallback callback) {
	net_secure_HandleClientConnectionCallback = callback;
	net_SetHandleClientConnectionCallback(
			net_TLSNetHandleClientConnectionCallbackWrapper);
}

int net_TLSNetHandleClientConnectionCallbackWrapper(NetTransportContext *ctx) {
	NETS_ENTER("net_TLSNetHandleClientConnectionCallbackWrapper");
	if ((ctx->ssl = wolfSSL_new(ctx->sslCtx)) == NULL) {
		NETS_MSG("wolfSSL_new failed\r\n");
		NETS_EXIT("net_TLSNetHandleClientConnectionCallbackWrapper", -5, 1);
		return -5;
	}
	wolfSSL_SetIOReadCtx(ctx->ssl, ctx);
	wolfSSL_SetIOWriteCtx(ctx->ssl, ctx);

	int cbrc = net_secure_HandleClientConnectionCallback(ctx);

	wolfSSL_free(ctx->ssl);

	if (cbrc != 0) {
		NETS_MSG("client callback failed %d\r\n", cbrc);
		NETS_EXIT("net_TLSNetHandleClientConnectionCallbackWrapper", cbrc, 1);
		return cbrc;
	}

	NETS_EXIT("net_TLSNetHandleClientConnectionCallbackWrapper", 0, 0);
	return cbrc;
}

int net_TLSStartServerConnection(NetTransportContext *ctx, SocketType sockType,
		uint16_t port) {
	NETS_ENTER("net_TLSStartServerConnection");

	if ((ctx->sslCtx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
		NETS_MSG("wolfSSL_CTX_new failed\r\n");
		NETS_EXIT("net_TLSConnectIp", -1, 1);
		return -1;
	}

	wolfSSL_SetIORecv(ctx->sslCtx, WolfSSL_IORecvCallback);
	wolfSSL_SetIOSend(ctx->sslCtx, WolfSSL_IOSendCallback);

	int rc = wolfSSL_CTX_use_certificate_buffer(ctx->sslCtx, DEVICE_CERT,
			DEVICE_CERT_SIZE, WOLFSSL_FILETYPE_ASN1);
	if (rc != WOLFSSL_SUCCESS) {
		NETS_MSG("wolfSSL_CTX_use_certificate_buffer failed %d\r\n", rc);
		NETS_EXIT("net_TLSStartServerConnection", rc, 1);
		return rc;
	}

	rc = wolfSSL_CTX_use_PrivateKey_buffer(ctx->sslCtx, PRIVATE_KEY,
			PRIVATE_KEY_SIZE, WOLFSSL_FILETYPE_ASN1);
	if (rc != WOLFSSL_SUCCESS) {
		NETS_MSG("wolfSSL_CTX_use_PrivateKey_buffer failed %d\r\n", rc);
		NETS_EXIT("net_TLSStartServerConnection", rc, 1);
		return rc;
	}

	rc = net_StartServerConnection(ctx, sockType, port);
	if (rc != 0) {
		NETS_MSG("net_StartServerConnection failed %d\r\n", rc);
		NETS_EXIT("net_TLSStartServerConnection", rc, 1);
		return rc;
	}

	NETS_EXIT("net_TLSStartServerConnection", 0, 0);
	return 0;
}

int net_TLSStopServerConnection(NetTransportContext *ctx) {
	NETS_ENTER("net_TLSStopServerConnection");
	wolfSSL_free(ctx->ssl);
	wolfSSL_CTX_free(ctx->sslCtx);
	int rc = wolfSSL_Cleanup();
	if (rc != WOLFSSL_SUCCESS) {
		NETS_MSG("wolfSSL_Cleanup failed %d\r\n", rc);
		NETS_EXIT("net_TLSStopServerConnection", rc, 1);
		return rc;

	}
	rc = net_StopServerConnection(ctx);
	if (rc != 0) {
		NETS_MSG("net_StopServerConnection failed %d\r\n", rc);
		NETS_EXIT("net_TLSStopServerConnection", rc, 1);
		return rc;

	}
	NETS_EXIT("net_TLSStopServerConnection", 0, 0);
	return 0;
}

int net_TLSDisconnect(NetTransportContext *ctx) {
	NETS_ENTER("net_TLSDisconnect");
	wolfSSL_free(ctx->ssl);
	wolfSSL_CTX_free(ctx->sslCtx);
	int rc = wolfSSL_Cleanup();
	if (rc != WOLFSSL_SUCCESS) {
		NETS_MSG("wolfSSL_Cleanup failed %d\r\n", rc);
		NETS_EXIT("net_TLSDisconnect", rc, 1);
		return rc;

	}
	rc = net_Disconnect(ctx);
	if (rc != 0) {
		NETS_MSG("net_Disconnect failed %d\r\n", rc);
		NETS_EXIT("net_TLSDisconnect", rc, 1);
		return rc;

	}
	NETS_EXIT("net_TLSDisconnect", 0, 0);
	return 0;
}

int net_TLSSend(NetTransportContext *ctx, const char* buffer,
		const uint32_t bufferSz, uint32_t timeoutMs) {
	NETS_ENTER("net_TLSSend");
	int rc = wolfSSL_write(ctx->ssl, buffer, bufferSz);
	if (rc <= 0) {
		NETS_MSG("wolfSSL_write failed %d\r\n", rc);
		NETS_EXIT("net_TLSSend", rc, 1);
		return rc;
	}
	NETS_EXIT("net_TLSSend", 0, 0);
	return rc;
}

int net_TLSReceive(NetTransportContext *ctx, char* buffer,
		const uint32_t bufferSz, uint32_t timeoutMs) {
	NETS_ENTER("net_TLSReceive");
	int rc = wolfSSL_read(ctx->ssl, (char*) buffer, bufferSz);
	if (rc < 0) {
		NETS_MSG("wolfSSL_read failed %d\r\n", rc);
		NETS_EXIT("net_TLSReceive", rc, 1);
		return rc;
	}
	buffer[rc] = '\0';
	NETS_EXIT("net_TLSReceive", 0, 0);
	return rc;
}

int net_TLSDestroy(NetTransportContext *ctx) {
	NETS_ENTER("net_TLSDestroy");
	int rc;
	if ((rc = net_Destroy(ctx)) != 0) {
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
