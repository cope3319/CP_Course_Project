/**
 * @file ble.c
 * @author
 * @date
 * @brief Contains all the functions to interface the application with the HM-18
 *   BLE module and the LEUART driver. Uses a circular buffer to store data going
 *   out.
 *
 */


//***********************************************************************************
// Include files
//***********************************************************************************
#include "ble.h"
#include <string.h>

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// private variables
//***********************************************************************************

#define CSIZE 64
typedef struct {
	char cbuf[CSIZE];
	uint8_t size_mask;
	uint32_t size;
	uint32_t read_ptr;
	uint32_t write_ptr;
} BLE_CIRCULAR_BUF;

#define CIRC_TEST_SIZE 3
typedef struct {
	char test_str[CIRC_TEST_SIZE][CSIZE];
	char result_str[CSIZE];
}CIRC_TEST_STRUCT;

static CIRC_TEST_STRUCT		test_struct;
static BLE_CIRCULAR_BUF		ble_cbuf;
/***************************************************************************//**
 * @brief BLE module
 * @details
 *  This module contains all the functions to interface the application layer
 *  with the HM-18 Bluetooth module.  The application does not have the
 *  responsibility of knowing the physical resources required, how to
 *  configure, or interface to the Bluetooth resource including the LEUART
 *  driver that communicates with the HM-18 BLE module.
 *
 ******************************************************************************/

//***********************************************************************************
// Private functions
//***********************************************************************************
static void ble_circ_init(void);
static void ble_circ_push(char *string);

static uint8_t ble_circ_space(void);
static void update_circ_wrtindex(BLE_CIRCULAR_BUF *index_struct, uint32_t update_by);
static void update_circ_readindex(BLE_CIRCULAR_BUF *index_struct, uint32_t update_by);

//***********************************************************************************
// Global functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 *	Update Circular Buffer Write Index
 *
 * @details
 *	Updates the circular buffer write index. The write index will update by the amount
 *	given in 'update_by' and will circle around if neccesary.
 *
 * @note
 *	Index implementation is being used rather than pointer implementation
 *
 * @param[in] *index_struct
 * A pointer to the CLE_CIRCULAR_BUF struct to update
 *
 * @param[in] update_by
 * The integer amount to update the BLE write index by.
 *
 ******************************************************************************/
void update_circ_wrtindex(BLE_CIRCULAR_BUF *index_struct, uint32_t update_by){
	index_struct->write_ptr += update_by;
	index_struct->write_ptr &= index_struct->size_mask;
}

/***************************************************************************//**
 * @brief
 *	Update Circular Buffer Read Index
 *
 * @details
 *	Updates the circular buffer read index. The read index will be updated by
 *	the amount given in 'update_by' and will circle around if neccesary.
 *
 * @note
 *	Index implementation is used rather than pointer implementation.
 *
 * @param[in] *index_struct
 * A pointer to the CLE_CIRCULAR_BUF struct to update
 *
 * @param[in] update_by
 * The integer amount to update the BLE read index by.
 *
 *
 ******************************************************************************/
void update_circ_readindex(BLE_CIRCULAR_BUF *index_struct, uint32_t update_by){
	index_struct->read_ptr += update_by;
	index_struct->read_ptr &= index_struct->size_mask;
}

/***************************************************************************//**
 * @brief
 *	Initialize Circular Buffer
 *
 * @details
 *	Initializes all values of the private circular buffer being used for LEUART
 *	operation.
 *
 * @note
 *	CSIZE variable defines the size of the buffer.
 *
 *
 ******************************************************************************/
static void ble_circ_init(void){
	ble_cbuf.read_ptr = 0;
	ble_cbuf.write_ptr = 0;
	ble_cbuf.size = CSIZE;
	ble_cbuf.size_mask = CSIZE - 1;
}

/***************************************************************************//**
 * @brief
 *	Push a packet onto the circular buffer
 *
 * @details
 *	Will push the given string onto the circular buffer and update the write
 *	index accordingly. The packet header will be the string length.
 *	Will not allow circular buffer to overflow.
 *
 * @note
 * Will assert false if the circular buffer does not have enough space
 *
 * @param[in] string
 * The string to be pushed onto the circular buffer.
 *
 ******************************************************************************/
static void ble_circ_push(char *string){
	uint8_t space = ble_circ_space();
	uint32_t length = strlen(string);
	if((space - length - 1) < 0) EFM_ASSERT(false);
	uint8_t n = 0;
	ble_cbuf.cbuf[ble_cbuf.write_ptr] = length + 1;
	update_circ_wrtindex(&ble_cbuf, 1);
	while(n < length){
		ble_cbuf.cbuf[ble_cbuf.write_ptr] = string[n];
		update_circ_wrtindex(&ble_cbuf, 1);
		n++;
	}
}

/***************************************************************************//**
 * @brief
 *	Pop a packet from the circular buffer.
 *
 * @details
 *	Will get the packet length from the first read index then pull the full packet
 *	from the circular buffer. Depending on the passed variable test, the data from
 *	the circular buffer will be:
 *	-test=true -> stored in test_struce.result_str[] var
 *	-test=false -> sent to the HM18 peripheral using leuart_start
 *
 * @note
 *	Will return true and exit if the TX SM is busy.
 *
 * @param[in] test
 *	Defines what will happen with the data pulled from the circular buffer. If false,
 *	data is sent via LEUART.
 *
 *	@return
 *	Returns false if data was popped off buffer, true if no data was popped.
 *
 ******************************************************************************/
bool ble_circ_pop(bool test){
	if(leuart_tx_busy(LEUART0)) return true;
	if(ble_circ_space() == CSIZE) return true;
	uint8_t length = ble_cbuf.cbuf[ble_cbuf.read_ptr];
	update_circ_readindex(&ble_cbuf, 1);
	uint8_t n = 0;
	char str[length + 1];
	if(test == true){
		memset(test_struct.result_str, 0 , CSIZE);
		while( n < (length-1)){
			test_struct.result_str[n] = ble_cbuf.cbuf[ble_cbuf.read_ptr];
			update_circ_readindex(&ble_cbuf, 1);
			n++;
		}
		return false;
	}
	else{
		while( n < (length-1) ){
			str[n] = ble_cbuf.cbuf[ble_cbuf.read_ptr];
			update_circ_readindex(&ble_cbuf, 1);
			n++;
		}
		str[n] = 0;
		leuart_start(LEUART0, str, length);
		return false;
	}
}

/***************************************************************************//**
 * @brief
 *	Space available on circular buffer.
 *
 * @details
 *	Will return an 8-bit unsigned integer with the amount of space remaining on
 *	the circular buffer.
 *
 * @note
 *	Uses the private circular buffer implemented. Must be initialized prior to calling
 *	this function.
 *
 * @return
 * The amount of space available on the circular buffer.
 *
 ******************************************************************************/
uint8_t ble_circ_space(void){
	return (ble_cbuf.size - ((ble_cbuf.write_ptr - ble_cbuf.read_ptr) & ble_cbuf.size_mask));
}




/***************************************************************************//**
 * @brief
 *	HM18 BLE Module Init Function
 *
 * @details
 * This function will open the BLE module by initializing the LEUART peripheral
 * for proper communication witht the HM18.
 *
 * @note
 * The passed events will be called at the end of the RX/TX state machine.
 *
 * @param[in] tx_event
 * The callback to be added at the end of TX transfer.
 *
 * @param[in] rx_event
 * The callback to be added at the end of RX recieve.
 ******************************************************************************/
void ble_open(uint32_t tx_event, uint32_t rx_event){
	LEUART_OPEN_STRUCT leuart_settings;
	leuart_settings.baudrate = HM10_BAUDRATE;
	leuart_settings.databits = HM10_DATABITS;
	leuart_settings.enable = HM10_ENABLE;
	leuart_settings.parity = HM10_PARITY;
	leuart_settings.stopbits = HM10_STOPBITS;
	leuart_settings.rx_loc = LEUART0_RX_ROUTE;
	leuart_settings.rx_pin_en = true;
	leuart_settings.tx_loc = LEUART0_TX_ROUTE;
	leuart_settings.tx_pin_en = true;
	leuart_settings.rx_en = true;
	leuart_settings.tx_en = true;
	leuart_settings.rx_done_evt = rx_event;
	leuart_settings.tx_done_evt = tx_event;
	leuart_settings.refFreq = HM10_REFFREQ;

	ble_circ_init();

	leuart_open(HM10_LEUART0, &leuart_settings);
}


/***************************************************************************//**
 * @brief
 *  HM18 BLE Write Function
 *
 * @details
 * 	Adds a string to the circuar buffer and then pops it to begin circular buffer
 * 	LEUART operation.
 *
 * @note
 * LEUART0 peripheral will be used
 *
 * @param[in] string
 * The string that will be written to the HM18 BLE Module
 *
 ******************************************************************************/
void ble_write(char* string){
	ble_circ_push(string);
	ble_circ_pop(CIRC_OPER);
}

/***************************************************************************//**
 * @brief
 *   BLE Test performs two functions.  First, it is a Test Driven Development
 *   routine to verify that the LEUART is correctly configured to communicate
 *   with the BLE HM-18 module.  Second, the input argument passed to this
 *   function will be written into the BLE module and become the new name
 *   advertised by the module while it is looking to pair.
 *
 * @details
 * 	 This global function will use polling functions provided by the LEUART
 * 	 driver for both transmit and receive to validate communications with
 * 	 the HM-18 BLE module.  For the assignment, the communication with the
 * 	 BLE module must use low energy design principles of being an interrupt
 * 	 driven state machine.
 *
 * @note
 *   For this test to run to completion, the phone most not be paired with
 *   the BLE module.  In addition for the name to be stored into the module
 *   a breakpoint must be placed at the end of the test routine and stopped
 *   at this breakpoint while in the debugger for a minimum of 5 seconds.
 *
 * @param[in] *mod_name
 *   The name that will be written to the HM-18 BLE module to identify it
 *   while it is advertising over Bluetooth Low Energy.
 *
 * @return
 *   Returns bool true if successfully passed through the tests in this
 *   function.
 ******************************************************************************/
bool ble_test(char *mod_name){
	uint32_t	str_len;

	CORE_DECLARE_IRQ_STATE;
	CORE_ENTER_CRITICAL();

	// This test will limit the test to the proper setup of the LEUART
	// peripheral, routing of the signals to the proper pins, pin
	// configuration, and transmit/reception verification.  The test
	// will communicate with the BLE module using polling routines
	// instead of interrupts.
	// How is polling different than using interrupts?
	// ANSWER: Polling constantly checks the state of the RXDATAV register
	//		   to see if data is available. This uses lots of energy. Interrupts
	//		   only pull the data when it's available, so not constantly checking.
	// How does interrupts benefit the system for low energy operation?
	// ANSWER: As said above, interrupts make it so the system is only on and pulling
	// 		   data from the RXDATA register when there is data available. Otherwise,
	//		   the module is in the lowest non-blocked energy mode.
	// How does interrupts benefit the system that has multiple tasks?
	// ANSWER: Interrupts benefit in a system with multiple tasks because processes
	//		   will only be ran when the tasks are ready to be dealt with. This causes the
	//		   energy consumption and overall processsing overhead to go down.

	// First, you will need to review the DSD HM10 datasheet to determine
	// what the default strings to write data to the BLE module and the
	// expected return statement from the BLE module to test / verify the
	// correct response

	// The test_str is used to tell the BLE module to end a Bluetooth connection
	// such as with your phone.  The ok_str is the result sent from the BLE module
	// to the micro-controller if there was not active BLE connection at the time
	// the break command was sent to the BLE module.
	// Replace the test_str "" with the command to break or end a BLE connection
	// Replace the ok_str "" with the result that will be returned from the BLE
	//   module if there was no BLE connection
	char		test_str[80] = "AT";
	char		ok_str[80] = "OK";


	// output_str will be the string that will program a name to the BLE module.
	// From the DSD HM10 datasheet, what is the command to program a name into
	// the BLE module?
	// ANSWER: "AT+NAME<nameString>"
	// The  output_str will be a string concatenation of the DSD HM10 command
	// and the input argument sent to the ble_test() function
	// Replace the output_str "" with the command to change the program name
	// Replace the result_str "" with the first part of the expected result
	//  the backend of the expected response will be concatenated with the
	//  input argument
	char		output_str[80] = "AT+NAME";
	char		result_str[80] = "OK+Set:";


	// To program the name into your module, you must reset the module after you
	// have sent the command to update the modules name.  What is the DSD HM10
	// name to reset the module?
	// ANSWER: "AT+RESET"
	// Replace the reset_str "" with the command to reset the module
	// Replace the reset_result_str "" with the expected BLE module response to
	//  to the reset command
	char		reset_str[80] = "AT+RESET";
	char		reset_result_str[80] = "OK+RESET";
	char		return_str[80];

	bool		success;
	bool		rx_disabled, rx_en, tx_en;
	uint32_t	status;

	// These are the routines that will build up the entire command and response
	// of programming the name into the BLE module.  Concatenating the command or
	// response with the input argument name
	strcat(output_str, mod_name);
	strcat(result_str, mod_name);

	// The test routine must not alter the function of the configuration of the
	// LEUART driver, but requires certain functionality to insure the logical test
	// of writing and reading to the DSD HM10 module.  The following c-lines of code
	// save the current state of the LEUART driver that will be used later to
	// re-instate the LEUART configuration

	status = leuart_status(HM10_LEUART0);
	if (status & LEUART_STATUS_RXBLOCK) {
		rx_disabled = true;
		// Enabling, unblocking, the receiving of data from the LEUART RX port
		leuart_cmd_write(HM10_LEUART0, LEUART_CMD_RXBLOCKDIS);
	}
	else rx_disabled = false;
	if (status & LEUART_STATUS_RXENS) {
		rx_en = true;
	} else {
		rx_en = false;
		// Enabling the receiving of data from the RX port
		leuart_cmd_write(HM10_LEUART0, LEUART_CMD_RXEN);
		while (!(leuart_status(HM10_LEUART0) & LEUART_STATUS_RXENS));
	}

	if (status & LEUART_STATUS_TXENS){
		tx_en = true;
	} else {
		// Enabling the transmission of data to the TX port
		leuart_cmd_write(HM10_LEUART0, LEUART_CMD_TXEN);
		while (!(leuart_status(HM10_LEUART0) & LEUART_STATUS_TXENS));
		tx_en = false;
	}
//	leuart_cmd_write(HM10_LEUART0, (LEUART_CMD_CLEARRX | LEUART_CMD_CLEARTX));

	// This sequence of instructions is sending the break ble connection
	// to the DSD HM10 module.
	// Why is this command required if you want to change the name of the
	// DSD HM10 module?
	// ANSWER: Changing the name of the module will cause issues with the communciation
	// 		   between the HM18 and the device connected. The other device will be expecting
	//		   the legacy name and will not be able to connect.
	str_len = strlen(test_str);
	for (int i = 0; i < str_len; i++){
		leuart_app_transmit_byte(HM10_LEUART0, test_str[i]);
	}

	// What will the ble module response back to this command if there is
	// a current ble connection?
	// ANSWER: "OK+LOST"
	str_len = strlen(ok_str);
	for (int i = 0; i < str_len; i++){
		return_str[i] = leuart_app_receive_byte(HM10_LEUART0);
		if (ok_str[i] != return_str[i]) {
				EFM_ASSERT(false);;
		}
	}

	// This sequence of code will be writing or programming the name of
	// the module to the DSD HM10
	str_len = strlen(output_str);
	for (int i = 0; i < str_len; i++){
		leuart_app_transmit_byte(HM10_LEUART0, output_str[i]);
	}

	// Here will be the check on the response back from the DSD HM10 on the
	// programming of its name
	str_len = strlen(result_str);
	for (int i = 0; i < str_len; i++){
		return_str[i] = leuart_app_receive_byte(HM10_LEUART0);
		if (result_str[i] != return_str[i]) {
				EFM_ASSERT(false);;
		}
	}

	// It is now time to send the command to RESET the DSD HM10 module
	str_len = strlen(reset_str);
	for (int i = 0; i < str_len; i++){
		leuart_app_transmit_byte(HM10_LEUART0, reset_str[i]);
	}

	// After sending the command to RESET, the DSD HM10 will send a response
	// back to the micro-controller
	str_len = strlen(reset_result_str);
	for (int i = 0; i < str_len; i++){
		return_str[i] = leuart_app_receive_byte(HM10_LEUART0);
		if (reset_result_str[i] != return_str[i]) {
				EFM_ASSERT(false);
		}
	}

	// After the test and programming have been completed, the original
	// state of the LEUART must be restored
	if (!rx_en) leuart_cmd_write(HM10_LEUART0, LEUART_CMD_RXDIS);
	if (rx_disabled) leuart_cmd_write(HM10_LEUART0, LEUART_CMD_RXBLOCKEN);
	if (!tx_en) leuart_cmd_write(HM10_LEUART0, LEUART_CMD_TXDIS);
	leuart_if_reset(HM10_LEUART0);

	success = true;


	CORE_EXIT_CRITICAL();
	return success;
}

/***************************************************************************//**
 * @brief
 *   Circular Buff Test is a Test Driven Development function to validate
 *   that the circular buffer implementation
 *
 * @details
 * 	 This Test Driven Development test has tests integrated into the function
 * 	 to validate that the routines can successfully identify whether there
 * 	 is space available in the circular buffer, the write and index pointers
 * 	 wrap around, and that one or more packets can be pushed and popped from
 * 	 the circular buffer.
 *
 * @note
 *   If anyone of these test will fail, an EFM_ASSERT will occur.  If the
 *   DEBUG_EFM=1 symbol is defined for this project, exiting this function
 *   confirms that the push, pop, and the associated utility functions are
 *   working.
 *
 * @par
 *   There is a test escape that is not possible to test through this
 *   function that will need to be verified by writing several ble_write()s
 *   back to back and verified by checking that these ble_write()s were
 *   successfully transmitted to the phone app.
 *
 ******************************************************************************/
 void circular_buff_test(void){
	 bool buff_empty;
	 int test1_len = 50;
	 int test2_len = 25;
	 int test3_len = 5;

	 // Why this 0 initialize of read and write pointer?
	 // Student Response:
	 //	The first index of the array being used as a circular buffer is index 0;
	 ble_cbuf.read_ptr = 0;
	 ble_cbuf.write_ptr = 0;

	 // Why do none of these test strings contain a 0?
	 // Student Response:
	 //	0, or '\0' in ASCII, indicates the end of a string. This would cause the test to stop.
	 for (int i = 0;i < test1_len; i++){
		 test_struct.test_str[0][i] = i+1;
	 }
	 test_struct.test_str[0][test1_len] = 0;

	 for (int i = 0;i < test2_len; i++){
		 test_struct.test_str[1][i] = i + 20;
	 }
	 test_struct.test_str[1][test2_len] = 0;

	 for (int i = 0;i < test3_len; i++){
		 test_struct.test_str[2][i] = i +  35;
	 }
	 test_struct.test_str[2][test3_len] = 0;

	 // What is this test validating?
	 // Student response:
	 // Validating that the buffer is empty (space available == total space)

	 EFM_ASSERT(ble_circ_space() == CSIZE);

	 // Why is there only one push to the circular buffer at this stage of the test
	 // Student Response:
	 // Pushing a single byte to the buffer to test that the space function works properly.
	 ble_circ_push(&test_struct.test_str[0][0]);

	 // What is this test validating?
	 // Student response:
	 // Verifies that the space function is working properly per the last call to push a single byte.
	 EFM_ASSERT(ble_circ_space() == (CSIZE - test1_len - 1));

	 // Why is the expected buff_empty test = false?
	 // Student Response:
	 // There should be content in the buffer to be popped. The function returns false if popped data.
	 buff_empty = ble_circ_pop(CIRC_TEST);
	 EFM_ASSERT(buff_empty == false);
	 for (int i = 0; i < test1_len; i++){
		 EFM_ASSERT(test_struct.test_str[0][i] == test_struct.result_str[i]);
	 }

	 // What is this test validating?
	 // Student response:
	 // Testing that the popped string length is the same as the test string.
	 EFM_ASSERT(strlen(test_struct.result_str) == test1_len);

	 // What is this test validating?
	 // Student response: Validating that everything was popped and the buffer is empty.
	 EFM_ASSERT(ble_circ_space() == CSIZE);


	 // What does this next push on the circular buffer test?
	 // Student Response:
	 // This push should cause the circular buffer to wrap around.
	 ble_circ_push(&test_struct.test_str[1][0]);


	 EFM_ASSERT(ble_circ_space() == (CSIZE - test2_len - 1));

	 // What does this next push on the circular buffer test?
	 // Student Response: Tests whether multiple strings can be pushed properly onto the buffer.
	 ble_circ_push(&test_struct.test_str[2][0]);


	 EFM_ASSERT(ble_circ_space() == (CSIZE - test2_len - 1 - test3_len - 1));

	 // What does this next push on the circular buffer test?
	 // Student Response:
	 // This isn't a push... but we're testing that the buffer is correctly wrapping.
	 EFM_ASSERT(abs(ble_cbuf.write_ptr - ble_cbuf.read_ptr) < CSIZE);

	 // Why is the expected buff_empty test = false?
	 // Student Response:
	 // There should be content in the buffer to be popped. The function returns false if data is popped off.
	 buff_empty = ble_circ_pop(CIRC_TEST);
	 EFM_ASSERT(buff_empty == false);
	 for (int i = 0; i < test2_len; i++){
		 EFM_ASSERT(test_struct.test_str[1][i] == test_struct.result_str[i]);
	 }

	 // What is this test validating?
	 // Student response: making sure the length of the popped string is the same as the test string.
	 EFM_ASSERT(strlen(test_struct.result_str) == test2_len);

	 EFM_ASSERT(ble_circ_space() == (CSIZE - test3_len - 1));

	 // Why is the expected buff_empty test = false?
	 // Student Response:
	 // We expect that there is still data on the buffer, so it should return false,
	 // indicating that data was popped of.
	 buff_empty = ble_circ_pop(CIRC_TEST);
	 EFM_ASSERT(buff_empty == false);
	 for (int i = 0; i < test3_len; i++){
		 EFM_ASSERT(test_struct.test_str[2][i] == test_struct.result_str[i]);
	 }

	 // What is this test validating?
	 // Student response: Making sure the length of the next popped string is the same as the third test.
	 // Then making sure that the cirular buffer is empty.
	 EFM_ASSERT(strlen(test_struct.result_str) == test3_len);

	 EFM_ASSERT(ble_circ_space() == CSIZE);

	 // Using these three writes and pops to the circular buffer, what other test
	 // could we develop to better test out the circular buffer?
	 // Student Response: We can ensure that the buffer doesn't throw any errors
	 // when overfilling. It should just not add that data.


	 // Why is the expected buff_empty test = true?
	 // Student Response:
	 // We expect that the buffer is empty so nothing should have been popped off.
	 buff_empty = ble_circ_pop(CIRC_TEST);
	 EFM_ASSERT(buff_empty == true);
	 ble_write("\nPassed Circular Buffer Test\n");

 }




