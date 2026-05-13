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
#include "light_sensor.h"
#include "timer.h"

  uint16_t light_read() {
  bool done = false;
  uint16_t light_read = 0;
  char adc_text[] = "ADC = ";
  char light_text[] = "Light = ";
  static uint8_t increment = 1;
  uint8_t duty_cycle = 0;

  light_read = ADC1_in(6);
  return light_read;
  }
  
  void light_display(uint16_t adc_reading)
  {

  uint16_t light_read = adc_reading;
  char adc_text[] = "ADC = ";
  char light_text[] = "Light = ";
  static uint8_t increment = 1;
  uint8_t duty_cycle = 0;

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

 void light_correction(uint16_t adc_reading)
  {
  uint16_t light_read = adc_reading;
  char adc_text[] = "ADC = ";
  char light_text[] = "Light = ";
  static uint8_t increment = 1;
  uint8_t duty_cycle = 0;
  
  if (light_read > light_threshold) {
    if (duty_cycle >= MIN_DUTY_CYCLE) {
      duty_cycle -= increment;
      TIMA0_C0_set_pwm_dc(duty_cycle);
      TIMA0_C0_pwm_enable();
      msec_delay(200);
      light_read = ADC1_in(6);
      lcd_set_ddram_addr(LCD_LINE1_ADDR + LCD_CHAR_POSITION_8);
      lcd_write_doublebyte(light_read);
      lcd_set_ddram_addr(LCD_LINE2_ADDR + LCD_CHAR_POSITION_9);
      lcd_write_string("Bright ");
    }
  }

  else if (light_read < dark_threshold) {
    if (duty_cycle <= MAX_DUTY_CYCLE) {
      duty_cycle += increment;
      TIMA0_C0_set_pwm_dc(duty_cycle);
      TIMA0_C0_pwm_enable();
      msec_delay(200);
      light_read = ADC1_in(6);
      lcd_set_ddram_addr(LCD_LINE1_ADDR + LCD_CHAR_POSITION_8);
      lcd_write_doublebyte(light_read);
      lcd_set_ddram_addr(LCD_LINE2_ADDR + LCD_CHAR_POSITION_9);
      lcd_write_string("Dark   ");
    }
}
}

