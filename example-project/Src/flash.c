/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "flash.h"
#include <inttypes.h>

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

/*
 * saved device's current operation status to flash memory address
 * before saving, deletes the whole bank
 * dest_address suggest to be defined @ ADDR_FLASH_PAGE_256 (dest_address = FLASH_USER_START_ADDR)
 * returns flash error code or FLASH_SUCCESS on success
 */
uint32_t save_device_info_to_flash(uint32_t dest_address, uint64_t src_address)
{
	uint32_t BankNumber = 0;
	uint32_t Address = 0, PAGEError = 0;
	__IO uint32_t MemoryProgramStatus = 0;
	__IO uint64_t data64 = 0;
	static FLASH_EraseInitTypeDef EraseInitStruct;

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
		//Error occurred while mass erase.
		return HAL_FLASH_GetError();
	}

	/* Program the user Flash area word by word
	(area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, dest_address, src_address);

	/* Check if the programmed data is OK
	  MemoryProgramStatus = 0: data programmed correctly
	  MemoryProgramStatus != 0: number of words not programmed correctly ******/

	data64 = *(__IO uint64_t *)dest_address;

	if(data64 != src_address)
	{
		MemoryProgramStatus++;
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

	/* Lock the Flash to disable the flash control register access (recommended
	to protect the FLASH memory against possible unwanted operation) *********/
	HAL_FLASH_Lock();

	return FLASH_SUCCESS;
}

/*
 * read saved device info
 * if value = already saved -> load that data
 */
uint32_t read_device_info_from_flash(uint32_t dest_address)
{
	char *p_address_256 = dest_address;

	printf("%" PRIx64 "\n", *(p_address_256));

	printf("p address val with string: %s\n",  ullx(*p_address_256));

	printf("char at 256 0: %c\n", (int)*(p_address_256));

	return FLASH_SUCCESS;
}
