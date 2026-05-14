// *****************************************************************************
// ***************************    C Source Code     ****************************
// *****************************************************************************
//   DESIGNER NAME:  Casey J. Vosselman
//
//         VERSION:  1.0
//
//       FILE NAME:  soil_sensor.h
//
//-----------------------------------------------------------------------------
// DESCRIPTION
//
//    This file contains a collection of functions to interact with the 
//    Capacitive Soil Moisture Sensor Module from Sunfounder. 
//    The module is configured for to  taking in data from ADC0, channel 5. 
//    To provide less noise, the amount of samples included in an average 
//    needed to be changed. I did not want to overwrite the overarching init
//    function thus had to create a function that changed the values at the 
//    register level.
//
//    Features of other functions:
//
//    - Displaying values of reads based on threshold values; give status based
//      reads
//
//    - Utilized an LED BAR to represent soil moisture
//-----------------------------------------------------------------------------

#ifndef __SOIL_SENSOR_H__
#define __SOIL_SENSOR_H__

//-----------------------------------------------------------------------------
// Load standard C include files
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "LaunchPad.h"
#include "adc.h"
#include "clock.h"
#include "lcd1602.h"
#include <ti/devices/msp/msp.h>

//-----------------------------------------------------------------------------
// Loads MSP launchpad board support macros and definitions
//-----------------------------------------------------------------------------
#define dry_soil_value 3000                       // Dry is above | Wet is below
#define soil_step_size 680

// ----------------------------------------------------------------------------
// Prototype for support functions
// ----------------------------------------------------------------------------

uint16_t read_display_soil();
void average_adc_values();
uint16_t soil_read();
void display_soil(uint16_t adc_reading);
void light_correction(uint16_t adc_reading);

#endif 
