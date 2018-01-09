/**
 ******************************************************************************
 * @file    Templates/Src/main.c
 * @author  MCD Application Team
 * @brief   Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#define WOLFMQTT_DEBUG_CLIENT
#define PRINTF printf

#include "main.h"
#include "wolfmqtt/mqtt_client.h"
#include "stdio.h"
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/certs_test.h>
#include "wolfssl/wolfio.h"
#include "wolfssl_net.h"

// 0 = MQTT example, 1 = TLS example
#define EXAMPLE_KIND	1

#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
 set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/** @addtogroup STM32L4xx_HAL_Examples
 * @{
 */

/** @addtogroup Templates
 * @{
 */

/* Private typedef -----------------------------------------------------------*/

typedef struct WolfSocketContext {
	uint32_t id;
} WolfSocketContext;

uint32_t socketId = 0;

/* Private define ------------------------------------------------------------*/
#define SSID     "A66 Guest"
#define PASSWORD "Hello123"
//#define SSID     "AndroidAP"
//#define PASSWORD "Buzi3vagy"

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef uartHandle;
RNG_HandleTypeDef rngHandle;

uint8_t MAC_Addr[6];
uint8_t IP_Addr[4];
word16 packetId = 0;

/**
 * MQTT configuration
 */
#define DEFAULT_TIMEOUT			1000
#define MAX_BUFFER_SIZE         512

MqttClient mqttClient;
MqttNet mqttNetwork;
WolfSocketContext mqttContext = { 0 };

static uint8_t tx_buf[MAX_BUFFER_SIZE];
static uint8_t rx_buf[MAX_BUFFER_SIZE];

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);

/* Private functions ---------------------------------------------------------*/
static void Peripherals_Init(void);
static void UART_Init(void);
static void WIFIP_Init(void);
static void RNG_Init(void);

time_t custom_time(time_t x) {
	printf("Custom time invoked!\r\n");
	return 1515409298l;
}

unsigned int custom_rand_generate(void) {
	printf("Custom RNG invoked!\r\n");
	return HAL_RNG_GetRandomNumber(&rngHandle) >> 24;
}

static int MqttNetConnect(void *context, const char* host, word16 port,
		int timeout_ms) {
	printf("MqttNetConnect() - ");
	WolfSocketContext *ctx = (WolfSocketContext*) context;
	uint8_t destIp[4];

	if (WIFI_GetHostAddress((char*) host, destIp) != WIFI_STATUS_OK) {
		printf("FAIL DNS\r\n");
		return MQTT_CODE_ERROR_BAD_ARG;
	}

	printf("\r\n\tDNS lookup result: %s => %d.%d.%d.%d\r\n...", host, destIp[0],
			destIp[1], destIp[2], destIp[3]);

	if (WIFI_OpenClientConnection(ctx->id, WIFI_TCP_PROTOCOL, "TCP_CLIENT",
			destIp, port, timeout_ms) != WIFI_STATUS_OK) {
		printf("FAIL OPEN\r\n");
		return MQTT_CODE_ERROR_BAD_ARG;
	}

	printf("SUCCESS\r\n");
	return MQTT_CODE_SUCCESS;
}

static int MqttNetWrite(void *context, const byte* buf, int buf_len,
		int timeout_ms) {
	printf("MqttNetWrite() - ");
	WolfSocketContext *ctx = (WolfSocketContext*) context;
	uint16_t sent_len;

	if (WIFI_SendData(ctx->id, (byte*) buf, buf_len, &sent_len, timeout_ms)
			!= WIFI_STATUS_OK) {
		printf("FAIL\r\n");
		return MQTT_CODE_ERROR_BAD_ARG;
	}
	printf("SUCCESS\r\n");
	return buf_len;
}

static int MqttNetRead(void *context, byte* buf, int buf_len, int timeout_ms) {
	printf("MqttNetRead() - ");
	WolfSocketContext *ctx = (WolfSocketContext*) context;
	uint16_t rcpt_len = 0;

	if (WIFI_ReceiveData(ctx->id, buf, buf_len, &rcpt_len, timeout_ms)
			!= WIFI_STATUS_OK) {
		printf("FAIL\r\n");
		return MQTT_CODE_ERROR_BAD_ARG;
	}
	if (rcpt_len == 0) {
		printf("TIMEOUT\r\n");
		return MQTT_CODE_ERROR_TIMEOUT;
	}
	printf("SUCCESS\r\n");
	return rcpt_len;
}

static int MqttNetDisconnect(void *context) {
	printf("MqttNetDisconnect() - ");
	WolfSocketContext *ctx = (WolfSocketContext*) context;
	if (WIFI_CloseClientConnection(ctx->id) != WIFI_STATUS_OK) {
		printf("FAIL\r\n");
		return MQTT_CODE_ERROR_BAD_ARG;
	}
	printf("SUCCESS\r\n");
	return MQTT_CODE_SUCCESS;
}

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


static int mqttclient_message_cb(MqttClient *client, MqttMessage *msg,
		byte msg_new, byte msg_done) {

	byte buf[MAX_BUFFER_SIZE + 1];

	if (msg_new) {
		printf("\r\n\r\n--------- MESSAGE BEGIN ---------\r\n");
		XMEMCPY(buf, msg->topic_name, msg->topic_name_len);
		buf[msg->topic_name_len] = '\0';
		printf("Topic:\t\t%s\r\n", buf);
		printf("Content:\t");
	}

	XMEMCPY(buf, msg->buffer, msg->buffer_len);
	buf[msg->buffer_len] = '\0';

	printf("%s", buf);

	if (msg_done) {
		printf("\r\n---------- MESSAGE END ----------\r\n\r\n");
	}

	return MQTT_CODE_SUCCESS;
}

void wolfSSL_Logging_cb_f(const int logLevel, const char * const logMessage) {
	printf("[%d] - %s\r\n", logLevel, logMessage);
}


static void Wolfmqtt_PublishReceive(const char *host, int port) {
	mqttNetwork.connect = MqttNetConnect;
	mqttNetwork.disconnect = MqttNetDisconnect;
	mqttNetwork.write = MqttNetWrite;
	mqttNetwork.read = MqttNetRead;
	mqttNetwork.context = &mqttContext;

	int rc = 0;

	rc = MqttClient_Init(&mqttClient, &mqttNetwork, mqttclient_message_cb,
			tx_buf, MAX_BUFFER_SIZE, rx_buf, MAX_BUFFER_SIZE, 10000);
	printf("MQTT Init: %s (%d)\r\n", MqttClient_ReturnCodeToString(rc), rc);

	rc = MqttClient_NetConnect(&mqttClient, "iot.eclipse.org", 1883, 10000, 0,
	NULL);
	printf("MQTT NetConnect: %s (%d)\r\n", MqttClient_ReturnCodeToString(rc),
			rc);
	uint32_t connectedMs = HAL_GetTick();

	MqttConnect conn;
	XMEMSET(&conn, 0, sizeof(MqttConnect));

	conn.clean_session = 1;
	conn.keep_alive_sec = 30;
	conn.client_id = "test-client-id-device";
	conn.enable_lwt = 0;

	rc = MqttClient_Connect(&mqttClient, &conn);
	printf("MQTT Connect: %s (%d)\r\n", MqttClient_ReturnCodeToString(rc), rc);
	printf("\tMQTT Connect Ack: Return Code %u, Session Present %d\r\n",
			conn.ack.return_code,
			(conn.ack.flags & MQTT_CONNECT_ACK_FLAG_SESSION_PRESENT) ? 1 : 0);

	MqttSubscribe sub;
	XMEMSET(&sub, 0, sizeof(MqttSubscribe));

	MqttTopic top[] = { { "zksh/devices/first", 0 } };
	sub.topics = top;
	sub.topic_count = 1;

	rc = MqttClient_Subscribe(&mqttClient, &sub);
	printf("MQTT Subscribe: %s (%d)\r\n", MqttClient_ReturnCodeToString(rc),
			rc);

	MqttPublish publish;
	XMEMSET(&publish, 0, sizeof(MqttPublish));

	publish.retain = 0;
	publish.qos = MQTT_QOS_0;
	publish.duplicate = 0;
	publish.topic_name = "zksh/devices/second";
	publish.packet_id = packetId++;
	publish.buffer = (byte*) "devicetest";
	publish.total_len = (word16) XSTRLEN("devicetest");
	rc = MqttClient_Publish(&mqttClient, &publish);

	printf("MQTT Publish: Topic %s, %s (%d)\r\n", publish.topic_name,
			MqttClient_ReturnCodeToString(rc), rc);

	while (1) {
		rc = MqttClient_WaitMessage(&mqttClient, 5000);
		uint32_t connectionAge = HAL_GetTick() - connectedMs;

		MqttClient_Ping(&mqttClient);
		BSP_LED_Toggle(LED_GREEN);
	}
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

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void) {
	Peripherals_Init();

	wolfSSL_Debugging_ON();
	wolfSSL_SetLoggingCb(wolfSSL_Logging_cb_f);

	if (WIFI_Connect(SSID, PASSWORD, WIFI_ECN_WPA2_PSK) != WIFI_STATUS_OK) {
		printf("ERROR: Couldn't connect to WiFi network: %s\r\n", SSID);
	}

	if (WIFI_GetIP_Address(IP_Addr) == WIFI_STATUS_OK) {
		printf("WiFi successfully joined with IP address: %d.%d.%d.%d\r\n",
				IP_Addr[0], IP_Addr[1], IP_Addr[2], IP_Addr[3]);
	}

	if (EXAMPLE_KIND) {
		int res = Wolfssl_TlsConnect("mqtt.googleapis.com", 8883);
		printf("---- TLS RESULT: %d ----\r\n", res);
	} else {
		Wolfmqtt_PublishReceive("mqtt.googleapis.com", 8883);
	}

}

static void Peripherals_Init(void) {
	/* STM32L4xx HAL library initialization:
	 - Configure the Flash prefetch, Flash preread and Buffer caches
	 - Systick timer is configured by default as source of time base, but user
	 can eventually implement his proper time base source (a general purpose
	 timer for example or other time source), keeping in mind that Time base
	 duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
	 handled in milliseconds basis.
	 - Low Level Initialization
	 */
	HAL_Init();

	/* Configure the System clock to have a frequency of 80 MHz */
	SystemClock_Config();

	UART_Init();

	RNG_Init();

	WIFIP_Init();

	BSP_LED_Init(LED_GREEN);
}

static void RNG_Init(void) {

	__HAL_RCC_RNG_CLK_ENABLE()
	;

	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

	/*Select PLLQ output as RNG clock source */
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RNG;
	PeriphClkInitStruct.RngClockSelection = RCC_RNGCLKSOURCE_PLL;
	HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

	rngHandle.Instance = RNG;

	if (HAL_RNG_Init(&rngHandle) != HAL_OK) {
		printf("ERROR: could not configure RNG");
	}
}

static void WIFIP_Init(void) {
	if (WIFI_Init() != WIFI_STATUS_OK) {
		printf("ERROR: Couldn't initiate WiFi\r\n");
		return;
	}

	printf("WiFi successfully initiated\r\n");

	if (WIFI_GetMAC_Address(MAC_Addr) != WIFI_STATUS_OK) {
		printf("ERROR: Couldn't obtain MAC address\r\n");
		return;
	}

	printf("WiFi MAC: %X:%X:%X:%X:%X:%X\r\n", MAC_Addr[0], MAC_Addr[1],
			MAC_Addr[2], MAC_Addr[3], MAC_Addr[4], MAC_Addr[5]);
}

static void UART_Init(void) {
	uartHandle.Init.BaudRate = 115200;
	uartHandle.Init.WordLength = UART_WORDLENGTH_8B;
	uartHandle.Init.StopBits = UART_STOPBITS_1;
	uartHandle.Init.Parity = UART_PARITY_NONE;
	uartHandle.Init.Mode = UART_MODE_TX_RX;

	BSP_COM_Init(COM1, &uartHandle);
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follow :
 *            System Clock source            = PLL (MSI)
 *            SYSCLK(Hz)                     = 80000000
 *            HCLK(Hz)                       = 80000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 1
 *            APB2 Prescaler                 = 1
 *            MSI Frequency(Hz)              = 4000000
 *            PLL_M                          = 1
 *            PLL_N                          = 40
 *            PLL_R                          = 2
 *            PLL_P                          = 7
 *            PLL_Q                          = 4
 *            Flash Latency(WS)              = 4
 * @param  None
 * @retval None
 */
static void SystemClock_Config(void) {
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/* MSI is enabled after System reset, activate PLL with MSI as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
	RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 40;
	RCC_OscInitStruct.PLL.PLLR = 2;
	RCC_OscInitStruct.PLL.PLLP = 7;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		/* Initialization Error */
		while (1)
			;
	}

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	 clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
		/* Initialization Error */
		while (1)
			;
	}
}

PUTCHAR_PROTOTYPE {
	/* Place your implementation of fputc here */
	/* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
	HAL_UART_Transmit(&uartHandle, (uint8_t *) &ch, 1, 0xFFFF);

	return ch;
}

#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(char* file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{
	}
}
#endif

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
