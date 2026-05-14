// *****************************************************************************
// ***************************    C Source Code     ****************************
// *****************************************************************************
//   DESIGNER NAME:  Casey J. Vosselman
//
//         VERSION:  1.0
//
//       FILE NAME:  soil_sensor.c
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
//
//-----------------------------------------------------------------------------
// Load standard C include files
//-----------------------------------------------------------------------------
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "LaunchPad.h"
#include "adc.h"
#include "clock.h"
#include "lcd1602.h"
#include "spi.h"
#include "uart.h"
#include <ti/devices/msp/msp.h>
#include "soil_sensor.h"

//-----------------------------------------------------------------------------
// DESCRIPTION:
//  Simple function call to recieve the data from ADC0 Channel 5.
//
// INPUT PARAMETERS:
//   none
//
// OUTPUT PARAMETERS:
//   none
//
// RETURN:
//  uint16_t soil_read (ADC0 Channel 5 value)
// -----------------------------------------------------------------------------


uint16_t soil_read()
 {
  uint16_t soil_read = 0;

  soil_read = ADC0_in(5);
  return soil_read;
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//  Works in tandem with the soil_read() function. The idea is to use soil_read()
//  to get the value from the ADC and uses this function to display the values
//  on the LCD1602.
//
// INPUT PARAMETERS:
//   none
//
// OUTPUT PARAMETERS:
//   none
//
// RETURN:
//   none
// -----------------------------------------------------------------------------


void display_soil(uint16_t adc_reading)
{
  uint8_t led_idx = 0;
  uint16_t moisture_level = adc_reading; 
  uint16_t moisture_value = (moisture_level/soil_step_size);

   lcd_set_ddram_addr(LCD_LINE2_ADDR);

    if (moisture_level < dry_soil_value) 
    {
      lcd_write_string("Soil = Wet");
    } 
    else if (moisture_level > dry_soil_value) 
    {
      lcd_write_string("Soil = Dry");
    }

    lcd_set_ddram_addr(LCD_LINE1_ADDR);
    lcd_write_string("ADC = ");
    lcd_set_ddram_addr(LCD_LINE1_ADDR + LCD_CHAR_POSITION_8);
    lcd_write_doublebyte(moisture_level);
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//  Works in tandem with the soil_read() function. The idea is to use soil_read()
//  to get the value from the ADC and uses this function display the intensity
//  of moisture on the led bar (limited at 6 due to pin conflicts). The goal
//  would be to have 3-4 leds lit up.
//
//  This wasn't implemented in the final project due to time constraints and 
//  conflict with the SEG7 display. Professor Bruce recommended SPI as a solution.
//
// INPUT PARAMETERS:
//   none
//
// OUTPUT PARAMETERS:
//   none
//
// RETURN:
//   none
// -----------------------------------------------------------------------------

void soil_intensity(uint16_t adc_reading)
{
  uint8_t led_idx = 0;
  uint8_t moisture_level = adc_reading; 
  uint16_t moisture_value = (moisture_level/soil_step_size);


  for (led_idx = 0; led_idx < moisture_value; led_idx++) {
    led_on(led_idx);
  }
}
//-----------------------------------------------------------------------------
// DESCRIPTION:
//  Enables ADC averaging. Takes 16 samples and then averages them.
//
// INPUT PARAMETERS:
//   none
//
// OUTPUT PARAMETERS:
//   none
//
// RETURN:
//   none
// -----------------------------------------------------------------------------
void average_adc_values() {
  ADC0->ULLMEM.MEMCTL[0] = ADC12_MEMCTL_AVGEN_ENABLE;
  ADC0->ULLMEM.CTL1 = ADC12_CTL1_AVGN_AVG_16 | ADC12_CTL1_AVGD_SHIFT4;
}