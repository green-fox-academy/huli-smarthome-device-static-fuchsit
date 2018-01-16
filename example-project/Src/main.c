/**
 ******************************************************************************
m * @file    Templates/Src/main.c
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
#include "main.h"
#include "wolfssl/ssl.h"
#include "wolfssl/wolfcrypt/logging.h"
#include "wifi.h"
#include "net_transport.h"
#include "mqtt_net.h"
#include "net_secure.h"
#include "google_iot.h"
#include "ntp_client.h"
#include "rtc_utils.h"
#include "jsmn.h"

#define EXAMPLE_KIND	EXAMPLE_SIMPLE_MQTT
#define NEED_WIFI		1

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

uint32_t socketId = 0;
const char *STATE_PATTERN = "{\"ledOn\": %d}";

typedef enum ExampleKind {
	EXAMPLE_SIMPLE_MQTT, EXAMPLE_MQTT, EXAMPLE_HTTPS_REST,
} ExampleKind;

/* Private define ------------------------------------------------------------*/
#define SSID     "A66 Guest"
#define PASSWORD "Hello123"

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef uartHandle;
RNG_HandleTypeDef rngHandle;
GGL_InitDef gglConfig;
NetSecure_InitTypeDef secConfig;
MQTT_NetInitTypeDef mqttConfig;

uint8_t MAC_Addr[6];
uint8_t IP_Addr[4];


static const char *JSON_STRING =
	"{\"device\": \"johndoe\", \"id\": false, \"ip\": 1000,\n  "
	"}";

extern MqttClient mqttClient;

NetTransportContext netContext;

volatile int tryConnect = 0;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);

/* Private functions ---------------------------------------------------------*/
static void Peripherals_Init(void);
static void SW_STACK_Init(void);
static void UART_Init(void);
static void RNG_Init(void);
static void WIFI_GoOnline(void);

int MQTT_HandleMessageCallback(const char* topic, const char* message) {
	printf("Message arrived in topic: %s\r\nMessage:%s\r\n", topic, message);
	return 0;
}

int HandleClientCallback(NetTransportContext *ctx) {
	char buff[512];

	int rc = net_TLSReceive(ctx, buff, 512, 2000);
	if (rc < 0) {
		printf("ERROR: could not receive data: %d\r\n", rc);
		return 0;
	}
	char snd[] =
			"HTTP/1.0 200 OK\r\nContent-Type: \"application/json\"\r\n\r\n{\"test_key\":\"test_value\"}";
	if ((rc = net_TLSSend(ctx, snd, strlen(snd), 2000)) < 0) {
		printf("ERROR: could not send response: %d\r\n", rc);
		return 0;
	}
	return 0;
}

void HTTPSServerStart() {
	net_TLSSetHandleClientConnectionCallback(HandleClientCallback);
	int rc = net_TLSStartServerConnection(&netContext, SOCKET_TCP, 443);
	if (rc != 0) {
		printf("ERROR: net_TLSStartServerConnection: %d\r\n", rc);
		return;
	}
}

static void Wolfmqtt_PublishReceive(const char *host, int port) {
	int rc;
	if ((rc = GGL_MQTT_Connect()) != RC_SUCCESS) {
		printf("ERROR: GGL_MQTT_Connect FAILED %d - %s\r\n", rc,
				MqttClient_ReturnCodeToString(rc));
		return;
	}
	if ((rc = GGL_MQTT_Publish("events/report", "{\"state\": \"off\"}"))
			!= RC_SUCCESS) {
		printf("ERROR: GGL_MQTT_Publish FAILED %d - %s\r\n", rc,
				MqttClient_ReturnCodeToString(rc));
		return;
	}
	if ((rc = GGL_MQTT_Subscribe("config", MQTT_QOS_1)) != RC_SUCCESS) {
		printf("ERROR: GGL_MQTT_Subscribe FAILED %d - %s\r\n", rc,
				MqttClient_ReturnCodeToString(rc));
		return;
	}
	while (1) {
		if ((rc = GGL_MQTT_WaitForMessage(10000)) != RC_SUCCESS) {
			printf("ERROR: GGL_MQTT_WaitForMessage FAILED %d - %s\r\n", rc,
					MqttClient_ReturnCodeToString(rc));
		}
		if ((rc = GGL_MQTT_Ping()) != RC_SUCCESS) {
			printf("ERROR: GGL_MQTT_Ping FAILED %d - %s\r\n", rc,
					MqttClient_ReturnCodeToString(rc));
		}
	}

	GGL_MQTT_Disconnect();
}

static void SimpleMQTT_Example() {
	int rc = MqttClient_NetConnect(&mqttClient, "iot.eclipse.org", 1883, 2000,
			0, NULL);
	if (rc != MQTT_CODE_SUCCESS) {
		printf("ERROR MqttClient_NetConnect rc=%d\r\n", rc);
		return;
	}

	MqttConnect connect;
	connect.client_id = "test cli ID";
	connect.clean_session = 1;
	connect.keep_alive_sec = 30;
	rc = MqttClient_Connect(&mqttClient, &connect);
	if (rc != MQTT_CODE_SUCCESS || connect.ack.return_code != 0) {
		printf("ERROR MqttClient_Connect rc=%d, ack rc=%d\r\n", rc,
				connect.ack.return_code);
		return;
	}

	MqttPublish pub;
	XMEMSET(&pub, 0, sizeof(MqttPublish));
	pub.buffer = (byte*)"Test message";
	pub.buffer_len = strlen((char*)pub.buffer);
	pub.topic_name = "ptopic";
	pub.qos = MQTT_QOS_0;

	rc = MqttClient_Publish(&mqttClient, &pub);
	if (rc != MQTT_CODE_SUCCESS || connect.ack.return_code != 0) {
		printf("ERROR MqttClient_Publish rc=%d\r\n", rc);
		return;
	}

	MqttTopic top = { "stopic", MQTT_QOS_1 };

	MqttSubscribe sub;
	sub.topic_count = 1;
	sub.topics = &top;
	rc = MqttClient_Subscribe(&mqttClient, &sub);
	if (rc != MQTT_CODE_SUCCESS || connect.ack.return_code != 0) {
		printf("ERROR MqttClient_Subscribe rc=%d\r\n", rc);
		return;
	}

	while (1) {
		MqttClient_WaitMessage(&mqttClient, 10000);
		MqttClient_Ping(&mqttClient);
	}
}

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void) {
	Peripherals_Init();

	SW_STACK_Init();

	WIFI_GoOnline();

	conf_t conf;

	printf("hello\n");
	parse_JSON(&conf, JSON_STRING);
	printf("conf dev: %s\n", conf.device);

	switch (EXAMPLE_KIND) {
	case EXAMPLE_HTTPS_REST:
		HTTPSServerStart();
		break;
	case EXAMPLE_MQTT:
		Wolfmqtt_PublishReceive("mqtt.googleapis.com", 8883);
		break;
	case EXAMPLE_SIMPLE_MQTT:
		SimpleMQTT_Example();
		break;
	}
}

static void WIFI_GoOnline(void) {
	uint8_t online = 0;
	uint32_t tryCount = 1;
	tryConnect = 1;
	while (online == 0) {
		if (!tryConnect) {
			continue;
		}
		printf("Trying to connect to AP %s\r\n", SSID);
		if (WIFI_Connect(SSID, PASSWORD, WIFI_ECN_WPA2_PSK) != WIFI_STATUS_OK) {
			printf(
					"ERROR: Couldn't connect to WiFi network %s. Try count: %lu\r\n",
					SSID, tryCount);
		} else {
			online = 1;
		}
		tryConnect = 0;
		tryCount++;
	}

	if (WIFI_GetIP_Address(IP_Addr) == WIFI_STATUS_OK) {
		printf("WiFi successfully joined with IP address: %d.%d.%d.%d\r\n",
				IP_Addr[0], IP_Addr[1], IP_Addr[2], IP_Addr[3]);
	}

	tryConnect = 1;
	int rtcSet = 0;
	uint32_t rtcCount = 1;
	while (rtcSet == 0) {
		if (!tryConnect) {
			continue;
		}
		printf("Trying to set the RTC clock from NTP\r\n");
		uint32_t ntpResult = 0;
		if (NTPClient_GetTimeSeconds(&ntpResult) != 0) {
			printf("ERROR: could not query NTP time\r\n");
		} else {
			RTCUtils_SetEpochTimestamp(ntpResult);
			rtcSet = 1;
		}
		tryConnect = 0;
		rtcCount++;
	}

}

static void SW_STACK_Init() {
	// enable debug messages
	SHOME_DebugEnable();

	// initialize network layer (WiFi in our case)
	netContext.connection.id = 0;
	net_Init(&netContext);

	printf("before net secure init\n");

	secConfig.rngHandle = &rngHandle;
	secConfig.debugEnable = 1;
	net_SecureInit(&secConfig);

	printf("before mqtt init\n");

	mqttConfig.callback = MQTT_HandleMessageCallback;
	mqttConfig.ctx.id = 0;
	MQTT_Init(&mqttConfig);

	// initialize google stack
	GGL_DeviceDef device;
	device.deviceId = "test-iot-device-2";
	device.deviceRegistry = "greenfox-device-registry";
	device.projectId = "static-aventurin-fuchsit";
	device.region = "europe-west1";

	GGL_NetworkDef network;
	network.mqttHost = "mqtt.googleapis.com";
	network.mqttPort = 8883;

	gglConfig.callback = MQTT_HandleMessageCallback;
	gglConfig.device = device;
	gglConfig.network = network;
	GGL_IOT_Init(&gglConfig);

	NTPClient_Init("hu.pool.ntp.org", 123);
	printf("was here\n");
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

	BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
	BSP_LED_Init(LED_GREEN);

	// initialize real time clock peripheral
	RTCUtils_RTCInit();
}

void EXTI15_10_IRQHandler() {
	HAL_GPIO_EXTI_IRQHandler(USER_BUTTON_PIN);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	tryConnect = 1;
}

static void RNG_Init(void) {

	__HAL_RCC_RNG_CLK_ENABLE();

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
