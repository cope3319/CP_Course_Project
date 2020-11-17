/**
 * @file gpio.c
 * @author Connor Peskin
 * @date September 10, 2020
 * @brief Contains function for GPIO initialization.
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "gpio.h"


//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// Private variables
//***********************************************************************************


//***********************************************************************************
// Private functions
//***********************************************************************************


//***********************************************************************************
// Global functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 * Driver to initialize the GPIO peripherals.
 *
 * @details
 * This function will configure the GPIO pins for various app.c processes developed.
 * Will set drive strength and pin mode for proper GPIO pins.
 *
 * @note
 * Enums used in this function can be found in brd_config.h
 *
 ******************************************************************************/
void gpio_open(void){

	CMU_ClockEnable(cmuClock_GPIO, true);

	// Configure LED pins
	GPIO_DriveStrengthSet(LED0_PORT, LED0_DRIVE_STRENGTH);
	GPIO_PinModeSet(LED0_PORT, LED0_PIN, LED0_GPIOMODE, LED0_DEFAULT);

	GPIO_DriveStrengthSet(LED1_PORT, LED1_DRIVE_STRENGTH);
	GPIO_PinModeSet(LED1_PORT, LED1_PIN, LED1_GPIOMODE, LED1_DEFAULT);

	// Configure i2c gpio pins
	GPIO_DriveStrengthSet(SI7021_SENSOR_EN_PORT , SI7021_DRIVE_STRENGTH);
	GPIO_PinModeSet(SI7021_SENSOR_EN_PORT, SI7021_SENSOR_EN_PIN, SI7021_SENSOR_GPIOMODE, SI7021_EN_DEFAULT);

	GPIO_PinModeSet(SI7021_SDA_PORT, SI7021_SDA_PIN, SI7021_SDA_SCL_GPIOMODE , SI7021_EN_DEFAULT);
	GPIO_PinModeSet(SI7021_SCL_PORT, SI7021_SCL_PIN, SI7021_SDA_SCL_GPIOMODE , SI7021_EN_DEFAULT);

	/* UART Lab 5 */
	GPIO_DriveStrengthSet(UART_TX_PORT, UART_TX_DRIVESTRENGTH);
	GPIO_PinModeSet(UART_TX_PORT, UART_TX_PIN, UART_TX_GPIOMODE, UART_TX_EN_DEFUALT);
	GPIO_PinModeSet(UART_RX_PORT, UART_RX_PIN, UART_RX_GPIOMODE, UART_RX_EN_DEFUALT);
}
