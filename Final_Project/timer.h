// *****************************************************************************
// ***************************    C Source Code     ****************************
// *****************************************************************************
//   DESIGNER NAME:  Casey Vosselman
//
//         VERSION:  1.0
//
//       FILE NAME:  timer.h
//
//-----------------------------------------------------------------------------
// DESCRIPTION

//    This file contains a collection of functions and variables for initializing
//    and configuring timers used in the CSC 202 Plant Monitoring Device. 
//
//-----------------------------------------------------------------------------

#ifndef __TIMER_H__
#define __TIMER_H__

//-----------------------------------------------------------------------------
// Load standard C include files
//-----------------------------------------------------------------------------
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// Loads MSP launchpad board support macros and definitions
//-----------------------------------------------------------------------------


#define LOAD_VALUE (50000)
#define BUS_CLOCK (40E6)
#define TIMER_DIVIDE_BY (GPTIMER_CLKDIV_RATIO_DIV_BY_8)
#define TIMER_CPS_PCNT (0x64)
#define BUS_CLK_DIVDER ((TIMER_DIVIDE_BY + 1) * (TIMER_CPS_PCNT + 1))
#define TMR_CLK_PER_SECOND (BUS_CLOCK / BUS_CLK_DIVDER)
#define TMR_CLK_PER_MSEC (TMR_CLK_PER_SECOND / 1000)
#define SECONDS_PER_COUNT (1)
#define MAX_ISR_COUNT_DELAY (SECONDS_PER_COUNT * 1E3)

// -----------------------------------------------------------------------------
// Prototype for Launchpad support functions
// -----------------------------------------------------------------------------


void TIMG8_config(uint32_t load_value);
void TIMG8_enable(void);
void TIMG8_disable(void);
void TIMG8_enable_interrupt(void);
void TIMG8_IRQHandler(void);


void TIMA0_C0_init(void);
void TIMA0_C0_pwm_init(uint32_t load_value, uint32_t compare_value);
void TIMA0_C0_pwm_enable(void);
void TIMA0_C0_set_pwm_dc(uint8_t duty_cycle);

#endif