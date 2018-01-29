/**
 *
 */
#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_pit.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_port.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define PIT_LED_HANDLER PIT0_IRQHandler
#define PORTA_SWITCH_HANDLER PORTA_IRQHandler
#define PORTC_SWITCH_HANDLER PORTC_IRQHandler
#define PIT_IRQ_ID PIT0_IRQn
/* Get source clock for PIT driver */
#define PIT_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_BusClk)
/*******************************************************************************
 * Structures
 ******************************************************************************/
typedef struct state {

	void (*ptr)(void); /**pointer to function*/
	uint8_t next;
	uint8_t back;

} State;
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void turn_leds_off(void);
void turn_red_led_on(void);
void turn_green_led_on(void);
void turn_blue_led_on(void);
/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile bool pitIsrFlag = false;
volatile bool sw2 = false;
volatile bool sw3 = false;
volatile uint8_t current = 0;
const State led_state[3] = {
		{ &turn_red_led_on, 1, 2 },
		{ &turn_green_led_on, 2, 0 },
		{ &turn_blue_led_on, 0, 1 }
};
/*******************************************************************************
 * Code
 ******************************************************************************/



void PORTA_SWITCH_HANDLER()
{

	/*Limpiamos la bandera del pin que causo la interrupcion*/
	PORT_ClearPinsInterruptFlags(PORTA, 1 << 4);

	/*Si state es igual a 0 entonces se hace 1 y al revÃ©s*/
	sw2 = (0 == sw2) ? 1 : 0;
}

void PORTC_SWITCH_HANDLER()
{

	/*Limpiamos la bandera del pin que causo la interrupcion*/
	PORT_ClearPinsInterruptFlags(PORTC, 1 << 6);

	/*Si state es igual a 0 entonces se hace 1 y al revÃ©s*/
	sw3 = (0 == sw3) ? 1 : 0;
}

void PIT_LED_HANDLER(void)
{
	/* Clear interrupt flag.*/
	PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, kPIT_TimerFlag);
	pitIsrFlag = true;

}
/*!
 * @brief Main function
 */
int main(void) {
	/* Structure of initialize PIT */
	pit_config_t pitConfig;

	/* Board pin, clock, debug console init */
	BOARD_InitPins();
	BOARD_BootClockRUN();
	BOARD_InitDebugConsole();

	/*Habilitar el reloj SCG*/
	    CLOCK_EnableClock(kCLOCK_PortB);
	    CLOCK_EnableClock(kCLOCK_PortA);
	    CLOCK_EnableClock(kCLOCK_PortE);
	    CLOCK_EnableClock(kCLOCK_PortC);

	    /*Configurar el puerto para encender un LED*/
	    /* Input pin PORT configuration */
	    port_pin_config_t config_led = {
	    		kPORT_PullDisable,				/*Resistencias deshabilitadas*/
	    		kPORT_SlowSlewRate,				/*SlewRate menor velocidad*/
	    		kPORT_PassiveFilterEnable,		/*Filtro habilitado*/
	    		kPORT_OpenDrainDisable,			/**/
	    		kPORT_LowDriveStrength,			/**/
	    		kPORT_MuxAsGpio,				/*Modo GPIO*/
	    		kPORT_UnlockRegister };			/**/

	    /* Input pin PORT configuration */
	    port_pin_config_t config_switch = {
	    		kPORT_PullDisable,
	    		kPORT_SlowSlewRate,
	    		kPORT_PassiveFilterEnable,
	    		kPORT_OpenDrainDisable,
	    		kPORT_LowDriveStrength,
	    		kPORT_MuxAsGpio,
	    		kPORT_UnlockRegister};

	    PORT_SetPinInterruptConfig(PORTA, 4, kPORT_InterruptFallingEdge);
	    PORT_SetPinInterruptConfig(PORTC, 6, kPORT_InterruptFallingEdge);
	    /* Sets the configuration */
	    PORT_SetPinConfig(PORTB, 21, &config_led);
	    PORT_SetPinConfig(PORTB, 22, &config_led);
	    PORT_SetPinConfig(PORTE, 26, &config_led);
	    PORT_SetPinConfig(PORTA, 4, &config_switch);
	    PORT_SetPinConfig(PORTC, 6, &config_switch);

	NVIC_EnableIRQ(PORTA_IRQn);
	NVIC_EnableIRQ(PORTC_IRQn);

	gpio_pin_config_t led_config = { kGPIO_DigitalOutput, 1 };
	gpio_pin_config_t switch_config = { kGPIO_DigitalInput, 0 };

	/* Sets the configuration */
	GPIO_PinInit(GPIOA, 4, &switch_config);
	GPIO_PinInit(GPIOC, 6, &switch_config);

	GPIO_PinInit(GPIOB, 21, &led_config);
	GPIO_PinInit(GPIOB, 22, &led_config);
	GPIO_PinInit(GPIOE, 26, &led_config);


	/*******************************************************************************
	 * Pit
	 ******************************************************************************/

	/*
	 * pitConfig.enableRunInDebug = false;
	 */
	PIT_GetDefaultConfig(&pitConfig);

	/* Init pit module */
	PIT_Init(PIT, &pitConfig);

	/* Set timer period for channel 0 */
	PIT_SetTimerPeriod(PIT, kPIT_Chnl_0,
			USEC_TO_COUNT(1000000U, PIT_SOURCE_CLOCK));

	/* Enable timer interrupts for channel 0 */
	PIT_EnableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);

	/* Enable at the NVIC */
	EnableIRQ(PIT_IRQ_ID);

	/* Start channel 0 */
	PRINTF("\r\nStarting channel No.0 ...");
	PIT_StartTimer(PIT, kPIT_Chnl_0);

	while (true)
	{
		/* Check whether occur interupt and toggle LED */
		if (true == pitIsrFlag)
		{
			PRINTF("\r\n Channel No.0 interrupt is occured !");

			if(sw3)
			{

			}
			else if (sw2)
			{
				current = led_state[current].next;
				led_state[current].ptr();
			}
			else
			{
				//calling function in current state
				current = led_state[current].back;
				led_state[current].ptr();
			}

			pitIsrFlag = false;
		}
	}
}

void turn_leds_off(void)
{
	GPIO_SetPinsOutput(GPIOB, 1 << 21);
	GPIO_SetPinsOutput(GPIOB, 1 << 22);
	GPIO_SetPinsOutput(GPIOE, 1 << 26);
}

void turn_red_led_on(void)
{
	turn_leds_off();
	GPIO_ClearPinsOutput(GPIOB, 1 << 22);
}
void turn_green_led_on(void)
{
	turn_leds_off();
	GPIO_ClearPinsOutput(GPIOE, 1 << 26);
}
void turn_blue_led_on(void)
{
	turn_leds_off();
	GPIO_ClearPinsOutput(GPIOB, 1 << 21);
}
