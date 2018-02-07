
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef FLASH_H
#define FLASH_H

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "stm32l475e_iot01.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

#define ADDR_FLASH_PAGE_256   ((uint32_t)0x08080000) /* Base @ of Page 256, 2 Kbytes */
#define ADDR_FLASH_PAGE_257   ((uint32_t)0x08080800) /* Base @ of Page 257, 2 Kbytes */
#define ADDR_FLASH_PAGE_511   ((uint32_t)0x080ff800) /* Base @ of Page 511, 2 Kbytes */
/* Exported functions ------------------------------------------------------- */


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

#define FLASH_SUCCESS			-1
#define FLASH_ROW_SIZE          32

/* !!! Be careful the user area should be in another bank than the code !!! */
#define FLASH_USER_START_ADDR   ADDR_FLASH_PAGE_256   /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR     ADDR_FLASH_PAGE_511 + FLASH_PAGE_SIZE - 1   /* End @ of user Flash area */


#endif /* FLASH_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
