//***********************************************************************************
// Include files
//***********************************************************************************
#ifndef	APP_HG
#define	APP_HG

/* System include statements */


/* Silicon Labs include statements */
#include "em_cmu.h"
#include "em_assert.h"

/* The developer's include statements */
#include "cmu.h"
#include "gpio.h"
#include "letimer.h"
#include "brd_config.h"
#include "scheduler.h"
#include "Si7021.h"
#include "ble.h"
#include "HW_Delay.h"
#include <stdio.h>


//***********************************************************************************
// defined files
//***********************************************************************************
#define		PWM_PER				2.7		// PWM period in seconds
#define		PWM_ACT_PER			0.15	// PWM active period in seconds
#define 	LETIMER0_COMP0_CB	0x1		// 0b1 - COMP0 callback
#define 	LETIMER0_COMP1_CB	0x2		// 0b10 - COMP1 callback
#define 	LETIMER0_UF_CB		0x4		// 0b100 - Underflow callback
#define		I2C_7021_READ_CB	0x8		// 0b1000 - Callback upon completion of i2c read SM
#define		BOOT_UP_CB			0x10	// 0b10000 - Bootup callback
#define		BLE_TX_DONE_CB		0x20	// 0b100000 - BLE TX Done callback
#define     I2C_7021_WRITE_CB   0x40    // 0b1000000 - Callback upong completion of i2c write SM

#define 	SYSTEM_BLOCK_EM 	EM3


//***********************************************************************************
// global variables
//***********************************************************************************


//***********************************************************************************
// function prototypes
//***********************************************************************************
void app_peripheral_setup(void);
void scheduled_letimer0_uf_evt(void);
void scheduled_letimer0_comp0_evt(void);
void scheduled_letimer0_comp1_evt(void);
void scheduled_si7021_tempDone();
void scheduled_boot_up_cb(void);
void app_letimer_pwm_open(float period, float act_period, uint32_t out0_route, uint32_t out1_route);
void ble_tx_done_cb(void);
#endif
