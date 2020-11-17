/**
 * @file Si7021.c
 * @author Connor Peskin
 * @date October 16, 2020
 * @brief Functions to interact with the si7021 temperature/humidity sensor.
 * Currently only implements Temperature read.
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "Si7021.h"


//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// Private variables
//***********************************************************************************
static uint32_t reading;

//***********************************************************************************
// Private functions
//***********************************************************************************



//***********************************************************************************
// Global functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 *	SI7021 Open for Operation function
 *
 * @details
 *	Creates an I2C_OPEN_STRUCT for I2C operation with the SI7021 module
 *	onboard the PG12 Starter Kit. Opens this Device for I2C operation.
 *
 * @note
 *	Will use the I2C0 peripheral.
 *
 ******************************************************************************/
void si7021_i2c_open(){
	I2C_OPEN_STRUCT i2c_setup;
	i2c_setup.enable = true; //enable i2c operation
	i2c_setup.master = true; //we are operating as master
	i2c_setup.refFreq = 0; //current configured reference clock will be used
	i2c_setup.freq = FREQ_I2C; //will be just below the Si7021 max of 400kHz
	i2c_setup.clhr =  i2cClockHLRAsymetric; // 6:3 ratio
	i2c_setup.sda_pin_route = SI7021_SDA_ROUTE; // route to LOC15 i2c
	i2c_setup.scl_pin_route = SI7021_SCL_ROUTE; // route to LOC15 i2c
	i2c_setup.sda_pin_en = true;
	i2c_setup.scl_pin_en = true;

	i2c_open(I2C0, &i2c_setup);
}

/***************************************************************************//**
 * @brief
 *	Read Temperature from SI7021
 *
 * @details
 *	Calls for I2C to start a Temperature Read from the SI7021 temperature
 *	sensor.
 *
 * @note
 *	Stores the device transmission data in the private 'reading' variable.
 *
 ******************************************************************************/
void si7021_read(uint32_t read_cb){
	uint32_t numBytes = 2; // 16 bites.... but does it matter ;)
	bool readWrite = false;  // we will be writing first
	i2c_start(I2C0, SLAVE_ADDR, &reading, numBytes, MEASURE_TEMP_NHOLD, readWrite, read_cb);
}

/***************************************************************************//**
 * @brief
 *	Converts Temperature Reading to deg F
 *
 * @details
 *	Uses the private variable 'reading' to calculate deg C then convert it into
 *	deg F to get a decimal temperature reading.
 *
 * @note
 *	This should only be called from the scheduler callback function- after a read
 *	transmission has completed.
 *
 ******************************************************************************/
float tempConvert_si7021(){
	float temp = ((175.72 * (float)reading) / 65536) - 46.85; // conversion to C
	temp = (temp * (9.0/5.0)) + 32; //conversion
 	return temp;
}



