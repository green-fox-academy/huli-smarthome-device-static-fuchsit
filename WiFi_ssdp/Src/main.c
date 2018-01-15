/**
  ******************************************************************************
  * @file    Wifi/WiFi_ssdp/src/main.c
  * @author  MCD Application Team
  * @brief   This file provides main program functions
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright © 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "wifi.h"

/* Private defines -----------------------------------------------------------*/
/* Update SSID and PASSWORD with own Access point settings */
#define SSID     "A66 Guest"
#define PASSWORD "Hello123"

#define WIFI_WRITE_TIMEOUT 10000
#define WIFI_READ_TIMEOUT  10000


#define PORT           1900
/* Private typedef------------------------------------------------------------*/
typedef enum
{
  WS_IDLE = 0,
  WS_CONNECTED,
  WS_DISCONNECTED,
  WS_ERROR,
} WebServerState_t;

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern UART_HandleTypeDef hDiscoUart;
static   uint8_t http[1024];
static   uint8_t resp[1024];
uint16_t respLen;
uint8_t  IP_Addr[4];
uint8_t  MAC_Addr[6];
int32_t Socket = -1;
static   WebServerState_t  State = WS_ERROR;
char     ModuleName[32];

/* Private function prototypes -----------------------------------------------*/

#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */


static void SystemClock_Config(void);
static WIFI_Status_t SendWebPage(uint8_t ledIsOn);
static void WebServerProcess(void);

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure LEDRED */
  BSP_LED_Init(LED2);

  	  __HAL_RCC_GPIOA_CLK_ENABLE();    // Enable the GPIOA port's clock first

     GPIO_InitTypeDef LEDRED;            // create a config structure
     LEDRED.Pin = GPIO_PIN_0;             // this is about PIN 0
     LEDRED.Mode = GPIO_MODE_OUTPUT_PP;  // Configure as output with push-up-down enabled
     LEDRED.Pull = GPIO_PULLDOWN;        // the push-up-down should work as pulldown
     LEDRED.Speed = GPIO_SPEED_HIGH;     // we need a high-speed output

     HAL_GPIO_Init(GPIOA, &LEDRED);

  /* WIFI Web Server demonstration */

  /* Initialize all configured peripherals */
  hDiscoUart.Instance = DISCOVERY_COM1;
  hDiscoUart.Init.BaudRate = 115200;
  hDiscoUart.Init.WordLength = UART_WORDLENGTH_8B;
  hDiscoUart.Init.StopBits = UART_STOPBITS_1;
  hDiscoUart.Init.Parity = UART_PARITY_NONE;
  hDiscoUart.Init.Mode = UART_MODE_TX_RX;
  hDiscoUart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hDiscoUart.Init.OverSampling = UART_OVERSAMPLING_16;
  hDiscoUart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hDiscoUart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  BSP_COM_Init(COM1, &hDiscoUart);
  printf("****** WiFi ssdp demonstration****** \n\n");



  /*Initialize and use WIFI module */
  if(WIFI_Init() ==  WIFI_STATUS_OK)
  {

  printf("ES-WIFI Initialized.\n");


      if(WIFI_GetMAC_Address(MAC_Addr) == WIFI_STATUS_OK)
      {

        printf("> es-wifi module MAC Address : %X:%X:%X:%X:%X:%X\n",
               MAC_Addr[0],
               MAC_Addr[1],
               MAC_Addr[2],
               MAC_Addr[3],
               MAC_Addr[4],
               MAC_Addr[5]);

      }
      else
      {

        printf("> ERROR : CANNOT get MAC address\n");

      }

   if( WIFI_Connect(SSID, PASSWORD, WIFI_ECN_WPA2_PSK) == WIFI_STATUS_OK)
    {

      printf("> es-wifi module connected \n");


      if(WIFI_GetIP_Address(IP_Addr) == WIFI_STATUS_OK)
      {

      printf("> es-wifi module got IP Address : %d.%d.%d.%d\n",
               IP_Addr[0],
               IP_Addr[1],
               IP_Addr[2],
               IP_Addr[3]);

      printf(">Start HTTP Server... \n");
      printf(">Wait for connection...  \n");
      BSP_LED_On(LED2);

        State = WS_IDLE;

      }
      else
      {

    printf("> ERROR : es-wifi module CANNOT get IP address\n");

      }
    }
    else
    {


 printf("> ERROR : es-wifi module NOT connected\n");

    }
  }
  else
  {

   printf("> ERROR : WIFI Module cannot be initialized.\n");

  }

  while(1)
  {
      WebServerProcess ();
  }
}

/**
  * @brief  Send HTML page
  * @param  None
  * @retval None
  */
static WIFI_Status_t SendWebPage(uint8_t ledIsOn)
{
  uint8_t  temp[50];
  uint16_t SentDataLength;
  WIFI_Status_t ret;

  /* construct web page content */

/*
  strcpy((char *)http, (char *)"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n");
  strcat((char *)http, (char *)"<html>\r\n<body>\r\n");
  strcat((char *)http, (char *)"<title>STM32 Web Server</title>\r\n");
  strcat((char *)http, (char *)"<h2>Fuchsit</h2>\r\n");
  strcat((char *)http, (char *)"<br /><hr>\r\n");
*/
  strcpy((char *)http, (char *)"****Hello, I am an IoT device with id: fuchsit 1****\n");

  ret = WIFI_SendData(0, (uint8_t *)http, strlen((char *)http), &SentDataLength, WIFI_WRITE_TIMEOUT);

  if((ret == WIFI_STATUS_OK) && (SentDataLength != strlen((char *)http)))
  {
    ret = WIFI_STATUS_ERROR;
  }

  return ret;
}


/**
  * @brief  Send HTML page
  * @param  None
  * @retval None
  */
static void WebServerProcess(void)
{
  uint8_t LedState = 0;

  switch(State)
  {
  case WS_IDLE:
    Socket = 0;
    WIFI_StartServer(Socket, WIFI_UDP_PROTOCOL, "", PORT);

    if(Socket != -1)
    {
      printf("> HTTP Server Started \n");

      State = WS_CONNECTED;
    }
    else
    {

      printf("> ERROR : Connection cannot be established.\n");

      State = WS_ERROR;
    }
    break;

  case WS_CONNECTED:
	//printf(">>>RESP1:\n\n %s" , resp);
    WIFI_ReceiveData(Socket, resp, 1200, &respLen, WIFI_READ_TIMEOUT);
    if( respLen > 0)
    {
      if(strstr((char *)resp, "fuchsit")) /* GET: put web page */
      {
    	  printf(">>>RESP2:\n\n %s" , resp);
    	  printf("fuchsit\n");
    	  SendWebPage(LedState);
    	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    	  HAL_Delay(5000);
    	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);

    //	  if(SendWebPage(LedState) != WIFI_STATUS_OK)
      //  {

        //  printf("> ERROR : Cannot send web page\n");

       //   State = WS_ERROR;
       // }
      } else{
    	  printf("you called: %s\n" , resp);
    	  printf("this call is not for me \ni am waiting for my name: fuchsit \n\n");
    	  }
      }

/*      else if(strstr((char *)resp, "POST")) POST: received info
        {
          if(strstr((char *)resp, "radio"))
          {
            if(strstr((char *)resp, "radio=0"))
            {
              LedState = 0;
              BSP_LED_Off(LED2);
            }
            else if(strstr((char *)resp, "radio=1"))
            {
              LedState = 1;
              BSP_LED_On(LED2);
            }

    //        if(SendWebPage(LedState) != WIFI_STATUS_OK)
            {

              printf("> ERROR : Cannot send web page\n");

              State = WS_ERROR;
          }
        }
      }
    }*/
    if(WIFI_StopServer(Socket) == WIFI_STATUS_OK)
    {
      WIFI_StartServer(Socket, WIFI_UDP_PROTOCOL, "", PORT);
    }
    else
    {
      State = WS_ERROR;
    }
    break;
  case WS_ERROR:
  default:
    break;
  }
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
static void SystemClock_Config(void)
{
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
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }
}


/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART1 and Loop until the end of transmission */
  HAL_UART_Transmit(&hDiscoUart, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}


#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(char* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

