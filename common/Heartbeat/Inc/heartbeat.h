#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include "main.h"

void set_restart_timeout(uint32_t timeout);

int restart_due_to_timeout_needed();

void stop_restart_timeout();


/**
  * @brief TIM MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param htim: TIM handle pointer
  * @retval None
  */
void TIM4_Init(TIM_HandleTypeDef *htim);

void TIM3_IRQHandler(void);

void save_config();

void restart_device();

void restart_procedure();

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif //HEARTBEAT_H
