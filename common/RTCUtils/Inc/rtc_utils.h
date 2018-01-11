#ifndef RTC_UTILS_H
#define RTC_UTILS_H

#include <stdint.h>

#if defined(STM32L475xx)
#include "stm32l4xx_hal.h"
#elif defined(STM32F746xx)
#include "stm32f7xx_hal.h"
#endif

/*! \brief      Initializes the RTC hardware with HAL
 *  \discussion Tested on STM32F7 and STM32 IoT boards
 */
void RTCUtils_RTCInit();

/*! \brief      Reads date and time into the specified RTC structures
 *  \discussion
 *
 *  \param      time     		a pointer to the created RTC_TimeTypeDef structure
 *  \param		date			a pointer to the created RTC_DateTypeDef structure
 */
void RTCUtils_GetDateTime(RTC_TimeTypeDef *time, RTC_DateTypeDef *date);

/*! \brief      Returns the unix timestamp based on the RTC
 *  \discussion
 *
 *  \return     unsigned integer - epoch timestamp based on RTC
 */
uint32_t RTCUtils_GetEpochTimestamp();

/*! \brief      Sets the RTC to the specified epoch timestamp
 *  \discussion
 *
 *  \param      timestamp      the epoch timestamp to set
 */
void RTCUtils_SetEpochTimestamp(uint32_t timestamp);

#endif // RTC_UTILS_H
