/*
 * i2c.h
 *
 *  Created on: Sep 29, 2020
 *      Author: peski
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#ifndef SRC_HEADER_FILES_I2C_H_
#define SRC_HEADER_FILES_I2C_H_

/* System include statements */


/* Silicon Labs include statements */
#include "stdbool.h"
#include "em_i2c.h"
#include "gpio.h"
#include "sleep_routines.h"
#include "app.h"

/* The developer's include statements */



//***********************************************************************************
// defined files
//***********************************************************************************
#define 	I2C_EM_BLOCK		EM2  //first mode it cannot enter while in i2c

//***********************************************************************************
// global variables
//***********************************************************************************
typedef struct {
	// from I2C_Init_Typedef
	bool 					enable; 		//enable the i2c peripheral upon completion of open
	bool					master; 		//set the master (true) or slave(false)
	uint32_t				refFreq;		//I2C reference clocked assumed when configuring bus frequency setup
	uint32_t 				freq;			//(Max) I2C bus frequency to use. Only in master mode
	I2C_ClockHLR_TypeDef 	clhr;			//clock low_high ration control
	uint32_t				sda_pin_route;	//out sda route to gpio port/pin
	uint32_t				scl_pin_route;	//out scl route to gpio port/pin
	bool					sda_pin_en;		//enable sda route
	bool					scl_pin_en;		//enable scl route
} I2C_OPEN_STRUCT ;

typedef struct {
	I2C_TypeDef 	*i2c; 			//i2c peripheral being used
	uint32_t		current_state; 	//current state of state machine
	uint32_t		device_addr; 	//address of slave device being used
	uint32_t		command;		//command to be sent to slave device
	uint32_t		*i2c_reg;		//i2c register address being requested
	bool			readWrite;		//true: read, false: write
	uint32_t		*readData;		//pointer of where to store a read result or get write data
	uint32_t		numBytes;		//number of bytes to transfer
	bool			SMbusy;			//make sure the state machine doesn't restart when busy
	uint32_t		callback;		//the callback event at completion
} I2C_STATE_MACHINE_STRUCT;

//STATES FOR State Machine
typedef enum {
	INIT_SEND_ADDR,
	SEND_MEASURE_TEMP_NHOLD,
	SEND_RPT_START_ADDR,
	READ_MS_BYTE,
	READ_LS,
	STOP_END
} DEFINED_STATES;



//***********************************************************************************
// function prototypes
//***********************************************************************************
void i2c_open(I2C_TypeDef *i2c, I2C_OPEN_STRUCT *i2c_setup);
void i2c_bus_reset(I2C_TypeDef *i2c);
void I2C0_IRQHandler(void);
void I2C1_IRQHandler(void);
void i2c_start(I2C_TypeDef *i2c, uint32_t slaveAddr, uint32_t *data, uint32_t numBytes, uint32_t command, bool readWrite, uint32_t	scheduled_SO7021_READ_CB);


#endif /* SRC_HEADER_FILES_I2C_H_ */
