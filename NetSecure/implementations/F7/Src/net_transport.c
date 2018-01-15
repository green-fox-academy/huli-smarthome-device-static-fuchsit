#include "app_ethernet.h"
#include "ethernetif.h"
#include "user_settings.h"
#include "net_transport.h"
#include "smarthome_log.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/netbuf.h"
#include "lwip/ip4.h"
#include "lwip/api.h"

#if defined(STM32L475xx)
#include "stm32l4xx_hal.h"
#elif defined(STM32F746xx)
#include "stm32f7xx_hal.h"
#endif

#define MAX_DATA_CHUNK_SIZE			1400

#define NET_ENTER(fnc)				SHOME_LogEnter("net", fnc)
#define NET_MSG(fnc, ...)			SHOME_LogMsg(fnc, ##__VA_ARGS__)
#define NET_EXIT(fnc, rc, fail)		SHOME_LogExit("net", fnc, rc, fail)

typedef int (*NetReadWriteCb)(struct netconn *conn, const char* buffer,
		const uint32_t bufferSz, uint16_t *processed, uint32_t timeoutMs);

int net_InnerSendUnsafe(struct netconn *conn, const char* buffer,
		const uint32_t bufferSz, uint16_t *sent, uint32_t timeoutMs);
int net_InnerReceiveUnsafe(struct netconn *conn, const char* buffer,
		const uint32_t bufferSz, uint16_t *received, uint32_t timeoutMs);

static void net_LwIPNetifConfig(void);

typedef enum ServerConnectionStatus {
	SC_INIT, SC_CONNECTED
} ServerConnectionStatus;

NetHandleClientConnectionCallback net_HandleClientCallback;
NetTransportContext *net_context;

int net_Init(NetTransportContext *ctx) {
	NET_ENTER("net_Init");
	net_context = ctx;

	/* Create tcp_ip stack thread */
	tcpip_init(NULL, NULL);

	net_LwIPNetifConfig();

	if (netif_is_up(&net_context->connection.netInterface)) {
		/* Update DHCP state machine */
		DHCP_NetifUp();
	} else {
		/* Update DHCP state machine */
		DHCP_NetifDown();
		NET_MSG("ERROR: the network cable is not connected\r\n");
		NET_EXIT("net_Init", -1, 1);
		return -1;
	}

	/* Start DHCPClient */
	osThreadDef(DHCP, DHCP_thread, osPriorityBelowNormal, 0,
			configMINIMAL_STACK_SIZE * 2);
	osThreadCreate(osThread(DHCP), &net_context->connection.netInterface);

	return RC_SUCCESS;
}

/**
 * @brief  Initializes the lwIP stack
 * @param  None
 * @retval None
 */
static void net_LwIPNetifConfig(void) {
	ip_addr_t ipaddr;
	ip_addr_t netmask;
	ip_addr_t gw;

	ip_addr_set_zero_ip4(&ipaddr);
	ip_addr_set_zero_ip4(&netmask);
	ip_addr_set_zero_ip4(&gw);

	struct netif *gnetif = &net_context->connection.netInterface;

	netif_add(gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init,
			&tcpip_input);

	/*  Registers the default network interface. */
	netif_set_default(gnetif);

	if (netif_is_link_up(gnetif)) {
		/* When the netif is fully configured this function must be called.*/
		netif_set_up(gnetif);
	} else {
		/* When the netif link is down this function must be called */
		netif_set_down(gnetif);
	}
}

int net_DNSLookup(const char* host, uint8_t* resultIp) {
	NET_ENTER("net_DNSLookup");

	ip_addr_t res;
	int rc = netconn_gethostbyname(host, &res);
	if (rc != ERR_OK) {
		NET_MSG("DNS lookup failed!\r\n");
		NET_EXIT("net_DNSLookup", rc, 1);
		return rc;
	}

	resultIp[0] = ip4_addr1(&res);
	resultIp[1] = ip4_addr2(&res);
	resultIp[2] = ip4_addr3(&res);
	resultIp[3] = ip4_addr4(&res);

	NET_EXIT("net_DNSLookup", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_Connect(NetTransportContext *ctx, SocketType sockType, const char* host,
		uint16_t port, uint32_t timeoutMs) {
	NET_ENTER("net_Connect");

	uint8_t destIp[4];
	int rc;
	if ((rc = net_DNSLookup(host, destIp)) != ERR_OK) {
		NET_MSG("DNS lookup failed!\r\n");
		NET_EXIT("net_Connect", rc, 1);
		return rc;
	}

	NET_MSG("DNS lookup result: %s => %d.%d.%d.%d\r\n", host, destIp[0],
			destIp[1], destIp[2], destIp[3]);

	rc = net_ConnectIp(ctx, sockType, destIp, port, timeoutMs);
	if (rc != ERR_OK) {
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

	enum netconn_type protocol =
			sockType == SOCKET_UDP ? NETCONN_UDP : NETCONN_TCP;

	ctx->connection.conn = netconn_new(protocol);
	if (ctx->connection.conn == NULL) {
		NET_MSG("Could not create connection - netconn_new\r\n");
		NET_EXIT("net_ConnectIp", -1, 1);
		return -1;
	}

	ip_addr_t addr;
	IP4_ADDR(&addr, targetIp[0], targetIp[1], targetIp[2], targetIp[3]);

	int rc = netconn_connect(ctx->connection.conn, &addr, port);
	if (rc != ERR_OK) {
		NET_MSG("Could not connect - netconn_connect\r\n");
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

int net_StartServerConnection(NetTransportContext *ctx, SocketType sockType,
		uint16_t port) {
	NET_ENTER("net_StartServerConnection");

	enum netconn_type protocol =
			sockType == SOCKET_UDP ? NETCONN_UDP : NETCONN_TCP;

	ctx->connection.conn = netconn_new(protocol);
	if (ctx->connection.conn == NULL) {
		NET_MSG("Could not create connection - netconn_new\r\n");
		NET_EXIT("net_StartServerConnection", -1, 1);
		return -1;
	}

	int rc = netconn_bind(ctx->connection.conn, NULL, port);
	if (rc != ERR_OK) {
		NET_MSG("Could not create connection - netconn_new\r\n");
		NET_EXIT("net_StartServerConnection", rc, 1);
		return -1;
	}

	netconn_listen(ctx->connection.conn);
	while (1) {
		int cliRc = netconn_accept(ctx->connection.conn,
				&ctx->connection.clientConn);
		if (cliRc != ERR_OK) {
			NET_MSG("ERROR: Could not establish client connection: %d\r\n",
					cliRc);
		}

		int cbrc = net_HandleClientCallback(ctx);
		netconn_delete(ctx->connection.clientConn);
		if (cbrc < 0) {
			NET_MSG("Callback returned %d -> ending server process\r\n", cbrc);
			net_StopServerConnection(ctx);
			NET_EXIT("net_StartServerConnection", RC_SUCCESS, 0);
			return 0;
		}
	}
	NET_EXIT("net_StartServerConnection", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_StopServerConnection(NetTransportContext *ctx) {
	NET_EXIT("net_StopServerConnection", RC_SUCCESS, 0);

	int rc = netconn_delete(ctx->connection.conn);

	if (rc != ERR_OK) {
		NET_MSG("netconn_delete failed %d\r\n", rc);
		NET_EXIT("net_StopServerConnection", rc, 1);
		return rc;
	}
	NET_EXIT("net_StopServerConnection", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_Disconnect(NetTransportContext *ctx) {
	NET_ENTER("net_Disconnect");
	int rc = netconn_disconnect(ctx->connection.conn);
	if (rc != ERR_OK) {
		NET_MSG("Could not close connection\r\n");
		NET_EXIT("net_Disconnect", rc, 1);
	}
	NET_EXIT("net_Disconnect", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_ProcessRecoursively(struct netconn* conn, const char *buffer,
		uint32_t bufferSz, uint16_t *totalSent, NetReadWriteCb callback,
		uint32_t timeoutMs, uint32_t spentTime) {
	NET_ENTER("net_ProcessRecoursively");

	if (spentTime > timeoutMs) {
		NET_MSG("ERROR: no more data, so timeout\r\n");
		NET_EXIT("net_ProcessRecoursively", RC_TIMEOUT, 1);
		return RC_TIMEOUT;
	}
	uint32_t iterationStart = HAL_GetTick();

	uint32_t remaining = bufferSz - *totalSent;
	uint32_t toSend = MAX_DATA_CHUNK_SIZE < remaining ? MAX_DATA_CHUNK_SIZE : remaining;
	uint16_t currentProcessed = 0;
	char *remBufferPtr = (char*) buffer + *totalSent;
	int rc = callback(conn, remBufferPtr, toSend, &currentProcessed, timeoutMs);
	if (rc != ERR_OK) {
		NET_EXIT("net_ProcessRecoursively", rc, 1);
		return rc;
	}

	spentTime += (HAL_GetTick() - iterationStart);

	*totalSent += currentProcessed;
	if (*totalSent == bufferSz) {
		NET_EXIT("net_ProcessRecoursively", RC_SUCCESS, 0);
		return RC_SUCCESS;
	}

	rc = net_ProcessRecoursively(conn, buffer, bufferSz, totalSent, callback,
			timeoutMs, spentTime);

	if (rc != ERR_OK) {
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
	int rc = net_ProcessRecoursively(ctx->connection.conn, buffer, bufferSz,
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
	int rc = net_ProcessRecoursively(ctx->connection.conn, buffer, bufferSz,
			&totalReceived, net_InnerReceiveUnsafe, timeoutMs, 0);
	if (rc == RC_SUCCESS) {
		NET_EXIT("net_Receive", RC_SUCCESS, 0);
		return totalReceived;
	}
	if (rc == RC_TIMEOUT && totalReceived >= 0) {
		NET_MSG("WARNING: timeout, data size in read buffer: %d\r\n",
				totalReceived);
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

int net_InnerSendUnsafe(struct netconn *conn, const char* buffer,
		const uint32_t bufferSz, uint16_t *sent, uint32_t timeoutMs) {
	NET_ENTER("net_InnerSendUnsafe");

	struct netbuf *netbuf = netbuf_new();
	int rc = netbuf_ref(netbuf, (const void*) buffer, bufferSz);
	if (rc != ERR_OK) {
		NET_MSG("ERROR: could not create netbuf rc=%d\r\n", rc);
		NET_EXIT("net_Send", rc, 1);
		return rc;
	}

	rc = netconn_send(conn, netbuf);
	if (rc) {
		NET_EXIT("net_InnerSendUnsafe", rc, 1);
		return rc;
	}
	*sent = bufferSz;
	NET_EXIT("net_InnerSendUnsafe", RC_SUCCESS, 0);
	return RC_SUCCESS;
}

int net_InnerReceiveUnsafe(struct netconn *conn, const char* buffer,
		const uint32_t bufferSz, uint16_t *received, uint32_t timeoutMs) {
	NET_ENTER("net_InnerReceiveUnsafe");

	struct netbuf *netbuf = netbuf_new();
	int rc = netbuf_ref(netbuf, (const void*) buffer, bufferSz);
	if (rc != ERR_OK) {
		NET_MSG("ERROR: could not create netbuf rc=%d\r\n", rc);
		NET_EXIT("net_Send", rc, 1);
		return rc;
	}

	rc = netconn_recv(conn, &netbuf);
	if (rc) {
		NET_EXIT("net_InnerReceiveUnsafe", rc, 1);
		return rc;
	}
	*received = netbuf->p->tot_len;
	NET_EXIT("net_InnerReceiveUnsafe", RC_SUCCESS, 0);
	return RC_SUCCESS;
}
