/**
 * @file leuart.c
 * @author
 * @date
 * @brief Contains all the functions of the LEUART peripheral
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************

//** Standard Library includes
#include <string.h>

//** Silicon Labs include files
#include "em_gpio.h"
#include "em_cmu.h"

//** Developer/user include files
#include "leuart.h"
#include "scheduler.h"

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// private variables
//***********************************************************************************
uint32_t	rx_done_evt;
uint32_t	tx_done_evt;
bool		leuart0_tx_busy;
static 		LEUART_SM_STRUCT	leuart_sm;

/***************************************************************************//**
 * @brief LEUART driver
 * @details
 *  This module contains all the functions to support the driver's state
 *  machine to transmit a string of data across the LEUART bus.  There are
 *  additional functions to support the Test Driven Development test that
 *  is used to validate the basic set up of the LEUART peripheral.  The
 *  TDD test for this class assumes that the LEUART is connected to the HM-18
 *  BLE module.  These TDD support functions could be used for any TDD test
 *  to validate the correct setup of the LEUART.
 *
 ******************************************************************************/

//***********************************************************************************
// Private functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 *	TXBL Interrupt Handler for LEUART SM TX Sequence
 *
 * @details
 * 	The TXBL interrupt will handle the entirety of a TX data transfer for the
 * 	LEUART TX state machine. It will send data until the last bit and then will
 * 	go into the STOP_CLOSE state and wait for the TXC to be asserted.
 *
 * @note
 * 	TXBL interrupt is enabled at the last character being sent.
 *
 ******************************************************************************/
static void txbl_int(){
	switch(leuart_sm.current_state){
		case INIT_UART:
			// ready to begin sending data
			leuart_sm.current_state = SEND_DATA;
			break;
		case SEND_DATA:
			leuart_sm.leuart->TXDATA = leuart_sm.output[leuart_sm.count];
//			while(leuart_sm.leuart->SYNCBUSY);
			leuart_sm.count ++;
			if(leuart_sm.count == (leuart_sm.length)){
				leuart_sm.current_state = STOP_CLOSE;
				leuart_sm.leuart->IEN &= ~LEUART_IEN_TXBL;
				leuart_sm.leuart->IEN |= LEUART_IEN_TXC;
			}
			break;
		case STOP_CLOSE:
			EFM_ASSERT(false);
			break;
		default:
			EFM_ASSERT(false);
	}
}

/***************************************************************************//**
 * @brief
 *	TXC Interrupt Handler for LEUART SM TX Sequence
 *
 * @details
 * 	When TXC interrupt is enabled and it is then asserted, this indicated the
 * 	completion of a TX transfer. The SM will conclude and the device will return back
 * 	to the previous EM mode.
 *
 * @note
 * 	The tx_done_event scheduled event will be added.
 *
 ******************************************************************************/
static void txc_int(){
	switch(leuart_sm.current_state){
		case INIT_UART:
			EFM_ASSERT(false);
			break;
		case SEND_DATA:
			EFM_ASSERT(false);
			break;
		case STOP_CLOSE:
			leuart_sm.leuart->CMD |= LEUART_CMD_TXDIS;
//			while(leuart_sm.leuart->SYNCBUSY);
			add_scheduled_event(tx_done_evt);
			leuart_sm.leuart->IEN &= ~LEUART_IEN_TXC;
			sleep_unblock_mode(LEUART_TX_EM);
			leuart_sm.SMbusy = false;
			leuart_sm.current_state = INIT_UART;
			return;
		default:
			EFM_ASSERT(false);
	}
}


//***********************************************************************************
// Global functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 *	LEUART Init/Open Function
 *
 * @details
 * 	This function will open the given LEUART peripheral to the leuart_settings
 * 	passed. The LEUART clock will be enabled, pins will be routed, and the RX
 * 	and TX registers will be cleared. The IRQ handler will also be enabled.
 *
 * @note
 * 	The LEUART SM will be set to NOT busy when this function is called
 *
 * @param[in] *leuart
 * A pointer to the LEUART Peripheral to be opened.
 *
 * @param[in] *leuart_settings
 * A pointer to an LEUART_OPEN_STRUCT will all of the desired settings set.
 *
 ******************************************************************************/
void leuart_open(LEUART_TypeDef *leuart, LEUART_OPEN_STRUCT *leuart_settings){
	LEUART_Init_TypeDef leuartInit_struct;
	leuartInit_struct.enable = leuart_settings->enable;
	leuartInit_struct.refFreq = leuart_settings->refFreq;
	leuartInit_struct.baudrate = leuart_settings->baudrate;
	leuartInit_struct.databits = leuart_settings->databits;
	leuartInit_struct.parity = leuart_settings->parity;
	leuartInit_struct.stopbits = leuart_settings->stopbits;

	if(leuart == LEUART0) CMU_ClockEnable(cmuClock_LEUART0, true);
	if ((leuart->IF & 0x01) == 0) {
		leuart->IFS = 0x01;
		EFM_ASSERT(leuart->IF & 0x01);
		leuart->IFC = 0x01;
	} else {
		leuart->IFC = 0x01;
		EFM_ASSERT(!(leuart->IF & 0x01));
	} //verify clock tree is enabled

	leuart->ROUTELOC0 = leuart_settings->rx_loc;
	leuart->ROUTELOC0 |= leuart_settings->tx_loc;
	leuart->ROUTEPEN  = (LEUART_ROUTEPEN_RXPEN * leuart_settings->rx_pin_en);
	leuart->ROUTEPEN  |=	(LEUART_ROUTEPEN_TXPEN * leuart_settings->tx_pin_en);

	rx_done_evt = leuart_settings->rx_done_evt;
	tx_done_evt = leuart_settings->tx_done_evt;
	leuart_sm.SMbusy = false;

	LEUART_Init(leuart, &leuartInit_struct) ;
	leuart_cmd_write(HM10_LEUART0, (LEUART_CMD_CLEARRX | LEUART_CMD_CLEARTX));
	LEUART_Enable(leuart, leuart_settings->enable);
	while(!((leuart->STATUS & LEUART_STATUS_RXENS)& leuart_settings->rx_en) && !((leuart->STATUS & LEUART_STATUS_TXENS) & leuart_settings->tx_en));

	if(leuart == LEUART0) NVIC_EnableIRQ(LEUART0_IRQn);
}

/***************************************************************************//**
 * @brief
 *	LEUART0 IRQ Handler
 *
 * @details
 * 	Handles the interrupts of TXBL and TXC to implement the state machine in
 * 	operation.
 *
 * @note
 * This function will clear all LEUART0 interrupts
 ******************************************************************************/
//
void LEUART0_IRQHandler(void){
	uint32_t int_flag = LEUART0->IF & LEUART0->IEN;
	LEUART0->IFC = int_flag;

	if(int_flag & LEUART_IF_TXBL){
		txbl_int();
	}
	if(int_flag & LEUART_IF_TXC){
		txc_int();
	}
}

/***************************************************************************//**
 * @brief
 *	Start an LEUART transmission
 *
 * @details
 *  This function will initialize a TX transfer of the given string across the
 *  given LEUART peripheral. The TXBL and TXC interrupts are used and the state
 *  machine will send the first byte when TXBL IF is asserted.
 *
 * @note
 * If a current transmission is active, the function will wait for it to finish
 * before beginning a new one
 *
 * @param[in] *leuart
 * A poiner to the LEUART peripheral to be used
 *
 * @param[in] *string
 * A pointer to the string to be sent.
 *
 * @param[in] string_len
 * The number of characters to be sent.
 ******************************************************************************/

void leuart_start(LEUART_TypeDef *leuart, char *string, uint32_t string_len){
	while(leuart_tx_busy(leuart)); //stall if  busy
	CORE_DECLARE_IRQ_STATE;
	CORE_ENTER_CRITICAL();


	leuart_sm.leuart = leuart;
	leuart_sm.length = string_len;
	leuart_sm.count = 0;
	strcpy(leuart_sm.output, string);
	leuart_sm.SMbusy = true;
	sleep_block_mode(LEUART_TX_EM);

	leuart_sm.leuart->CMD |= LEUART_CMD_TXEN;
	leuart->IEN |= LEUART_IEN_TXBL;

	CORE_EXIT_CRITICAL();

}

/***************************************************************************//**
 * @brief
 *	LEUART TX Busy Check
 *
 * @details
 * Will return true if TX SM is busy. Will return false if not.
 *
 * @note
 *	Checks SMbusy from LEUART_SM
 *
 * @param[in] *leuart
 * A pointer to the LEUART peripheral to be checked
 ******************************************************************************/

bool leuart_tx_busy(LEUART_TypeDef *leuart){
	return leuart_sm.SMbusy;
}


/***************************************************************************//**
 * @brief
 *   LEUART STATUS function returns the STATUS of the peripheral for the
 *   TDD test
 *
 * @details
 * 	 This function enables the LEUART STATUS register to be provided to
 * 	 a function outside this .c module.
 *
 * @param[in] *leuart
 *   Defines the LEUART peripheral to access.
 *
 * @return
 * 	 Returns the STATUS register value as an uint32_t value
 *
 ******************************************************************************/

uint32_t leuart_status(LEUART_TypeDef *leuart){
	uint32_t	status_reg;
	status_reg = leuart->STATUS;
	return status_reg;
}

/***************************************************************************//**
 * @brief
 *   LEUART CMD Write sends a command to the CMD register
 *
 * @details
 * 	 This function is used by the TDD test function to program the LEUART
 * 	 for the TDD tests.
 *
 * @note
 *   Before exiting this function to update  the CMD register, it must
 *   perform a SYNCBUSY while loop to ensure that the CMD has by synchronized
 *   to the lower frequency LEUART domain.
 *
 * @param[in] *leuart
 *   Defines the LEUART peripheral to access.
 *
 * @param[in] cmd_update
 * 	 The value to write into the CMD register
 *
 ******************************************************************************/

void leuart_cmd_write(LEUART_TypeDef *leuart, uint32_t cmd_update){

	leuart->CMD = cmd_update;
	while(leuart->SYNCBUSY);
}

/***************************************************************************//**
 * @brief
 *   LEUART IF Reset resets all interrupt flag bits that can be cleared
 *   through the Interrupt Flag Clear register
 *
 * @details
 * 	 This function is used by the TDD test program to clear interrupts before
 * 	 the TDD tests and to reset the LEUART interrupts before the TDD
 * 	 exits
 *
 * @param[in] *leuart
 *   Defines the LEUART peripheral to access.
 *
 ******************************************************************************/

void leuart_if_reset(LEUART_TypeDef *leuart){
	leuart->IFC = 0xffffffff;
}

/***************************************************************************//**
 * @brief
 *   LEUART App Transmit Byte transmits a byte for the LEUART TDD test
 *
 * @details
 * 	 The BLE module will respond to AT commands if the BLE module is not
 * 	 connected to the phone app.  To validate the minimal functionality
 * 	 of the LEUART peripheral, write and reads to the LEUART will be
 * 	 performed by polling and not interrupts.
 *
 * @note
 *   In polling a transmit byte, a while statement checking for the TXBL
 *   bit in the Interrupt Flag register is required before writing the
 *   TXDATA register.
 *
 * @param[in] *leuart
 *   Defines the LEUART peripheral to access.
 *
 * @param[in] data_out
 *   Byte to be transmitted by the LEUART peripheral
 *
 ******************************************************************************/

void leuart_app_transmit_byte(LEUART_TypeDef *leuart, uint8_t data_out){
	while (!(leuart->IF & LEUART_IF_TXBL));
	leuart->TXDATA = data_out;
}


/***************************************************************************//**
 * @brief
 *   LEUART App Receive Byte polls a receive byte for the LEUART TDD test
 *
 * @details
 * 	 The BLE module will respond to AT commands if the BLE module is not
 * 	 connected to the phone app.  To validate the minimal functionality
 * 	 of the LEUART peripheral, write and reads to the LEUART will be
 * 	 performed by polling and not interrupts.
 *
 * @note
 *   In polling a receive byte, a while statement checking for the RXDATAV
 *   bit in the Interrupt Flag register is required before reading the
 *   RXDATA register.
 *
 * @param[in] leuart
 *   Defines the LEUART peripheral to access.
 *
 * @return
 * 	 Returns the byte read from the LEUART peripheral
 *
 ******************************************************************************/
uint8_t leuart_app_receive_byte(LEUART_TypeDef *leuart){
	uint8_t leuart_data;
	while (!(leuart->IF & LEUART_IF_RXDATAV));
	leuart_data = leuart->RXDATA;
	return leuart_data;
}
