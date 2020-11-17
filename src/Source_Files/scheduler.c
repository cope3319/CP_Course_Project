/**
 * @file scheduler.c
 * @author Connor Peskin
 * @date September 17, 2020
 * @brief Outlines a scheduler that stores callback variables to a static
 * event_scheduled variable to schedule the event.
 */


//***********************************************************************************
// Include files
//***********************************************************************************

//** Standard Libraries

//** Silicon Lab include files
#include "em_assert.h"
#include "em_emu.h"
//** User/developer include files
#include "scheduler.h"

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// Private variables
//***********************************************************************************
static uint32_t event_scheduled;

//***********************************************************************************
// Private functions
//***********************************************************************************


//***********************************************************************************
// Global functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 * Initializes the Scheduler
 *
 * @details
 * Initializes the scheduler by setting the static event_scheduled variable to 0x0
 *
 * @note
 * This is an atomic operation.
 *
 ******************************************************************************/
void scheduler_open(void){
	__disable_irq();
	event_scheduled = 0;
	__enable_irq();
}

/***************************************************************************//**
 * @brief
 * Adds event to Scheduler
 *
 * @details
 * Adds an event to the scheduler by OR'ing the events callback with the
 * static event_scheduled variable.
 *
 * @note
 * This is an atomic operation to avoid missing scheduled events.
 *
 * @param[in]
 * The callback variable for the desired event to add to the scheduler.
 *
 ******************************************************************************/
void add_scheduled_event(uint32_t event){
	__disable_irq();
	event_scheduled |= event;
	__enable_irq();
}

/***************************************************************************//**
 * @brief
 * Removes event from Scheduler
 *
 * @details
 * Removes an event to the scheduler by removing the events callback from the
 * static event_scheduled variable.
 *
 * @note
 * This is an atomic operation to avoid missing scheduled events.
 *
 * @param[in]
 * The callback variable for the desired event to remove from the scheduler.
 *
 ******************************************************************************/
void remove_scheduled_event(uint32_t event){
	__disable_irq();
	event_scheduled &= ~event;
	__enable_irq();
}

/***************************************************************************//**
 * @brief
 * Returns the Currently Scheduled Events
 *
 * @details
 * This function simply returns the static event_scheduled variable.
 *
 * @note
 *
 *
 * @return
 * Returns the currently scheduled events (by callback)- the static
 * event_scheduled variable.
 *
 ******************************************************************************/
uint32_t get_scheduled_events(void){
	return event_scheduled;
}
