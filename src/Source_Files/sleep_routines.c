/**
* @file sleep_routines.c
* @brief Sets up sleep routines to keep device in lowest possible energy mode
***************************************************************************
* @section License
* <b>(C) Copyright 2015 Silicon Labs, http://www.silabs.com</b>
***************************************************************************
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*
* DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
* obligation to support this Software. Silicon Labs is providing the
* Software "AS IS", with no express or implied warranties of any kind,
* including, but not limited to, any implied warranties of merchantability
* or fitness for any particular purpose or warranties against infringement
* of any proprietary rights of a third party.
*
* Silicon Labs will not be liable for any consequential, incidental, or
* special damages, or any other relief, or for any claim by any third party,
* arising from your use of this Software.
*
*/


//***********************************************************************************
// Include files
//***********************************************************************************

//** Standard Libraries

//** Silicon Lab include files

//** User/developer include files
#include "sleep_routines.h"

//***********************************************************************************
// Private variables
//***********************************************************************************
static int lowest_energy_mode[MAX_ENERGY_MODES];

//***********************************************************************************
// Private functions
//***********************************************************************************

//***********************************************************************************
// Global functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 * Initializes the sleep handler
 *
 * @details
 * Sets all entries of the static lowest_energy_mode[] array to 0.
 *
 * @note
 * Upon initialization,
 *
 ******************************************************************************/
void sleep_open(void){
	for(int i = 0; i < MAX_ENERGY_MODES; i++) lowest_energy_mode[i] = 0;
}

/***************************************************************************//**
 * @brief
 * Blocks the sleep mode passed.
 *
 * @details
 * This will modify the static lowest_energy_mode[] array to block the passed
 * sleep mode. A sleep mode should not be blocked more than 5 times- if so, the
 * sleep mode is likely not being unblocked.
 *
 * @note
 * This is an atomic operation. A mode should not be blocked 5 times without
 * being unblocked. This will cause this function to fail an assert statement.
 *
 * @param[in]
 * The desired sleep mode to block.
 *
 ******************************************************************************/
void sleep_block_mode(uint32_t EM){
	CORE_DECLARE_IRQ_STATE;
	CORE_ENTER_CRITICAL();

	lowest_energy_mode[EM] ++;
	EFM_ASSERT(lowest_energy_mode[EM] < 5);

	CORE_EXIT_CRITICAL();
}

/***************************************************************************//**
 * @brief
 * Blocks the sleep mode passed.
 *
 * @details
 * This will modify the static lowest_energy_mode[] array to unblock the passed
 * sleep mode. A sleep mode should unblocked if it has not been blocked.
 *
 * @note
 * This is an atomic operation. If a sleep mode is unblocked and hasn't been blocked
 * this function may fail an assert statement.
 *
 * @param[in]
 * The desired sleep mode to unblock.
 *
 *
 ******************************************************************************/
void sleep_unblock_mode(uint32_t EM){
	if(lowest_energy_mode[EM] > 0){
		CORE_DECLARE_IRQ_STATE;
		CORE_ENTER_CRITICAL();

		lowest_energy_mode[EM] --;
		EFM_ASSERT(lowest_energy_mode[EM]>=0);

		CORE_EXIT_CRITICAL();
	}
}

/***************************************************************************//**
 * @brief
 * Enters the lowest-energy available sleep mode
 *
 * @details
 * This function uses the static lowest_energy_modes[] array to find the deepest
 * sleep energy mode that is not blocked. It then enters that energy mode.
 *
 * @note
 * This function runs atomically. This will never enter the EM4 energy state
 * with the current implementation.
 *
 ******************************************************************************/
void enter_sleep(void){
	CORE_DECLARE_IRQ_STATE;
	CORE_ENTER_CRITICAL();

	if(lowest_energy_mode[EM0] > 0) {
		CORE_EXIT_CRITICAL();
		return;
	}
	if(lowest_energy_mode[EM1] > 0){
		CORE_EXIT_CRITICAL();
		return;
	}
	if(lowest_energy_mode[EM2] > 0){
		EMU_EnterEM1();
		CORE_EXIT_CRITICAL();
		return;
	}
	if(lowest_energy_mode[EM3] > 0){
		EMU_EnterEM2(true);
		CORE_EXIT_CRITICAL();
		return;
	}
	EMU_EnterEM3(true);
	CORE_EXIT_CRITICAL();
}

/***************************************************************************//**
 * @brief
 * Returns the lowest energy mode available.
 *
 * @details
 * This function refers to the static lowest_energy_mode[] array to find the lowest
 * available non-blocked energy mode.
 *
 * @note
 * This function will never return that the EM4 energy mode is available.
 *
 * @return
 * Returns the lowest available energy mode (0 = EM0) as an unsigned integer.
 *
 ******************************************************************************/
uint32_t current_block_energy_mode(void){
	int i = 0;
	while(i < MAX_ENERGY_MODES){
		if(lowest_energy_mode[i] != 0){
			return i;
		}
		i += 1;
	}
	return MAX_ENERGY_MODES-1;
}
