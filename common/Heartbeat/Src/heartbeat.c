/*****************************************
 * FUT
 * if given operation throws global error, which prevents normal operation
 * than the restart_needed should be updated with TRUE
 *
 * a period elapsed callback interrupt should run with exp backoff times to check current error status
 * (or device.state_of_operation)
 * if there is a global error device should restart
 */

/*
 * FUT
 * add operation continuity control to GGL mode too
 */

#include "heartbeat.h"
#include "aircondi.h"

volatile int restart_enabled = FALSE;
volatile int restart_needed = FALSE;
volatile uint32_t process_start;
volatile uint32_t restart_timeout_deadlie;
TIM_HandleTypeDef TIM4Handle;

void set_restart_timeout(uint32_t timeout) {
	restart_enabled = TRUE;
	process_start = HAL_GetTick();
	restart_timeout_deadlie = timeout;
}

int restart_due_to_timeout_needed() {

	if (!restart_enabled)
		return FALSE;

	if (HAL_GetTick() - restart_timeout_deadlie > process_start)
		return TRUE;
	else
		return FALSE;
}

void stop_restart_timeout() {
	restart_enabled = FALSE;
}

/* restart handler TIM handle declaration */
TIM_HandleTypeDef TIM4Handle;

/**
  * @brief TIM MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param htim: TIM handle pointer
  * @retval None
  */
void TIM4_Init(TIM_HandleTypeDef *htim)
{

	  /*##-1- Enable peripherals and GPIO Clocks #################################*/
	  /* TIMx Peripheral clock enable */
	 __HAL_RCC_TIM4_CLK_ENABLE();

	TIM4Handle.Instance = TIM4;

	 /* Initialize TIMx peripheral as follows:
	      + Period = 10000 - 1
	      + Prescaler = (SystemCoreClock/10000) - 1
	      + ClockDivision = 0
	      + Counter direction = Up
	 */
	TIM4Handle.Init.Period            = 10000 - 1;
	TIM4Handle.Init.Prescaler         = 8000 - 1; // FUT to be calculated
	TIM4Handle.Init.ClockDivision     = 0;
	TIM4Handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
	TIM4Handle.Init.RepetitionCounter = 0;

	 if (HAL_TIM_Base_Init(&TIM4Handle) != HAL_OK)
	 {
	   /* Initialization Error */
	   //Error_Handler();
	 }

	 /*##-2- Start the TIM Base generation in interrupt mode ####################*/
	 /* Start Channel1 */
	// FUT null the timer register
	 if (HAL_TIM_Base_Start_IT(&TIM4Handle) != HAL_OK)
	 {
	   /* Starting Error */
	   //Error_Handler();
	 }


  /*##-2- Configure the NVIC for TIMx ########################################*/
  /* Set the TIMx priority */
  HAL_NVIC_SetPriority(TIM4_IRQn, 3, 0);

  /* Enable the TIMx global Interrupt */
  HAL_NVIC_EnableIRQ(TIM4_IRQn);
}

void TIM4_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&TIM4Handle);
}

void save_config() {
	//save configuration to EPROM
}
void restart_device() {
	//restarts board
}
void restart_procedure() {
	save_config();
	restart_device();
	//printf("restart procedure called\n");
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
/*	if (restart_due_to_timeout_needed())
		restart_procedure();

	//printf("callback called\n");*/
	temp_set(user_min, user_max);

	/*printf("Actual Temp: %d\n", temp);
	printf("User min: %d\n", user_min);
	printf("User max: %d\n", user_max);
	printf("Pwm: %d\n", TIM2 -> CCR3);*/
}
/* **************************************** */
