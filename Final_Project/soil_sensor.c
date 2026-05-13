//-----------------------------------------------------------------------------
// Load standard C include files
//-----------------------------------------------------------------------------
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
//-----------------------------------------------------------------------------
// Loads MSP launchpad board support macros and definitions
//-----------------------------------------------------------------------------
#include "LaunchPad.h"
#include "adc.h"
#include "clock.h"
#include "lcd1602.h"
#include "spi.h"
#include "uart.h"
#include <ti/devices/msp/msp.h>
#include "soil_sensor.h"

uint16_t soil_read()
 {
  uint16_t soil_read = 0;

  soil_read = ADC0_in(5);
  return soil_read;
}

void display_soil(uint16_t adc_reading)
{
  uint8_t led_idx = 0;
  uint16_t moisture_level = adc_reading; 
  uint16_t moisture_value = (moisture_level/soil_step_size);

   lcd_set_ddram_addr(LCD_LINE2_ADDR);

    if (moisture_level < dry_soil_value) 
    {
      lcd_write_string("Status = Wet");
    } 
    else if (moisture_level > dry_soil_value) 
    {
      lcd_write_string("Status = Dry");
    }

    lcd_set_ddram_addr(LCD_LINE1_ADDR);
    lcd_write_string("ADC = ");
    lcd_set_ddram_addr(LCD_LINE1_ADDR + LCD_CHAR_POSITION_8);
    lcd_write_doublebyte(moisture_level);
}

void soil_intensity(uint16_t adc_reading)
{
  uint8_t led_idx = 0;
  uint8_t moisture_level = adc_reading; 
  uint16_t moisture_value = (moisture_level/soil_step_size);


  for (led_idx = 0; led_idx < moisture_value; led_idx++) {
    led_on(led_idx);
  }
}

void average_adc_values() {
  ADC0->ULLMEM.MEMCTL[0] = ADC12_MEMCTL_AVGEN_ENABLE;
  ADC0->ULLMEM.CTL1 = ADC12_CTL1_AVGN_AVG_16 | ADC12_CTL1_AVGD_SHIFT4;
}