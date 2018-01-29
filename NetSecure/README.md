# NetSecure library

The motivation was to create a unified interface over TCP/IP stack-specific implementations for basic network tasks and hide the complexity of using **WolfSSL** for secure communication.

The **net_transport.h** should be implemented on each platform. For example on the STM32L4 IoT Discovery board we use the esWIFI module's AC SPI interface for handling the network, on the F7 board we're using LwIP.

The header file contains all the specs, which is necessary to implement the functions. You can find the implementation for the 2 boards in the board projects.

# Usage

1. You need to create your WolfSSL configuration with the following name: **user_settings.h**
2. You need to create the ```NetConnectionContext``` structure, which contains the platform-specific network informations, which are necessary for your network layer implementation and setup the SSL keys in **net_settings.h** file
3. You need to implement the **net_transport.h** file in your project for your platform.

For L4 and F7 boards you can use these:

| Board | Directory |
|-------|-----------|
| L4 | [Using WiFi module](implementations/L4) |
| F7 | [Using LwIP](implementations/F7) |

## HTTP REST service
```c
#include "net_transport.h"

#define NET_TIMEOUT 2000


int HandleClientConnectionCallback(NetTransportContext *ctx) {
    char request[512];
	
    int rc = net_Receive(ctx, buff, 512, NET_TIMEOUT);
    if (rc < 0) {
        printf("ERROR: could not receive data: %d\r\n", rc);
        return 0;
    }

    char response[] = "HTTP/1.0 200 OK\r\nContent-Type: \"application/json\"\r\n\r\n{\"test_key\":\"test_value\"}";
    if ((rc = net_Send(ctx, response, strlen(response), NET_TIMEOUT)) < 0) {
        printf("ERROR: could not send response: %d\r\n", rc);
        return 0;
    }
    return 0;
}

void main() {
    NetTransportContext netContext;
    netContext.connection.id = 0;
    net_Init(&netContext);

    net_SetHandleClientConnectionCallback(HandleClientConnectionCallback);
    int rc = net_StartServerConnection(&netContext, SOCKET_TCP, 80);
    if (rc != 0) {
        printf("ERROR: net_StartServerConnection: %d\r\n", rc);
        return;
    }
}

```

## TLS REST service

```c
#include "net_secure.h"

#define NET_TIMEOUT 2000


int HandleClientConnectionCallback(NetTransportContext *ctx) {
    char request[512];
	
    int rc = net_TLSReceive(ctx, buff, 512, NET_TIMEOUT);
    if (rc < 0) {
        printf("ERROR: could not receive data: %d\r\n", rc);
        return 0;
    }

    char response[] = "HTTP/1.0 200 OK\r\nContent-Type: \"application/json\"\r\n\r\n{\"test_key\":\"test_value\"}";
    if ((rc = net_TLSSend(ctx, response, strlen(response), NET_TIMEOUT)) < 0) {
        printf("ERROR: could not send response: %d\r\n", rc);
        return 0;
    }
    return 0;
}

void main() {
    NetTransportContext netContext;
    netContext.connection.id = 0;
    net_Init(&netContext);

    net_TLSSetHandleClientConnectionCallback(HandleClientConnectionCallback);
    int rc = net_TLSStartServerConnection(&netContext, SOCKET_TCP, 443);
    if (rc != 0) {
        printf("ERROR: net_TLSStartServerConnection: %d\r\n", rc);
        return;
    }
}

```
