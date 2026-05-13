



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