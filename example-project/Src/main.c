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
#include "main.h"
#include <inttypes.h>


/*
 * http://www.st.com/content/ccc/resource/training/technical/
 * product_training/91/e3/aa/26/e6/69/4f/de/STM32L4_Memory_Flash.pdf/
 * files/STM32L4_Memory_Flash.pdf/jcr:content/translations/en.STM32L4_Memory_Flash.pdf
 *
 * 1mb flash
 * 2x512 kbyte banks
 * each 256 pages of 2kbyte memory/page
 * 8 row/page -> 256 byte/row
 *
 * 8 row * 256 kbyte * 256 page * 2 bank = 1Mbyte flash memory
 */


#define FLASH_ROW_SIZE          32

/* !!! Be careful the user area should be in another bank than the code !!! */
#define FLASH_USER_START_ADDR   ADDR_FLASH_PAGE_256   /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR     ADDR_FLASH_PAGE_511 + FLASH_PAGE_SIZE - 1   /* End @ of user Flash area */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint32_t BankNumber = 0;
uint32_t Address = 0, PAGEError = 0;
__IO uint32_t MemoryProgramStatus = 0;
__IO uint64_t data64 = 0;

/*Variable used for Erase procedure*/
static FLASH_EraseInitTypeDef EraseInitStruct;

#define DATA_64                 ((uint64_t)0x0000000000000065)

/* Table used for fast programming */
static const uint64_t Data64_To_Prog[FLASH_ROW_SIZE] = {
  0x0000000000000062, 0x0000000000000062, 0x0000000000000063, 0x3333333333333333,
  0x4444444444444444, 0x5555555555555555, 0x6666666666666666, 0x7777777777777777,
  0x8888888888888888, 0x9999999999999999, 0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB,
  0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE, 0xFFFFFFFFFFFFFFFF,
  0x0011001100110011, 0x2233223322332233, 0x4455445544554455, 0x6677667766776677,
  0x8899889988998899, 0xAABBAABBAABBAABB, 0xCCDDCCDDCCDDCCDD, 0xEEFFEEFFEEFFEEFF,
  0x2200220022002200, 0x3311331133113311, 0x6644664466446644, 0x7755775577557755,
  0xAA88AA88AA88AA88, 0xBB99BB99BB99BB99, 0xEECCEECCEECCEECC, 0x0000000000000062};

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static uint32_t GetBank(uint32_t Address);


#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
 set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef uartHandle;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* Private functions ---------------------------------------------------------*/
static void Peripherals_Init(void);
static void UART_Init(void);

/*
 * returns uint64_t as hex in string format for embedded systems
 * https://stackoverflow.com/questions/9225567/how-to-print-a-int64-t-type-in-c
 */
char* ullx(uint64_t val)
{
    static char buf[34] = { [0 ... 33] = 0 };
    char* out = &buf[33];
    uint64_t hval = val;
    unsigned int hbase = 16;

    do {
        *out = "0123456789abcdef"[hval % hbase];
        --out;
        hval /= hbase;
    } while(hval);

    *out-- = 'x', *out = '0';

    return out;
}


/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void) {

	uint32_t src_addr = (uint32_t)Data64_To_Prog;
	uint8_t data_index = 0;

	Peripherals_Init();

	/* Unlock the Flash to enable the flash control register access *************/
	HAL_FLASH_Unlock();

	/* Erase the user Flash area
	(area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	/* Clear OPTVERR bit set on virgin samples */
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

	/* Get the bank */
	BankNumber = GetBank(FLASH_USER_START_ADDR);

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_MASSERASE;
	EraseInitStruct.Banks     = BankNumber;

	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
	{
		/*
		  Error occurred while mass erase.
		  User can add here some code to deal with this error.
		  To know the code error, user can call function 'HAL_FLASH_GetError()'
		*/
		/* Infinite loop */
		while (1)
		{
			printf("Error using flash erase!\n");
			HAL_Delay(500);
		}
	}

	/* Program the user Flash area word by word
	(area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	Address = FLASH_USER_START_ADDR;

	int i = 0;
	while (Address < (FLASH_USER_END_ADDR - (FLASH_ROW_SIZE*sizeof(uint64_t))))
	{

		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FAST, Address, (uint64_t)src_addr) == HAL_OK)
		{
			Address = Address + (FLASH_ROW_SIZE*sizeof(uint64_t));
		}
		else
		{
		  /* Error occurred while writing data in Flash memory.
			 User can add here some code to deal with this error */
			while (1)
			{
				printf("Error using flash programming fast!\n");
				HAL_Delay(500);
			 }
		}
	}

	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FAST_AND_LAST, Address, (uint64_t)src_addr) != HAL_OK)
	{
		/* Error occurred while writing data in Flash memory.
		   User can add here some code to deal with this error */
		while (1)
		{
			printf("Error using flash programming fast and last!\n");
			HAL_Delay(500);
		}
	}

	/* Lock the Flash to disable the flash control register access (recommended
	 to protect the FLASH memory against possible unwanted operation) *********/
	HAL_FLASH_Lock();


	/* Check if the programmed data is OK
	  MemoryProgramStatus = 0: data programmed correctly
	  MemoryProgramStatus != 0: number of words not programmed correctly ******/
	Address = FLASH_USER_START_ADDR;
	MemoryProgramStatus = 0x0;

	while (Address < FLASH_USER_END_ADDR)
	{
		for (data_index = 0; data_index < FLASH_ROW_SIZE; data_index++)
		{
			data64 = *(__IO uint64_t *)Address;

			if(data64 != Data64_To_Prog[data_index])
			{
				MemoryProgramStatus++;
			}

			Address = Address + sizeof(uint64_t);
		}
	}

	/*Check if there is an issue to program data*/
	if (MemoryProgramStatus == 0)
	{
	/* No error detected. Switch on LED1*/
		printf("no error detected in memory check!\n");
	}
	else
	{
	/* Error detected. Switch on LED2*/
		printf("some error detected in memory check!\n");
	}

	/* Infinite loop */


	HAL_FLASH_Unlock();

	 Address = FLASH_USER_START_ADDR;

	 __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

	 	/* Get the bank */
	 	BankNumber = GetBank(FLASH_USER_START_ADDR);

	 	/* Fill EraseInit structure*/
	 	EraseInitStruct.TypeErase = FLASH_TYPEERASE_MASSERASE;
	 	EraseInitStruct.Banks     = BankNumber;

	 HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);

	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, Address, DATA_64);


	uint64_t *p_address = FLASH_USER_START_ADDR;
	uint64_t *p_address_at_end = FLASH_USER_END_ADDR;

	char *p_address_256 = ADDR_FLASH_PAGE_256;
	//uint64_t *p_address_257 = ADDR_FLASH_PAGE_257;


	//printf("value at address: %x\n", *(p_address + 1));

	printf("%" PRIx64 "\n", *(p_address));

	printf("p address value = %jd (0x%jx)\n", *(p_address + 1), *(p_address + 1));

	for (int i = 0; i < 32; ++i) {
		printf("p address val with string at %d: %s\n", i,  ullx(*(p_address + i)));
	}

	printf("char at 256 32: %c\n", (int)*(p_address_256 + 33));
	printf("char at 256 0: %c\n", (int)*(p_address_256));
	//printf("char at 257 0: %c\n", (int)*(p_address_257));

	printf("address of 256 0: %p\n", *p_address);
	printf("address of 256 1: %p\n", (p_address_256 + 1));
	printf("text: %c\n", (int)*(p_address));
	printf("text: %c\n", (int)*(p_address + 1));
	printf("text: %c\n", (int)*(p_address + 2));
	printf("text: %c\n", (int)*(p_address + 3));

	HAL_FLASH_Lock();

	while (1){}

}

/**
* @brief  Gets the bank of a given address
* @param  Addr: Address of the FLASH Memory
* @retval The bank of a given address
*/
static uint32_t GetBank(uint32_t Addr)
{
	uint32_t bank = 0;

	if (READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE) == 0)
	{
	/* No Bank swap */
		if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
		{
			bank = FLASH_BANK_1;
		}
		else
		{
			bank = FLASH_BANK_2;
		}
	}
	else
	{
		/* Bank swap */
		if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
		{
			bank = FLASH_BANK_2;
		}
		else
		{
			bank = FLASH_BANK_1;
		}
	}


	return bank;
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

	BSP_LED_Init(LED_GREEN);
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
void SystemClock_Config(void) {
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
