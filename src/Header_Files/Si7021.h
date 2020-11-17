/*
 * Si7021.h
 *
 *  Created on: Oct 6, 2020
 *      Author: peski
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#ifndef SRC_HEADER_FILES_SI7021_H_
#define SRC_HEADER_FILES_SI7021_H_

/* System include statements */
#include "i2c.h"
#include "app.h"
#include "brd_config.h"

/* Silicon Labs include statements */


//***********************************************************************************
// defined files
//***********************************************************************************
#define		FREQ_I2C			I2C_FREQ_FAST_MAX //400kHz I2C Frequency
#define		SLAVE_ADDR			0x40 //Si7021 default slave address
#define		MEASURE_TEMP_NHOLD	0xF3 //Measure temperature with No Hold

#define     si7021_I2C          I2C0
#define     READ_USER1_REG_CMD  0xE7
#define     USER1_RESET_REG     0b00111010 //expected user1 register upon reset of the si7021
#define     RH10_TEMP13         0b10111010 //RH resolution 10-bit, temp resolution 13 bit 


//***********************************************************************************
// global variables
//***********************************************************************************


//***********************************************************************************
// function prototypes
//***********************************************************************************

void si7021_i2c_open();
void si7021_read(uint32_t SI7021_READ_CB);
float tempConvert_si7021();
void si7021_TDD_config(void)

#endif /* SRC_HEADER_FILES_SI7021_H_ */
