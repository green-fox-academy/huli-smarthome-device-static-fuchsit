#include <stdarg.h>
#include "user_settings.h"
#include "ntp_client.h"
#include "net_transport.h"
#include "smarthome_log.h"

#define NTP_ENTER(fnc)				SHOME_LogEnter("ntp", fnc)
#define NTP_MSG(fnc, ...)			SHOME_LogMsg(fnc, ##__VA_ARGS__)
#define NTP_EXIT(fnc, rc, fail)		SHOME_LogExit("ntp", fnc, rc, fail)

#define NTP_DEFAULT_TIMEOUT	5000
#define NTP_TIMESTAMP_EPOCH_DELTA	2208988800L

static char* ntp_client_NTPServerHost;
static uint16_t ntp_client_NTPServerPort;

uint32_t NTPClient_ChangeEndian(uint32_t networkEndian);

int NTPClient_GetTimeSeconds(uint32_t *result) {
	NTP_ENTER("NTPClient_GetTimeSeconds");
	ntp_packet packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	packet.li_vn_mode = 0x1b; // 00,011,011 for li = 0, vn = 3, and mode = 3

	NetTransportContext ctx = { 1 };

	int rc = net_Connect(&ctx, SOCKET_UDP, ntp_client_NTPServerHost,
			ntp_client_NTPServerPort, NTP_DEFAULT_TIMEOUT);
	if (rc != 0) {
		NTP_MSG("NTP connect error");
		NTP_EXIT("NTPClient_GetTimeSeconds", rc, 1);
		return rc;
	}

	rc = net_Send(&ctx, (char*) &packet, sizeof(ntp_packet),
			NTP_DEFAULT_TIMEOUT);
	if (rc < 0) {
		NTP_MSG("NTP send request error");
		NTP_EXIT("NTPClient_GetTimeSeconds", rc, 1);
		return rc;
	}

	rc = net_Receive(&ctx, (char*) &packet, sizeof(ntp_packet),
			NTP_DEFAULT_TIMEOUT);
	if (rc < 0) {
		NTP_MSG("NTP response receive error");
		NTP_EXIT("NTPClient_GetTimeSeconds", rc, 1);
		return rc;
	}

	rc = net_Disconnect(&ctx);
	if (rc < 0) {
		NTP_MSG("NTP disconnect error");
		NTP_EXIT("NTPClient_GetTimeSeconds", rc, 1);
		return rc;
	}

	*result = NTPClient_ChangeEndian(packet.rxTm_s) - NTP_TIMESTAMP_EPOCH_DELTA;
	NTP_EXIT("NTPClient_GetTimeSeconds", 0, 0);
	return 0;
}

void NTPClient_Init(const char* ntpHost, uint16_t ntpPort) {
	NTP_ENTER("NTPClient_Init");
	ntp_client_NTPServerHost = (char*) ntpHost;
	ntp_client_NTPServerPort = ntpPort;
	NTP_EXIT("NTPClient_Init", 0, 0);
}

uint32_t NTPClient_ChangeEndian(uint32_t networkEndian) {
	uint32_t res = (networkEndian >> 24);
	res |= (networkEndian & 0xFF0000) >> 8;
	res |= (networkEndian & 0xFF00) << 8;
	res |= (networkEndian & 0xFF) << 24;
	return res;
}

