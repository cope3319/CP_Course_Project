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
	uint32_t numBytes = 2; // 2 bytes, 13 bit 
	bool readWrite = true;  // we will be reading temp 
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

/***************************************************************************//**
 * @brief
 *	Test Driven Development Routine for the Si7021 
 *	
 * @details
 *  This TDD routine will test the I2C drivers and the verify the functionality 
 *  of the Si7021. Specifically, this will test single-byte read/write and double
 *  byte read from the Si7021. In this test, the output resolution for RH and 
 *  temperature will be adjusted to 10 and 13 respectively. 
 *
 * @note
 *  The I2C driver should support read/writes of at least 4 bytes (32 bit). 
 *  
 * ******************************************************************************/
void si7021_TDD_config(void){
	timer_delay(80u); // make sure that the device is booted up off reset
	/* Perform a single-byte read of the User 1 register on the SI 7021.*/
	uint32_t data = 0x0;
	bool readWrite = true;
	i2c_start(si7021_I2C, SLAVE_ADDR, &data,  1, READ_USER1_REG_CMD, readWrite, 0); // perform single byte read
	while(i2c_sm_busy(si7021_I2C)); //wait for TX oper
	EFM_ASSERT(data == USER1_RESET_REG); // Validate that the register is the expected reset value

	/* configure the si7021 user 1 register by performing a single-byte write) */
	data = RH10_TEMP13;
	readWrite = false; // will be a write
	i2c_start(si7021_I2C, SLAVE_ADDR, &data, 1, WRITE_USER1_REG_CMD, readWrite, 0);
	while(i2c_sm_busy(si7021_I2C));
	timer_delay(80u);

	/* now we want to verify that we successfully changed to a 13-bit temp read resolution*/
	data= 0x0;
	readWrite = true; //will be reading
	i2c_start(si7021_I2C, SLAVE_ADDR, &data,  1, READ_USER1_REG_CMD, readWrite, 0); // perform single byte read
	while(i2c_sm_busy(si7021_I2C)); //wait for TX oper
	EFM_ASSERT(data == RH10_TEMP13); // Validate that the register is the expected modified value

	//now perform a read and ensure that it's an accurate 
	reading = 0x0;
	readWrite = true;
	i2c_start(si7021_I2C, SLAVE_ADDR, &reading, 2, MEASURE_TEMP_NHOLD, readWrite);
	while(i2c_sm_busy(si7021_I2C));
	float temp = tempConvert_si7021();
	if((temp > 90) || (temp < 60)) EFM_ASSERT(false); // asserts if out of general expected range

	// PASSED TDD Configuration Test
	ble_write("\nPassed SI7021 TDD Test\n");
}

