#include <stdio.h>
#include <stdlib.h>

void json_hexa_for_rgbled(char led_color_hexa_RGB[]);

int red;
int blue;
int green;

#define REDPORT GPIOB
#define REDPIN GPIO_PIN_0
#define RED CCR3

#define BLUEPORT GPIOA
#define BLUEPIN GPIO_PIN_7
#define BLUE CCR2

#define GREENPORT GPIOA
#define GREENPIN GPIO_PIN_15
#define GREEN CCR1

UART_HandleTypeDef	uartHandle;
TIM_HandleTypeDef	TimHandleR;
TIM_HandleTypeDef	TimHandleB;
TIM_HandleTypeDef	TimHandleG;
TIM_OC_InitTypeDef	sConfigR;
TIM_OC_InitTypeDef	sConfigB;
TIM_OC_InitTypeDef	sConfigG;
GPIO_InitTypeDef	LEDRED;
GPIO_InitTypeDef	LEDBLUE;
GPIO_InitTypeDef	LEDGREEN;

static void RGB_Init(void);
static void UART_Init(void);
static void LED_Init_RED(void);
static void LED_Init_BLUE(void);
static void LED_Init_GREEN(void);

static void TIMER_Init_RED(void);
static void PWM_Init_RED(void);

static void TIMER_Init_BLUE(void);
static void PWM_Init_BLUE(void);

static void TIMER_Init_GREEN(void);
static void PWM_Init_GREEN(void);

static void LED_ON (int _red, int _blue, int _green);

static void LED_OFF (void);



