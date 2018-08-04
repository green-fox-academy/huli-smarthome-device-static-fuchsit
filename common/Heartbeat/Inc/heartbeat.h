#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include "main.h"

/*
 * FUT
 * will set the timeout before restarting if there is no connection
 */
void set_restart_timeout(uint32_t timeout);

/*
 * FUT
 * stops checking if net connection was timed out
 */
void stop_restart_timeout();


/*
 * FUT
 * will start a boot procedure if we lost net connection
 */
int restart_due_to_timeout_needed();


/**
  * @brief TIM MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param htim: TIM handle pointer
  * @retval None
  *
  * This timer is currently used to report fan and temp status
  * back to ggl core,
  * FUT
  * there is a need to sed a different timer handling gll core reports
  * and net connection timeouts
  */
void TIM4_Init(TIM_HandleTypeDef *htim);

void TIM4_IRQHandler(void);

/*
 * FUT
 * to save device struct info to flash memory
 */
void save_config();

/*
 * FUT
 * implement to restart device
 */
void restart_device();

/*
 * FUT
 * start the restart procedure to go back to net
 */
void restart_procedure();

/*
 * now handles ggl reports and fan operation
 * FUT
 * set a separate timer for ggl core and restart timer
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif //HEARTBEAT_H
