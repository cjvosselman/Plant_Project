// *****************************************************************************
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
// Load standard C include files
//-----------------------------------------------------------------------------
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "LaunchPad.h"
#include "adc.h"
#include "clock.h"
#include "lcd1602.h"
#include "light_sensor.h"
#include "spi.h"
#include "timer.h"
#include "uart.h"
#include <ti/devices/msp/msp.h>

//-----------------------------------------------------------------------------
// Loads MSP launchpad board support macros and definitions
//-----------------------------------------------------------------------------

volatile uint8_t duty_cycle = 0;


//-----------------------------------------------------------------------------
// DESCRIPTION:
//  Simple function call to recieve the data from ADC1 Channel 6.
//
// INPUT PARAMETERS:
//   none
//
// OUTPUT PARAMETERS:
//   none
//
// RETURN:
//  uint16_t soil_read (ADC1 Channel 6 value)
// -----------------------------------------------------------------------------

uint16_t light_read() {
  uint16_t light_read = 0;

  light_read = ADC1_in(6);
  return light_read;
}

//------------------------------------------------------------------------------
// DESCRIPTION:
//  Works in tandem with the light_read() function. The idea is to use light_read()
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

void light_display(uint16_t adc_reading) {

  uint16_t light_read = adc_reading;
  char adc_text[] = "ADC = ";
  char light_text[] = "Light = ";

  lcd_set_ddram_addr(LCD_LINE1_ADDR);
  lcd_write_string(adc_text);
  lcd_set_ddram_addr(LCD_LINE1_ADDR + LCD_CHAR_POSITION_8);
  lcd_write_doublebyte(light_read);
  lcd_set_ddram_addr(LCD_LINE2_ADDR);
  lcd_write_string(light_text);
  lcd_set_ddram_addr(LCD_LINE2_ADDR + LCD_CHAR_POSITION_9);
  TIMA0_C0_set_pwm_dc(duty_cycle);
  TIMA0_C0_pwm_enable();
  lcd_set_ddram_addr(LCD_LINE2_ADDR + LCD_CHAR_POSITION_9);
  lcd_write_string("Perfect");
}
//-----------------------------------------------------------------------------
// DESCRIPTION:
//  Works in tandem with the light_read() function. The idea is to use light_read()
//  as an input. This function will alter the duty cycle value of an LED
//  to get the photocell back to an adequate lighting threshold.
// 
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

void light_correction(uint16_t adc_reading) {
  uint16_t light_read = adc_reading;
  char adc_text[] = "ADC =  ";
  char light_text[] = "Light = ";
  uint8_t increment = 1;

  if (light_read > light_threshold) {
    if (duty_cycle > MIN_DUTY_CYCLE) {
      duty_cycle -= increment;

      TIMA0_C0_set_pwm_dc(duty_cycle);
      TIMA0_C0_pwm_enable();
      msec_delay(200);
      light_read = ADC1_in(6);
      lcd_set_ddram_addr(LCD_LINE1_ADDR);
      lcd_write_string(adc_text);
      lcd_set_ddram_addr(LCD_LINE1_ADDR + LCD_CHAR_POSITION_8);
      lcd_write_doublebyte(light_read);
      lcd_set_ddram_addr(LCD_LINE2_ADDR);
      lcd_write_string(light_text);
      lcd_set_ddram_addr(LCD_LINE2_ADDR + LCD_CHAR_POSITION_9);
      lcd_write_string("Bright ");
    }
    else {
    duty_cycle = MIN_DUTY_CYCLE;
    }
  }

  else if (light_read < dark_threshold) {
    if (duty_cycle <= MAX_DUTY_CYCLE) {
      duty_cycle += increment;
      TIMA0_C0_set_pwm_dc(duty_cycle);
      TIMA0_C0_pwm_enable();
      msec_delay(200);
      light_read = ADC1_in(6);
      lcd_set_ddram_addr(LCD_LINE1_ADDR);
      lcd_write_string(adc_text);
      lcd_set_ddram_addr(LCD_LINE1_ADDR + LCD_CHAR_POSITION_8);
      lcd_write_doublebyte(light_read);
      lcd_set_ddram_addr(LCD_LINE2_ADDR);
      lcd_write_string(light_text);
      lcd_set_ddram_addr(LCD_LINE2_ADDR + LCD_CHAR_POSITION_9);
      lcd_write_string("Dark   ");
    } 
    else {
    duty_cycle = MAX_DUTY_CYCLE;
    }
  }
}
