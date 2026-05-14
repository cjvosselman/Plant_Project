// ***************************    C Source Code     ****************************
// *****************************************************************************
//   DESIGNER NAME:  Casey J. Vosselman
//
//         VERSION:  1.0
//
//       FILE NAME:  light_sensor.c
//
//-----------------------------------------------------------------------------
// DESCRIPTION
//
//    This file contains a collection of functions to interact with the a 
//    resistive photocell. The module is configured for to  taking in data 
//    from ADC1, channel 6. 
//
//    Features of functions:
//
//    - Displaying values of reads based on threshold values; give status based
//      reads and utilizes an LED to get the lighting back to equilibrium
//
//-----------------------------------------------------------------------------

#ifndef __LIGHT_SENSOR_H__
#define __LIGHT_SENSOR_H__

//-----------------------------------------------------------------------------
// Loads standard C include files
//-----------------------------------------------------------------------------
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// --------------------------------------------------------------------------
// Prototype for Launchpad support functions
// --------------------------------------------------------------------------

#define light_step_size 450
#define light_threshold 1800
#define dark_threshold 1200
#define optimal_value 1600

#define MAX_DUTY_CYCLE 100
#define MIN_DUTY_CYCLE 0


void light_display(uint16_t adc_reading);
uint16_t light_read();
void light_correction(uint16_t adc_reading);

#endif