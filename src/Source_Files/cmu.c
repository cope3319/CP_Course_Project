/**
 * @file cmu.c
 * @author Connor Peskin
 * @date September 10, 2020
 * @brief Contains one function to setup CMU clocks.
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "cmu.h"

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// Private variables
//***********************************************************************************


//***********************************************************************************
// Private functions
//***********************************************************************************


//***********************************************************************************
// Global functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 * Driver to initialize the CMU clocks for this application.
 *
 * @details
 * This function enables oscillators, routes clock trees, and starts timers for various
 * app.c processes.
 *
 *
 * @note
 * This function should be called prior to other init/open functions, as they may have
 * oscillator/clock dependencies.
 *
 ******************************************************************************/
void cmu_open(void){

		CMU_ClockEnable(cmuClock_HFPER, true);

		// By default, Low Frequency Resistor Capacitor Oscillator, LFRCO, is enabled,
		// Disable the LFRCO oscillator
		CMU_OscillatorEnable(cmuOsc_LFRCO , false, false);	 // What is the enumeration required for LFRCO?

		// Disable the Low Frequency Crystal Oscillator, LFXO
//		CMU_OscillatorEnable(cmuOsc_LFXO , false, false);	// What is the enumeration required for LFXO?

		// No requirement to enable the ULFRCO oscillator.  It is always enabled in EM0-4H

		// Route LF clock to LETIMER0 clock tree
		CMU_ClockSelectSet(cmuClock_LFA , cmuSelect_ULFRCO);	// What clock tree does the LETIMER0 reside on?

		// Now, you must ensure that the global Low Frequency is enabled
		CMU_ClockEnable(cmuClock_CORELE, true);	//This enumeration is found in the Lab 2 assignment

		/* UART Lab 5 */
		CMU_OscillatorEnable(cmuOsc_LFXO, true, true); //enable the LFXO oscillator
		CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO); // route LFXO oscillator to LFB clock (used for LEUART0)
}

