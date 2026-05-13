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
#include "timer.h"
#include "uart.h"
#include <ti/devices/msp/msp.h>
#include "soil_sensor.h"
#include "light_sensor.h"

//-----------------------------------------------------------------------------
// Define function prototypes used by the program
//-----------------------------------------------------------------------------

void run_monitoring_system();
void water_plant();
void watering_system(bool moisture_level);
void config_pb1_interrupt();
void config_pb2_interrupt();

void scroll_message(const char string[], uint8_t line);
void lcd_write_string_window(const char string[], uint8_t start_lcd_addr,
                             uint8_t max_lcd_addr);

bool days_since_watered();


void interrupt_buzz();

//-----------------------------------------------------------------------------
// Define symbolic constants used by the program
//-----------------------------------------------------------------------------
#define DEFAULT_NAME ("Default")
#define DEFAULT_LIGHT (0)
#define DEBOUNCE_DELAY (10)
#define LCD_DEGREE_SYMBOL (0xDF)

#define MSPM0_TIMER_CLOCK_FREQ (200E3)
#define PWM_PERIOD (20E-3) // 20 ms period (50 Hz)
#define PWM_COUNT_50HZ (MSPM0_TIMER_CLOCK_FREQ * PWM_PERIOD)
#define SERVO_MIN_PULSE (0.5E-3) // 0.5 ms pulse = -90°
#define SERVO_MAX_PULSE (2.5E-3) // 2.5 ms pulse = +90°
#define SERVO_MIN_COUNT ((SERVO_MIN_PULSE / PWM_PERIOD) * PWM_COUNT_50HZ)
#define SERVO_MAX_COUNT ((SERVO_MAX_PULSE / PWM_PERIOD) * PWM_COUNT_50HZ)

#define SERVO_PWM_UPDATE_COUNT (5)
#define SERVO_PWM_UPDATE_DELAY (SERVO_PWM_UPDATE_COUNT * PWM_PERIOD)

#define CYCLE_DELAY (150)

#define LOAD_VALUE (50000)
#define COMPARE_VALUE (50000)

#define step_size 450
#define light_threshold 1800
#define dark_threshold 1200
#define optimal_value 1600

#define adc12_max 4096
#define soil_threshold 2200 // Dry is above | Wet is below
#define adc_to_tens(adc_val) ((adc_val) * (10 / adc12_max))

#define tick_to_sec(tick_counter) (tick_counter/10)

#define buzzer_enable (LP_RGB_BLU_LED_IDX)
#define dry_soil_value 3000

#define MAX_DUTY_CYCLE 100
#define MIN_DUTY_CYCLE 0
//-----------------------------------------------------------------------------
// Define global variables and structures here.
// NOTE: when possible avoid using global variables
//-----------------------------------------------------------------------------
bool g_pb2_pressed = false;
bool g_pb1_pressed = false;

/*extern volatile uint8_t ones_seconds;
extern volatile uint8_t tens_seconds;*/

volatile uint8_t tick_counter;

// Define a structure to hold different data types
typedef enum {STANDBY,
              LIGHT_DISPLAY,
              MOISTURE_DISPLAY,
              MOISTURE_ALERT,
              LIGHT_CORRECTION,                   
             } PLNT_STS_t;

int main(void) {
  // Configure the LaunchPad board
  clock_init_40mhz();
  launchpad_gpio_init();
  dipsw_init();
  I2C_mstr_init();
  lcd1602_init();
  UART_init(115200);
  seg7_init();
  timerA_config(LOAD_VALUE, COMPARE_VALUE);
  timerA_enable_interrupt();
  timerA_enable();
  ADC0_init(ADC12_MEMCTL_VRSEL_VDDA_VSSA);
  ADC1_init(ADC12_MEMCTL_VRSEL_INTREF_VSSA);
  TIMA0_C0_init();
  TIMA0_C0_pwm_init(4000, 0);
  TIMA0_C0_pwm_enable();
  lp_leds_init();

  // Enable interrupts
  
  config_pb1_interrupt();
  config_pb2_interrupt();

  // enter your code here
  run_monitoring_system();
  // run_monitoring_system();

  // Endless loop to prevent program from ending
  while (1)
    ;

} /* main */

// Notes from Casey: 
// Systick could be used to keep track of the timer
// Timer can be used to call functions when timer reaches zero. 

void run_monitoring_system() {

  bool done = false;
  PLNT_STS_t state = 0;

  while (!done) {

    lcd_set_ddram_addr(LCD_LINE1_ADDR);
    lcd_write_string("Plant Monitoring");
    lcd_set_ddram_addr(LCD_LINE2_ADDR + LCD_CHAR_POSITION_6);
    lcd_write_string("Active");
    uint16_t soil_value = soil_read();
    uint16_t light_value = light_read();

    if (g_pb1_pressed) {
      if (state == STANDBY) {
        state = MOISTURE_DISPLAY;
      } else {
        state = 0;
      }
      lcd_clear();
      g_pb1_pressed = false;
    }

    if (g_pb2_pressed) {
      if (state == STANDBY) {
        state = LIGHT_DISPLAY;
      } else {
        state = 0;
      }
      lcd_clear();
      g_pb2_pressed = false;
    }

    if (soil_read > (uint16_t) dry_soil_value)
    {
      state = MOISTURE_ALERT;
    }
    else {
    state = STANDBY; 
    }

    if ((light_value > light_threshold) || (light_value < dark_threshold))
    {
      state = LIGHT_CORRECTION;
    }
    else{
    state = STANDBY;
    }
    switch (state) {
    case (STANDBY):

    lcd_set_ddram_addr(LCD_LINE1_ADDR);
    lcd_write_string("Plant Monitoring");
    lcd_set_ddram_addr(LCD_LINE2_ADDR + LCD_CHAR_POSITION_6);
    lcd_write_string("Active");

    break;

    case (LIGHT_DISPLAY):

      light_display(light_value);
      
      break;

    case (MOISTURE_DISPLAY):

      display_soil(soil_value);

      break;

    case (MOISTURE_ALERT):
    interrupt_buzz();
    watering_system(false);
    water_plant();
    state = 0;

    break;

    case (LIGHT_CORRECTION):
    light_correction(light_value);
    break;
    }
  }
  }


// Utility Functions 

void GROUP1_IRQHandler(void) {
  uint32_t group_iidx_status;
  uint32_t gpio_mis;

  do {
    group_iidx_status = CPUSS->INT_GROUP[1].IIDX;

    switch (group_iidx_status) {
    // Interrupt from GPIOA
    case (CPUSS_INT_GROUP_IIDX_STAT_INT0):
      gpio_mis = GPIOA->CPU_INT.MIS;
      if ((gpio_mis & GPIO_CPU_INT_ICLR_DIO15_MASK) ==
          GPIO_CPU_INT_MIS_DIO15_SET) {
        g_pb2_pressed = true;
        // Manually clear bit to acknowledge interrupt
        GPIOA->CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO15_CLR;
      }

      break;

    // Interrupt from GPIOB
    case (CPUSS_INT_GROUP_IIDX_STAT_INT1):
      gpio_mis = GPIOB->CPU_INT.MIS;
      if ((gpio_mis & GPIO_CPU_INT_MIS_DIO18_MASK) ==
          GPIO_CPU_INT_MIS_DIO18_SET) {
        g_pb1_pressed = true;
        // Manually clear bit to acknowledge interrupt
        GPIOB->CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO18_CLR;
      }

      break;

    default:
      break;
    }
  } while (group_iidx_status != 0);
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//  This function configures PB1 based on its polarity, Port, and bit
//
// INPUT PARAMETERS:
//    none
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//   none
// -----------------------------------------------------------------------------
void config_pb1_interrupt() {
  // Set PB1 to rising edge (after inversion)
  GPIOB->POLARITY31_16 = GPIO_POLARITY31_16_DIO18_RISE;

  // Ensure bit is cleared
  GPIOB->CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO18_CLR;

  // Unmask PB1 to allow interrupt
  GPIOB->CPU_INT.IMASK = GPIO_CPU_INT_IMASK_DIO18_SET;

  NVIC_SetPriority(GPIOB_INT_IRQn, 2);
  NVIC_EnableIRQ(GPIOB_INT_IRQn);
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//  This function configures PB2 based on its polarity, Port, and bit
//
// INPUT PARAMETERS:
//    none
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//   none
// -----------------------------------------------------------------------------
void config_pb2_interrupt() {
  // Set PB1 to rising edge (after inversion)
  GPIOA->POLARITY15_0 = GPIO_POLARITY15_0_DIO15_RISE;

  // Ensure bit is cleared
  GPIOA->CPU_INT.ICLR = GPIO_CPU_INT_ICLR_DIO15_CLR;

  // Unmask PB1 to allow interrupt
  GPIOA->CPU_INT.IMASK = GPIO_CPU_INT_IMASK_DIO15_SET;

  NVIC_SetPriority(GPIOA_INT_IRQn, 2);
  NVIC_EnableIRQ(GPIOA_INT_IRQn);
}
void scroll_message(const char string[], uint8_t line) {

  // Clears the lcd, disables leds, and initializes switches
  lcd_set_ddram_addr(line);
  lcd_write_string("                ");
  leds_disable();
  dipsw_init();

  // Creates variable for the string, done flag, and for tracking the current
  // address and character

  uint8_t led_address;
  uint8_t character;
  bool done = false;
  uint8_t scroll_count = 0;

  // While done flag is false
  while (!done) {
    // for loop to iterate through lcd addresses
    for (led_address = (line + LCD_CHAR_POSITION_16);
         (led_address > line) && !done; led_address--) {
      // Clears lcd and displays the string using newly created function
      lcd_write_string("                ");
      lcd_set_ddram_addr(led_address);
      lcd_write_string_window(string, led_address, line + LCD_CHAR_POSITION_16);
      msec_delay(CYCLE_DELAY);
    }

    // Creates variable to track the string index
    uint8_t index = 0;

    // While the current index character isn't null and done flag is false
    while ((string[index] != '\0') && !done) {
      // Clears lcd and displays string using the newly created function
      lcd_write_string("                ");
      lcd_set_ddram_addr(line);
      lcd_write_string_window((string + index), line,
                              line + LCD_CHAR_POSITION_16);
      index++;
      msec_delay(CYCLE_DELAY);
    }
    scroll_count++;
    if (scroll_count == 1) {
      done = true;
    }
  }
  // Clears lcd and turns seg7 display off, Displays part 2 done and then clears
  // lcd
  lcd_clear();
  seg7_off();
}

// DESCRIPTION:
//  This function displays the string while taking into account whether the
//  string will fit in the lcd line based on the start and max address
//
// INPUT PARAMETERS:
//    string[]  - A char array that represents the inputted string to be
//    displayed to the lcd
//
//    start_lcd_addr - The starting LCD address where the message should start
//                     on
//
//    max_lcd_addr - The ending LCD address where the message should stop on
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//   none
// -----------------------------------------------------------------------------
void lcd_write_string_window(const char string[], uint8_t start_lcd_addr,
                             uint8_t max_lcd_addr) {
  // counter to track whether or not the current address is within range
  uint8_t counter = 0;

  // while the string isn't null and the counter is within 0-15
  while ((*string != '\0') && (counter <= (max_lcd_addr - start_lcd_addr))) {
    lcd_write_char(*string++);
    counter++;
  } /* while */
}

void interrupt_buzz()
{
  uint8_t beeps = 0;
  
  while (beeps < 2) {

  lp_leds_on(buzzer_enable);
  msec_delay(100);
  lp_leds_off(buzzer_enable);

  beeps++;
  }
  
}

void watering_system(bool moisture_level) {
  if (moisture_level == false) {

    lcd_set_ddram_addr(LCD_LINE1_ADDR);
    lcd_write_string("Water level:LOW ");

    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    lcd_write_string("Watering Plant");

    water_plant();

  } else {

    lcd_clear();
    lcd_set_ddram_addr(LCD_LINE1_ADDR);
    lcd_write_string("Water level:HIGH");

    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    lcd_write_string("Plant Wet");
  }
}

void water_plant() {

  uint8_t count = 0;

  motor0_init();
  motor0_pwm_init(4000, 0);

    motor0_set_pwm_count(SERVO_MIN_COUNT);
    motor0_pwm_enable();
    msec_delay(SERVO_PWM_UPDATE_DELAY);
    msec_delay(200);
    motor0_set_pwm_count(SERVO_MAX_COUNT);
    msec_delay(SERVO_PWM_UPDATE_DELAY);
    msec_delay(200);
  motor0_pwm_disable();
  }


bool days_since_watered() 
{
  bool done = false;
  uint8_t sec_count = 0;
  uint8_t hour_count = 0;
  uint8_t minute_count = 0;
  uint8_t day_count = 0;

  if (sec_count == 1)
    return true;
  sec_count = tick_to_sec(tick_counter); 
  if (sec_count == 59) {
    minute_count++;
    sec_count = 0;
  }
  else if (minute_count == 59) {
    hour_count++;
    minute_count = 0;
  }
  else if (hour_count == 24) {
    day_count++;
    hour_count = 0;
  }

}

/*

uint16_t soil_read()
 {
  uint16_t soil_value = 0;

  soil_value = ADC0_in(5);
  return soil_value;
}

void display_soil(uint16_t adc_reading)
{
  uint8_t led_idx = 0;
  uint16_t moisture_level = adc_reading; 
  uint16_t moisture_value = (moisture_level/step_size);

   lcd_set_ddram_addr(LCD_LINE1_ADDR);

    if (moisture_level < dry_soil_value) 
    {
      lcd_write_string("Status: Soil Wet");
    } 
    else if (moisture_level > dry_soil_value) 
    {
      lcd_write_string("Status: Soil Dry");
    }

    lcd_write_string("ADC:");

    lcd_set_ddram_addr(LCD_LINE2_ADDR);

    lcd_write_doublebyte(moisture_level);
}

void average_adc_values() {
  ADC0->ULLMEM.MEMCTL[0] = ADC12_MEMCTL_AVGEN_ENABLE;
  ADC0->ULLMEM.CTL1 = ADC12_CTL1_AVGN_AVG_16 | ADC12_CTL1_AVGD_SHIFT4;
}

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

void soil_intensity(uint16_t adc_reading)
{
  uint8_t led_idx = 0;
  uint8_t moisture_level = adc_reading; 
  uint16_t moisture_value = (moisture_level/step_size);

  leds_off();

  for (led_idx = 0; led_idx < moisture_value; led_idx++) {
    led_on(led_idx);
  }
}*/