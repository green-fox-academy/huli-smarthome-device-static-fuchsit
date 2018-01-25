#include "rgb_led_color.h"

void Project_Led_Lights (char *color) {
	RGB_Init();
	json_hexa_for_rgbled(color);
	LED_ON (red, blue, green);
}

void json_hexa_for_rgbled(char led_color_hexa_RGB[]){

	char red_text[5] = "0x";
	char blue_text[5] = "0x";
	char green_text[5] = "0x";

    int size = strlen(led_color_hexa_RGB);

	for (int i = 0 ; i <  size ; ++i){

        if (i < 2){
            red_text[i + 2] = led_color_hexa_RGB[i];
         }
        if ( i > 1 && i < 4){
            green_text [i] = led_color_hexa_RGB[i];
        }
        if ( i> 3 && i < 6){

            blue_text [i - 2] = led_color_hexa_RGB[i];
        }
	}
    red = strtol(red_text , NULL , 16);
    blue = strtol(blue_text , NULL , 16);
    green = strtol(green_text , NULL , 16);
    printf ("%d\n", red);
    printf ("%d\n", blue);
    printf ("%d\n", green);
}

void LED_ON (int _red, int _blue, int _green){

	TIM3->RED = (255 - _red);
	TIM3->BLUE = (255 - _blue);
	TIM2->GREEN = (255 - _green);

}

void LED_OFF (void) {

	TIM3->RED = 257;
	TIM3->BLUE = 257;
	TIM2->GREEN = 257;

}

void RGB_Init(void) {

	BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
	BSP_LED_Init(LED_GREEN);

//	UART_Init();

	TIMER_Init_RED();
	PWM_Init_RED();
	TIMER_Init_BLUE();
	PWM_Init_BLUE();
	TIMER_Init_GREEN();
	PWM_Init_GREEN();

	LED_Init_RED();
	LED_Init_BLUE();
	LED_Init_GREEN();

}

/*void UART_Init(void) {
	uartHandle.Init.BaudRate = 115200;
	uartHandle.Init.WordLength = UART_WORDLENGTH_8B;
	uartHandle.Init.StopBits = UART_STOPBITS_1;
	uartHandle.Init.Parity = UART_PARITY_NONE;
	uartHandle.Init.Mode = UART_MODE_TX_RX;

	BSP_COM_Init(COM1, &uartHandle);
}*/

void PWM_Init_RED(void) {
	HAL_TIM_PWM_Init(&TimHandleR);

	sConfigR.OCMode = TIM_OCMODE_PWM1;
	sConfigR.Pulse = 257;
	HAL_TIM_PWM_ConfigChannel(&TimHandleR , &sConfigR , TIM_CHANNEL_3);

	HAL_TIM_PWM_Start(&TimHandleR , TIM_CHANNEL_3);
}


void PWM_Init_BLUE(void) {
	HAL_TIM_PWM_Init(&TimHandleB);

	sConfigB.OCMode = TIM_OCMODE_PWM1;
	sConfigB.Pulse = 257;
	HAL_TIM_PWM_ConfigChannel(&TimHandleB , &sConfigB , TIM_CHANNEL_2);

	HAL_TIM_PWM_Start(&TimHandleB , TIM_CHANNEL_2);
}

void PWM_Init_GREEN(void) {
	HAL_TIM_PWM_Init(&TimHandleG);

	sConfigG.OCMode = TIM_OCMODE_PWM1;
	sConfigG.Pulse = 257;
	HAL_TIM_PWM_ConfigChannel(&TimHandleG , &sConfigG , TIM_CHANNEL_1);

	HAL_TIM_PWM_Start(&TimHandleG , TIM_CHANNEL_1);
}


void TIMER_Init_RED(void) {
	__HAL_RCC_TIM3_CLK_ENABLE();

	TimHandleR.Instance               = TIM3;
	TimHandleR.Init.Period            = 255; //16bit number max value 0xFFFF
	TimHandleR.Init.Prescaler         = 0;
	TimHandleR.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	TimHandleR.Init.CounterMode       = TIM_COUNTERMODE_UP;

	HAL_TIM_Base_Init(&TimHandleR); //Configure the timer

}

void TIMER_Init_BLUE(void) {

	TimHandleB.Instance               = TIM3;
	TimHandleB.Init.Period            = 255; //16bit number max value 0xFFFF
	TimHandleB.Init.Prescaler         = 0;
	TimHandleB.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	TimHandleB.Init.CounterMode       = TIM_COUNTERMODE_UP;

	HAL_TIM_Base_Init(&TimHandleB); //Configure the timer
}

void TIMER_Init_GREEN(void) {
	__HAL_RCC_TIM2_CLK_ENABLE();

	TimHandleG.Instance               = TIM2;
	TimHandleG.Init.Period            = 255; //16bit number max value 0xFFFF
	TimHandleG.Init.Prescaler         = 0;
	TimHandleG.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	TimHandleG.Init.CounterMode       = TIM_COUNTERMODE_UP;

	HAL_TIM_Base_Init(&TimHandleG); //Configure the timer
}

void LED_Init_RED(void) {

	__HAL_RCC_GPIOB_CLK_ENABLE();    // we need to enable the GPIO* port's clock first

	LEDRED.Pin = REDPIN;            // this is about PIN 1
	LEDRED.Mode = GPIO_MODE_AF_OD; // Configure as output with push-up-down enabled
	LEDRED.Pull = GPIO_NOPULL;      // the push-up-down should work as pulldown
	LEDRED.Speed = GPIO_SPEED_HIGH;     // we need a high-speed output
	LEDRED.Alternate = GPIO_AF2_TIM3;   //Alterante function to set PWM timer

	HAL_GPIO_Init(REDPORT, &LEDRED);   // initialize the pin on GPIO* port with HAL
}

void LED_Init_BLUE(void) {
	__HAL_RCC_GPIOA_CLK_ENABLE();    // we need to enable the GPIO* port's clock first

	LEDBLUE.Pin = BLUEPIN;            // this is about PIN 1
	LEDBLUE.Mode = GPIO_MODE_AF_OD; // Configure as output with push-up-down enabled
	LEDBLUE.Pull = GPIO_NOPULL;      // the push-up-down should work as pulldown
	LEDBLUE.Speed = GPIO_SPEED_HIGH;     // we need a high-speed output
	LEDBLUE.Alternate = GPIO_AF2_TIM3;   //Alterante function to set PWM timer

	HAL_GPIO_Init(BLUEPORT, &LEDBLUE);   // initialize the pin on GPIO* port with HAL
}
void LED_Init_GREEN(void) {

	LEDGREEN.Pin = GREENPIN;            // this is about PIN 1
	LEDGREEN.Mode = GPIO_MODE_AF_OD; // Configure as output with push-up-down enabled
	LEDGREEN.Pull = GPIO_NOPULL;      // the push-up-down should work as pulldown
	LEDGREEN.Speed = GPIO_SPEED_HIGH;     // we need a high-speed output
	LEDGREEN.Alternate = GPIO_AF1_TIM2;   //Alterante function to set PWM timer

	HAL_GPIO_Init(GREENPORT, &LEDGREEN);   // initialize the pin on GPIO* port with HAL
}

