# NTP client

This is a simple library, which is able to connect to an NTP (network time protocol) server and extract the current UNIX timestamp. The motivation behind this library is that we need to know the actual time on the board to be able to connect to Google IoT Core API and MQTT broker.

# Usage

```c
#include "ntp_client.h"

void main() {
    NTPClient_Init("hu.ntp.pool.org", 123);
    uint32_t timestamp;
    int rc = NTPClient_GetTimeSeconds(&timestamp);
    if (rc != 0) {
        printf("ERROR: NTP request failed with rc=%d\r\n", rc);
        return;
    }
    printf("UNIX timestamp from NTP server: %lu\r\n", timestamp);
}

```