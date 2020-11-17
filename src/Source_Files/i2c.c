/**
 * @file i2c.c
 * @author Connor Peskin
 * @date October 16, 2020
 * @brief Driver for I2c operation. Implements interrupt-driven I2C
 * communication.
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "i2c.h"

//***********************************************************************************
// defined files
//***********************************************************************************
static I2C_STATE_MACHINE_STRUCT i2c_sm;
static uint32_t	scheduled_Si7021_READ_CB;
static uint32_t scheduled_Si7021_WRITE_CB;

//***********************************************************************************
// Private variables
//***********************************************************************************

/***************************************************************************//**
 * @brief
 * 	ACK interrupt Handler for I2C state machine read sequence
 *
 * @details
 * 	Contains processes for data Read transmission for SM states INIT_SEND_ADDR,
 * 	SEND_MEASURE_TEMP_NHOLD, and SEND_RPT_START_ADDR.
 * 	This will transmit device_addr and command from private I2C_STATE_MACHINE_STRUCT
 * 	struct.
 *
 * @note
 *	If this interrupt handler is entered from states READ_MS_BYTE, READ_LS, or
 *	STOP_END, a false assert will be called
 *
 ******************************************************************************/
static void ack_int(){
	if(i2c_sm->readWrite == true){
		switch(i2c_sm.current_state){
			case INIT_SEND_ADDR_R:
				i2c_sm.i2c->TXDATA = i2c_sm.command;
				i2c_sm.current_state = SEND_MEASURE_TEMP_NHOLD;
				break;
			case SEND_CMD_R:
				i2c_sm.i2c->CMD = I2C_CMD_START;
				i2c_sm.readWrite = true;
				i2c_sm.i2c->TXDATA = (i2c_sm.command << 1) | i2c_sm.readWrite;
				i2c_sm.current_state = SEND_RPT_START_ADDR;
				break;
			case QUERY_READY:
				i2c_sm.current_state = READ_MS_BYTE;
				i2c_sm.i2c->IEN |= I2C_IEN_RXDATAV;//rxdatav interrupt
				break;
			case READ_DATA:
				EFM_ASSERT(false); // shouldn't be here
				break;
			case STOP_END:
				EFM_ASSERT(false); //shouldn't be here
				break;
			default:
				EFM_ASSERT(false);
		}	
	}
	if(i2c_sm->readWrite == false){
		switch(i2c_sm.current_state){
			case INIT_SEND_ADDR_W:
				break;
			case SEND_CMD_W:
				break;
			case SEND_DATA:
				break;
			case WAIT_COMPLETE:
				break;
			case STOP_END:
				break;
			default:
				EFM_ASSERT(false);
		}
	}
	
}

/***************************************************************************//**
 * @brief
 * 	NACK interrupt Handler for I2C state machine read sequence
 *
 * @details
 * 	Contains processes for data Read transmission for only the SM state called
 * 	SEND_RPT_START_ADDR.
 * 	Will transmit device_addr from private I2C_STATE_MACHINE_STRUCT struct.
 *
 * @note
 *	If this interrupt handler is entered from any state other than
 *	SEND_RPT_START_ADDR, a false assert statement will be called.
 *
 ******************************************************************************/
static void nack_int() {
	if(i2c_sm->readWrite == true){
		switch(i2c_sm.current_state){
			case INIT_SEND_ADDR:
				EFM_ASSERT(false); //shouldn't be here
				break;
			case SEND_CMD_R:
				EFM_ASSERT(false); //shouldn't be here
				break;
			case QUERY_READY:  //only state the SM should be in when receive NACK
				i2c_sm.i2c->CMD = I2C_CMD_START;
				i2c_sm.readWrite = true;
				i2c_sm.i2c->TXDATA = (i2c_sm.device_addr << 1) | i2c_sm.readWrite;
				break;
			case READ_DATA:
				EFM_ASSERT(false); //shouldn't be here
				break;
			case STOP_END:
				EFM_ASSERT(false); //shouldn't be here
				break;
			default:
				EFM_ASSERT(false);
		}	
	}
	if(i2c_sm->readWrite == false){
		switch(i2c_sm.current_state){
			case INIT_SEND_ADDR_W:
				break;
			case SEND_CMD_W:
				break;
			case SEND_DATA:
				break;
			case WAIT_COMPLETE:
				break;
			case STOP_END:
				break;
			default:
				EFM_ASSERT(false);
		}
	}
}

/***************************************************************************//**
 * @brief
 * 	RXDATAV interrupt Handler for I2C state machine read sequence
 *
 * @details
 * 	Contains processes for data Read transmission for SM states READ_MS_BYTE,
 * 	and READ_LS_BYTE.
 * 	Will store the data from the RXDATA buffer into the pointer from the
 * 	I2C_STATE_MACHINE_STRUCT, *readData. It stores MS byte first.
 *
 * @note
 *	If this interrupt handler is entered from any state other than READ_MS_BYTE
 *	or READ_LS, a false assert statement will be called.
 *
 ******************************************************************************/
static void rxdatav_int(){
	switch(i2c_sm.current_state){
		case INIT_SEND_ADDR:
			EFM_ASSERT(false); //shouldn't be here
			break;
		case SEND_MEASURE_TEMP_NHOLD:
			EFM_ASSERT(false); //shouldn't be here
			break;
		case SEND_RPT_START_ADDR:
			EFM_ASSERT(false); //shouldn't be here
			break;
		case READ_MS_BYTE:
			*i2c_sm.readData = (i2c_sm.i2c->RXDATA << 8); // Read the MS byte from RXDATA
			i2c_sm.i2c->CMD |= I2C_CMD_ACK; // ack to confirm it has been read
			i2c_sm.current_state = READ_LS;
			break;
		case READ_LS:
			*i2c_sm.readData |= (i2c_sm.i2c->RXDATA); // receive LS byte into shifted readData variable
			i2c_sm.i2c->CMD |= I2C_CMD_NACK;
			i2c_sm.i2c->CMD = I2C_CMD_STOP;
//			i2c_bus_reset(i2c_sm.i2c);
			i2c_sm.current_state = STOP_END;
			break;
		case STOP_END:
			EFM_ASSERT(false); //shouldn't be here
			break;
		default:
			EFM_ASSERT(false);
	}
}

/***************************************************************************//**
 * @brief
 * 	MSTOP interrupt Handler for I2C state machine read sequence
 *
 * @details
 * 	Contains processes for data Read transmission for only SM state STOP_END.
 * 	The blocked energy mode by I2C, I2C_EM_BLOCK, will be unblocked. A scheduled
 * 	event will be added from the I2C_STATE_MACHINE_STRUCT element callback.
 * 	The I2C bus will be reset and the SMbusy variable will be set to false.
 *
 * @note
 *	If this interrupt handler is entered from any state other than STOP_END,
 *	a false assert statement will be called.
 *
 ******************************************************************************/
static void mstop_int(){
	if(i2c_sm->readWrite == true){
		switch(i2c_sm.current_state){
			case INIT_SEND_ADDR:
				EFM_ASSERT(false); //shouldn't be here
				break;
			case SEND_MEASURE_TEMP_NHOLD:
				EFM_ASSERT(false); //shouldn't be here
				break;
			case SEND_RPT_START_ADDR:
				EFM_ASSERT(false); //shouldn't be here
				break;
			case READ_MS_BYTE:
				EFM_ASSERT(false); //shouldn't be here
				break;
			case READ_LS:
				EFM_ASSERT(false); //shouldn't be here
				break;
			case STOP_END:
				i2c_sm.SMbusy = false;
				sleep_unblock_mode(I2C_EM_BLOCK);
				add_scheduled_event(i2c_sm.callback);
				i2c_sm.current_state = INIT_SEND_ADDR;
				break;
			default:
				EFM_ASSERT(false);
		}	
	}
	if(i2c_sm->readWrite == false){
		switch(i2c_sm.current_state){
			case INIT_SEND_ADDR_W:
				break;
			case SEND_CMD_W:
				break;
			case SEND_DATA:
				break;
			case WAIT_COMPLETE:
				break;
			case STOP_END:
				break;
			default:
				EFM_ASSERT(false);
		}
	}
}


/***************************************************************************//**
 * @brief
 * 	TXBL Interrupt Handler for Transmit I2C State Machine
 *
 * @details
 * 	
 *
 * @note
 *	
 *	
 ******************************************************************************/
static void txbl_int(){
    
}

//***********************************************************************************
// Private functions
//***********************************************************************************


//***********************************************************************************
// Global functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 *	Function to initialize the I2C peripheral
 *
 * @details
 *	Compatible with both I2C0 and I2C1. This function will open the peripheral by
 *	enabling clocks, setting values passed by I2C_OPEN_STRUCT i2c_setup, and enabling
 *	interrupts.
 *
 * @note
 *	Interrupt implementation currently only for Read SM.
 *
 * @param[in] I2C_TypeDef
 * I2C peripheral to initialize.
 *
 * @param[in] I2C_OPEN_STRUCT
 * I2C_OPEN_STRUCT that contains all configuration options for I2C init.
 *
 ******************************************************************************/
void i2c_open(I2C_TypeDef *i2c, I2C_OPEN_STRUCT *i2c_setup){
	I2C_Init_TypeDef i2c_init_values;
	if(i2c == I2C0) CMU_ClockEnable(cmuClock_I2C0, true);
	if(i2c == I2C1) CMU_ClockEnable(cmuClock_I2C1, true);

	i2c_init_values.enable = i2c_setup->enable;
	i2c_init_values.master = i2c_setup->master;
	i2c_init_values.refFreq = i2c_setup->refFreq;
	i2c_init_values.freq = i2c_setup->freq;
	i2c_init_values.clhr = i2c_setup->clhr;

	if ((i2c->IF & 0x01) == 0) {
		i2c->IFS = 0x01;
		EFM_ASSERT(i2c->IF & 0x01);
		i2c->IFC = 0x01;
	} else {
		i2c->IFC = 0x01;
		EFM_ASSERT(!(i2c->IF & 0x01));
	}

    i2c->ROUTELOC0 = i2c_setup->sda_pin_route /* << _I2C_ROUTELOC0_SDALOC_SHIFT*/;
    i2c->ROUTELOC0|= i2c_setup->scl_pin_route /*<< _I2C_ROUTELOC0_SCLLOC_SHIFT*/;
    i2c->ROUTEPEN = (I2C_ROUTEPEN_SCLPEN * i2c_setup->scl_pin_en);
    i2c->ROUTEPEN |= (I2C_ROUTEPEN_SDAPEN * i2c_setup->sda_pin_en);
	I2C_Init(i2c, &i2c_init_values);

	i2c_bus_reset(i2c);
	/* Enabling Interrupts */
	i2c->IFC  = i2c->IF;		//clear interrupt flag register
	i2c->IEN  = I2C_IEN_ACK;  	//ack interrupt
	i2c->IEN |= I2C_IEN_NACK;	//nack interrupt
	i2c->IEN |= I2C_IEN_MSTOP;  //mstop interrupt

	scheduled_Si7021_READ_CB = I2C_7021_READ_CB;
	scheduled_Si7021_WRITE_CB = I2C_7021_WRITE_CB;

	if(i2c == I2C0) NVIC_EnableIRQ(I2C0_IRQn);
	if(i2c == I2C1) NVIC_EnableIRQ(I2C1_IRQn);
}

/***************************************************************************//**
 * @brief
 *	I2C Bus Reset Function
 *
 * @details
 *	Resets the desired I2C bus using START | STOP command method. This function will
 *	also clear all interrupt flags & TXBUFER, and RXBUFFER, then abort.
 *
 * @note
 *	This does not use the 9 NACK reset method and, consequently, cannot be used
 *	while in the middle of a transmission
 *
 * @param[in] I2C_TypeDef
 *	I2C peripheral to have bus reset.
 *
 ******************************************************************************/
void i2c_bus_reset(I2C_TypeDef *i2c){
	// save IEN state and disable during bus reset operation
	uint32_t IENstate;
	IENstate = i2c->IEN;
	i2c->IEN = false;

	if(i2c->STATE & I2C_STATE_BUSY){
		i2c->CMD = I2C_CMD_ABORT;
		while(i2c->STATE & I2C_STATE_BUSY);
	}

	i2c->IFC = i2c->IF; //clear interrupt flags
	i2c->CMD = I2C_CMD_CLEARTX; //clear the transmit buffe
	i2c->CMD = I2C_CMD_START | I2C_CMD_STOP; //transmit START immediately followed by STOP
	while(!(i2c->IF & I2C_IF_MSTOP)); //check MSTOP to verify the reset occured
	i2c->CMD = I2C_CMD_ABORT;

	// reset I2C state machine
	i2c_sm.SMbusy = false;
	i2c_sm.current_state = 0; // reset to first state. 0- first due to enum

	// clear IF and re-enable IEN state
	i2c->IFC = i2c->IF & ~I2C_IF_MSTOP;
	i2c->CMD = I2C_CMD_ABORT;
	i2c->IEN = IENstate;
}

/***************************************************************************//**
 * @brief
 *	Start an I2C Read/Write Transmission
 *
 * @details
 *	This function will begin a Master mode transmission initialize the state
 *	machine to the initial value, block the I2C_EM_BLOCK energy mode, and transfer
 *	the slave address with the read/write bit set by input variable readWrite.
 *
 * @note
 *	I2C_TypeDef The I2C START command will be sent in this function.
 *
 * @param[in] I2C_TypeDef
 *	The desired I2C peripheral to use.
 *
 * @param[in] uint32_t
 *	The 7 bit address of the desired SLAVE device *
 *
 * @param[in] uint32_t
 *	A pointer to where the transmission data will be stored.
 *
 * @param[in] uint32_t
 *	The number of bytes to be transmitted.
 *
 * @param[in] uint32_t
 *	The command to be transferred to the target device
 *
 * @param[in] bool
 *	A boolean variable indicating read/Write operation. 1-Read, 0- Write
 *
 * @param[in] uint32_t
 *	The callback event bit for the scheduler and to be used in the main.c while(1)
 *	loop.
 *
 ******************************************************************************/
void i2c_start(I2C_TypeDef *i2c, uint32_t slaveAddr, uint32_t *data, uint32_t numBytes, uint32_t command, bool readWrite, uint32_t	cb_event){
	EFM_ASSERT((i2c->STATE & _I2C_STATE_STATE_MASK) == I2C_STATE_STATE_IDLE); //make sure i2c is ready
	sleep_block_mode(I2C_EM_BLOCK); //make sure it doesn't go into the lowest available sleep state
	/* initialize the state machine struct */
	i2c_sm.current_state = 0; //this is a start function, so first state
	i2c_sm.device_addr = slaveAddr;
	i2c_sm.i2c = i2c;
	i2c_sm.command = command;
	i2c_sm.readWrite = readWrite;
	i2c_sm.readData = data;
	i2c_sm.numBytes = numBytes;
	i2c_sm.bytesDone = 0; //no bytes have been sent/recieved so far
	i2c_sm.SMbusy = true;
	i2c_sm.callback = cb_event;
	/* Here I turn off AUTOACK because after reading from the RX Buffer we may need to send a NACK.  */
	i2c_sm.i2c->CTRL &= ~I2C_CTRL_AUTOACK;

	i2c_sm.i2c->TXDATA = (slaveAddr << 1) | readWrite; // populate the txbus with the address to move forward with SM operation
	i2c_sm.i2c->CMD = I2C_CMD_START; // send start command, will also transmit address
}

/***************************************************************************//**
 * @brief
 *	I2C0 IRQ Handler
 *
 * @details
 *	Handles the interrupts of ACK, NACK, MSTOP, and RXDATAV to implement the
 *	STATE MACHINE in operation
 *
 * @note
 *	Currently only implemented for Read SM operation.
 *
 ******************************************************************************/
void I2C0_IRQHandler(void){
	uint32_t int_flag = I2C0->IF & I2C0->IEN;
	I2C0->IFC = int_flag;

	if(int_flag & I2C_IF_ACK){
		ack_int();
	}
	if(int_flag & I2C_IF_NACK){
		nack_int();
	}
	if(int_flag & I2C_IF_MSTOP){
		mstop_int();
	}
	if(int_flag & I2C_IF_RXDATAV){
		rxdatav_int();
	}
	if(int_flag & I2C_IF_TXBL){
		txbl_int(); //TODO
	}
	if(int_flag & I2C_IF_TXC){
		txc_int(); //TODO
	}

}

/***************************************************************************//**
 * @brief
 *	I2C1 IRQ Handler
 *
 * @details
 *	Handles the interrupts of ACK, NACK, MSTOP, and RXDATAV to implement the
 *	STATE MACHINE in operation
 *
 * @note
 *	Currently only implemented for Read SM operation.
 *
 ******************************************************************************/
void I2C1_IRQHandler(void){
	uint32_t int_flag = I2C1->IF & I2C1->IEN;
	I2C1->IFC = int_flag;

	if(int_flag & I2C_IF_ACK){
		ack_int();
	}
	if(int_flag & I2C_IF_NACK){
		nack_int();
	}
	if(int_flag & I2C_IF_MSTOP){
		mstop_int();
	}
	if(int_flag & I2C_IF_RXDATAV){
		rxdatav_int();
	}

}

bool i2c_sm_busy(I2C_TypeDef *i2c){
	if(i2c == i2c_sm.i2c) return i2c_sm.SMbusy;
	return false;
}
