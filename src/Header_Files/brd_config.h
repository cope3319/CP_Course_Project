#ifndef BRD_CONFIG_HG
#define BRD_CONFIG_HG

//***********************************************************************************
// Include files
//***********************************************************************************
/* System include statements */


/* Silicon Labs include statements */
#include "em_cmu.h"
#include "em_gpio.h"

/* The developer's include statements */


//***********************************************************************************
// defined files
//***********************************************************************************

// GPIO pin setup
#define STRONG_DRIVE

// LED 0 pin is
#define	LED0_PORT				gpioPortF
#define LED0_PIN				04u
#define LED0_DEFAULT			false 	// Default false (0) = off, true (1) = on
#define LED0_GPIOMODE			gpioModePushPull

// LED 1 pin is
#define LED1_PORT				gpioPortF
#define LED1_PIN				05u
#define LED1_DEFAULT			false	// Default false (0) = off, true (1) = on
#define LED1_GPIOMODE			gpioModePushPull

#ifdef STRONG_DRIVE
	#define LED0_DRIVE_STRENGTH		gpioDriveStrengthStrongAlternateStrong
	#define LED1_DRIVE_STRENGTH		gpioDriveStrengthStrongAlternateStrong
#else
	#define LED0_DRIVE_STRENGTH		gpioDriveStrengthWeakAlternateWeak
	#define LED1_DRIVE_STRENGTH		gpioDriveStrengthWeakAlternateWeak
#endif


// System Clock setup
#define MCU_HFXO_FREQ			cmuHFRCOFreq_26M0Hz


// LETIMER PWM Configuration

#define		PWM_ROUTE_0			LETIMER_ROUTELOC0_OUT0LOC_LOC28
#define		PWM_ROUTE_1			LETIMER_ROUTELOC0_OUT1LOC_LOC28

// I2C port/pin configuration
#define		SI7021_SCL_PORT			gpioPortC
#define		SI7021_SCL_PIN			11u
#define		SI7021_SDA_PORT			gpioPortC
#define		SI7021_SDA_PIN			10u
#define		SI7021_SENSOR_EN_PORT	gpioPortB
#define		SI7021_SENSOR_EN_PIN	10u

#define		SI7021_EN_DEFAULT		true
#define		SI7021_DRIVE_STRENGTH	gpioDriveStrengthWeakAlternateWeak
#define		SI7021_SENSOR_GPIOMODE	gpioModePushPull
#define 	SI7021_SDA_SCL_GPIOMODE	gpioModeWiredAnd

// i2c Temperature sensor pin configuration
#define 	SI7021_SDA_ROUTE		I2C_ROUTELOC0_SDALOC_LOC15
#define 	SI7021_SCL_ROUTE		I2C_ROUTELOC0_SCLLOC_LOC15

/* UART Lab 5 */
#define		UART_RX_PORT			gpioPortD
#define		UART_RX_PIN				11u
#define		UART_RX_GPIOMODE		gpioModeInput
#define		UART_RX_EN_DEFUALT		false
#define		UART_TX_PORT			gpioPortD
#define		UART_TX_PIN				10u
#define 	UART_TX_GPIOMODE		gpioModePushPull
#define		UART_TX_DRIVESTRENGTH	gpioDriveStrengthStrongAlternateWeak
#define		UART_TX_EN_DEFUALT		true





//***********************************************************************************
// global variables
//***********************************************************************************


//***********************************************************************************
// function prototypes
//***********************************************************************************

#endif
