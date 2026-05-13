//*****************************************************************************
//*****************************    C Source Code    ***************************
//*****************************************************************************
//  DESIGNER NAME:  TBD
//
//       LAB NAME:  TBD
//
//      FILE NAME:  main.c
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//    This project runs on the LP_MSPM0G3507 LaunchPad board interfacing to
//    the CSC202 Expansion board.
//
//    This code ... *** COMPLETE THIS BASED ON LAB REQUIREMENTS ***
//
//*****************************************************************************
//*****************************************************************************

//-----------------------------------------------------------------------------
// Loads standard C include files
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

//-----------------------------------------------------------------------------
// Define function prototypes used by the program
//-----------------------------------------------------------------------------
void TIMA0_C0_init(void);
void TIMA0_C0_pwm_init(uint32_t load_value, uint32_t compare_value);
void TIMA0_C0_pwm_enable(void);
void TIMA0_C0_set_pwm_dc(uint8_t duty_cycle);
uint32_t ADC1_in(uint8_t channel);
void ADC1_init(uint32_t reference);
void light_display(uint16_t adc_reading);
uint16_t light_read();
void light_correction(uint16_t adc_reading);
//-----------------------------------------------------------------------------
// Define symbolic constants used by the program
//-----------------------------------------------------------------------------
#define step_size 450
#define light_threshold 1800
#define dark_threshold 1200
#define optimal_value 1600

#define MAX_DUTY_CYCLE 100
#define MIN_DUTY_CYCLE 0
//-----------------------------------------------------------------------------
// Define global variables and structures here.
// NOTE: when possible avoid using global variables
//-----------------------------------------------------------------------------

// Define a structure to hold different data types

int main_light(void) {
  // Configure the LaunchPad board
  clock_init_40mhz();
  launchpad_gpio_init();
  I2C_mstr_init();
  lcd1602_init();
  leds_init();
  leds_enable();
  ADC1_init(ADC12_MEMCTL_VRSEL_INTREF_VSSA);

  TIMA0_C0_init();
  TIMA0_C0_pwm_init(4000, 0);
  TIMA0_C0_pwm_enable();
  lcd_clear();
  while(1){
  light_read();
  light_display(light_read());
  light_correction(light_read());
  }

  // Endless loop to prevent program from ending
  while (1)
    ;

} /* main */
  
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
  } else {
    TIMA0_C0_set_pwm_dc(duty_cycle);
    TIMA0_C0_pwm_enable();
    lcd_set_ddram_addr(LCD_LINE2_ADDR + LCD_CHAR_POSITION_9);
    lcd_write_string("Perfect");
  }
}

