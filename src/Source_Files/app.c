/**
 * @file app.c
 * @author Connor Peskin
 * @date September 10, 2020
 * @brief Contains all of the functions to set up and start PWM function using LETIMER0,
 * as well as all of the interrupt event handlers for the LETIMER0 peripheral. Also
 * contains i2c transmission functionality to read the temperature from the Si7021
 * peripheral.
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "app.h"
#include <string.h>


//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// Static / Private Variables
//***********************************************************************************

//#define BLE_TEST_ENABLED

//***********************************************************************************
// Private functions
//***********************************************************************************

//static void app_letimer_pwm_open(float period, float act_period, uint32_t out0_route, uint32_t out1_route);

//***********************************************************************************
// Global functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 * Function to setup the peripheral drivers for PWM use and Interrupts of LETIMER
 *
 * @details
 * This function calls the CMU initialization driver, GPIO init driver, and LETIMER PWM init driver
 * prior to starting the LETIMER.
 * The BLE module is also opened using LEUART and a circular buffer.
 *
 * @note
 * This function should be called to initialize all peripherals.
 *
 ******************************************************************************/

void app_peripheral_setup(void){
	cmu_open();
	gpio_open();
	sleep_open();
	scheduler_open();
	si7021_i2c_open();
	sleep_block_mode(SYSTEM_BLOCK_EM);
	ble_open(BLE_TX_DONE_CB,0);
	app_letimer_pwm_open(PWM_PER, PWM_ACT_PER, PWM_ROUTE_0, PWM_ROUTE_1);

	add_scheduled_event(BOOT_UP_CB); //TDD - Lab 5
}

/***************************************************************************//**
 * @brief
 * Uses parameters to create structure for the LETIMER PWM driver initilization driver to reference.
 *
 * @details
 * Using the parameters passed, this function creates an APP_LETIMER_PWM_TypeDef structure and
 * passes it to letimer_pwm_open for PWM driver init.
 * It then calls for the LETIMER to start counting.
 *
 * @note
 * This function should be called prior to starting the LETIMER counting, as LETIMER must be
 * configured to run in PWM format.
 *
 * @param[in] float
 * float value in seconds of how long the PWM value should remain in logic 0 (low).
 *
 * @param[in] float
 * float value in seconds of how long th PWM should remain in logic 1 (high) (active).
 *
 * @param[in] uint32_t
 * unsigned 32bit int with the location of the first routed output. Going to LETIMER_ROUTELOC0 register.
 *
 * @param[in] uint32_t
 * unsigned 32bit int with the location of the second routed output. Going to LETIMER_ROUTELOC0 register.
 *
 ******************************************************************************/
void app_letimer_pwm_open(float period, float act_period, uint32_t out0_route, uint32_t out1_route){
	// Initializing LETIMER0 for PWM operation by creating the
	// letimer_pwm_struct and initializing all of its elements
	APP_LETIMER_PWM_TypeDef letimer_pwm_struct; // declaration of pwm struct
	letimer_pwm_struct.debugRun = false;
	letimer_pwm_struct.enable = true;
	letimer_pwm_struct.out_pin_route0 = out0_route;
	letimer_pwm_struct.out_pin_route1 = out1_route;
	letimer_pwm_struct.out_pin_0_en = false; //true;
	letimer_pwm_struct.out_pin_1_en = false; //true;
	letimer_pwm_struct.period = period;
	letimer_pwm_struct.active_period = act_period;
	letimer_pwm_struct.comp0_irq_enable = false;
	letimer_pwm_struct.comp0_cb = LETIMER0_COMP0_CB;
	letimer_pwm_struct.comp1_irq_enable = false;
	letimer_pwm_struct.comp1_cb = LETIMER0_COMP1_CB;
	letimer_pwm_struct.uf_irq_enable = true;
	letimer_pwm_struct.uf_cb = LETIMER0_UF_CB;


	letimer_pwm_open(LETIMER0, &letimer_pwm_struct);

	// letimer_start will inform the LETIMER0 peripheral to begin counting.
	letimer_start(LETIMER0, true);
}


/***************************************************************************//**
 * @brief
 * LETIMER0 Comp0 Interrupt Event
 *
 * @details
 * The COMP0 event will currently do nothing.
 * @note
 * Code should currently not enter this event handler, it will always assert to
 * false and stop the running program.
 *
 *
 ******************************************************************************/
void scheduled_letimer0_comp0_evt(void){
	remove_scheduled_event(LETIMER0_COMP0_CB);
	EFM_ASSERT(false);
}


/***************************************************************************//**
 * @brief
 * LETIMER0 Comp1 Interrupt Event
 *
 * @details
 * The COMP1 event will currently do nothing.
 *
 * @note
 * Code should currently not enter this event handler, it will always assert to
 * false and stop the running program.
 *
 ******************************************************************************/
void scheduled_letimer0_comp1_evt(void){
	remove_scheduled_event(LETIMER0_COMP1_CB);
	EFM_ASSERT(false);
}

/***************************************************************************//**
 * @brief
 * LETIMER0 UF Interrupt Event
 *
 * @details
 * The UF interrupt handler is being used to cycle through energy modes. Upon
 * entering the underflow interrupt event the energy mode will change.
 *
 * @note
 * This will not cycle into the EM4 energy mode, as we need the low frequency
 * clocks for the LETIMER interrupts.
 *
 ******************************************************************************/
void scheduled_letimer0_uf_evt(void){
	remove_scheduled_event(LETIMER0_UF_CB);

	si7021_read(I2C_7021_READ_CB);
}

/***************************************************************************//**
 * @brief
 * SI7021 Temperature Reading Complete Handler
 *
 * @details
 * Will get the temperature (F) reading from the private variable in SI7021.c
 * using the tempConvert command.  If the temperature is above 80 (F), LED1
 * will be asserted. If the temperature is below 80 (F), LED1 will be deasserted.
 * The result temperature data will be transmitted to the HM18 peripheral via
 * LEUART.
 *
 * @note
 *	Corresponds with scheduled event 'I2C_7021_READ_CB'
 *
 ******************************************************************************/
void scheduled_si7021_tempDone(void){
	EFM_ASSERT(get_scheduled_events() & I2C_7021_READ_CB);
	remove_scheduled_event(I2C_7021_READ_CB);

	float tempReading = tempConvert_si7021();
	if(tempReading > 80) GPIO_PinOutSet(LED1_PORT, LED1_PIN);
	else GPIO_PinOutClear(LED1_PORT, LED1_PIN);

	char string[15];
	sprintf(string, "Temp = %.1f F\n", tempReading);
	if(string[10] == '0') sprintf(string, "Temp = %.0f F\n", tempReading);
	ble_write(string);
}

/***************************************************************************//**
 * @brief
 *	Boot Up Callback used for LEUART
 *
 * @details
 *	This function will be called upon booting up the device and will print
 *	"Hello World" to the LEUART peripheral. If BLE_TEST_ENABLED is defined,
 *	this function will test the LEUART communication with the peripheral.
 *
 * @note
 * Corresponds with scheduled event 'BOOT_UP_CB'
 *
 ******************************************************************************/
void scheduled_boot_up_cb(void){
	EFM_ASSERT(get_scheduled_events() & BOOT_UP_CB);
	remove_scheduled_event(BOOT_UP_CB);

#ifdef BLE_TEST_ENABLED
	bool success = ble_test("PESKIN_UART");
	EFM_ASSERT(success);
	timer_delay(2000u);
#endif

	circular_buff_test();
	si7021_TDD_config();

	ble_write("\nHello World\n");
	ble_write("Course Project I2C\n");
	ble_write("Connor Peskin\n");
	letimer_start(LETIMER0, true);
}

/***************************************************************************//**
 * @brief
 *	LEUART TX Complete UART
 *
 * @details
 *	This will be called at the end of the LEUART TX state machine. It will
 *	check the circular buffer to see if there is still data and send if there is.
 *
 * @note
 * Corresponds with scheduled event 'BOOT_UP_CB'
 *
 ******************************************************************************/
void ble_tx_done_cb(void){
	EFM_ASSERT(get_scheduled_events() & BLE_TX_DONE_CB);
	remove_scheduled_event(BLE_TX_DONE_CB);

	ble_circ_pop(false);
}


