#include "main.h"
#include "wolfmqtt/mqtt_client.h"
#include "stdio.h"
#include "sec_conf.h"

WolfSocketContext mqttContext = { 0 };

int WolfsslReadCallback(WOLFSSL* ssl, char* buf, int sz, void* context) {
	printf("WolfsslReadCallback() - ");
	WolfSocketContext *ctx = (WolfSocketContext*) context;
	int totalSent = 0;
	int sendingSize = sz;
	int remainingDataSize = sz;
	char *sendStartPtr = buf;

	do {
		if (sendingSize > ES_WIFI_PAYLOAD_SIZE) {
			sendingSize = ES_WIFI_PAYLOAD_SIZE;
		}
		int sent = 0;
		int pRc = WIFI_ReceiveData(ctx->id, (byte*) sendStartPtr, sendingSize,
				&sent, DEFAULT_TIMEOUT);
		if (pRc != WIFI_STATUS_OK) {
			printf("FAIL RC: %d\r\n", pRc);
			break;
		}
		totalSent += sent;
		sendStartPtr += sendingSize;
		remainingDataSize -= sendingSize;
		sendingSize = remainingDataSize;

	} while (remainingDataSize > 0);
	printf("SUCCESS\r\n");
	return totalSent;
}

int WolfsslWriteCallback(WOLFSSL* ssl, char* buf, int sz, void* context) {
	printf("WolfsslWriteCallback() - ");
	WolfSocketContext *ctx = (WolfSocketContext*) context;

	int totalSent = 0;
	int sendingSize = sz;
	int remainingDataSize = sz;
	char *sendStartPtr = buf;

	do {
		if (sendingSize > ES_WIFI_PAYLOAD_SIZE) {
			sendingSize = ES_WIFI_PAYLOAD_SIZE;
		}
		int sent = 0;
		int pRc = WIFI_SendData(ctx->id, (byte*) sendStartPtr, sendingSize,
				&sent, DEFAULT_TIMEOUT);
		if (pRc != WIFI_STATUS_OK) {
			printf("FAIL RC: %d\r\n", pRc);
			break;
		}
		totalSent += sent;
		sendStartPtr += sendingSize;
		remainingDataSize -= sendingSize;
		sendingSize = remainingDataSize;

	} while (remainingDataSize > 0);
	printf("SUCCESS\r\n");
	return totalSent;
}

void wolfSSL_Logging_cb_f(const int logLevel, const char * const logMessage) {
	printf("[%d] - %s\r\n", logLevel, logMessage);
}

static int Wolfssl_TlsConnect(const char *host, int port) {
	WOLFSSL *ssl;
	WOLFSSL_CTX *ctx;

	wolfSSL_Init();

	if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
		return -1;
	}

	wolfSSL_SetIORecv(ctx, WolfsslReadCallback);
	wolfSSL_SetIOSend(ctx, WolfsslWriteCallback);

	int rc;
	if ((rc = wolfSSL_CTX_load_verify_buffer(ctx, PUBLIC_KEY, PUBLIC_KEY_SIZE,
			WOLFSSL_FILETYPE_ASN1)) != WOLFSSL_SUCCESS) {
		return rc;
	}

	if ((ssl = wolfSSL_new(ctx)) == NULL) {
		return -3;
	}

	wolfSSL_SetIOReadCtx(ssl, &mqttContext);
	wolfSSL_SetIOWriteCtx(ssl, &mqttContext);

	wolfSSL_set_verify(ssl, WOLFSSL_VERIFY_NONE, NULL);
	wolfSSL_CTX_set_verify(ssl, WOLFSSL_VERIFY_NONE, NULL);

	uint8_t destIp[4];
	if (WIFI_GetHostAddress((char*) host, destIp) != WIFI_STATUS_OK) {
		printf("FAIL DNS\r\n");
		return -4;
	}

	if (WIFI_OpenClientConnection(mqttContext.id, WIFI_TCP_PROTOCOL,
			"TCP_CLIENT", destIp, port, DEFAULT_TIMEOUT) != WIFI_STATUS_OK) {
		return -5;
	}

	int resCode = wolfSSL_connect(ssl);
	printf("wolfSSL_connect() - RS: %d\r\n", resCode);
	if (resCode != SSL_SUCCESS) {
		return -6;
	}

	wolfSSL_free(ssl); /* Free the wolfSSL object                  */
	wolfSSL_CTX_free(ctx); /* Free the wolfSSL context object          */
	wolfSSL_Cleanup(); /* Cleanup the wolfSSL environment          */
	WIFI_CloseClientConnection(mqttContext.id); /* Close the connection to the server       */

	return 0;
}


