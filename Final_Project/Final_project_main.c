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

//-----------------------------------------------------------------------------
// Define function prototypes used by the program
//-----------------------------------------------------------------------------
void UART_write_string(const char *string);
void run_monitoring_system();
void water_plant();
void watering_system(bool moisture_level);
void config_pb1_interrupt();
void config_pb2_interrupt();

void scroll_message(const char string[], uint8_t line);
void lcd_write_string_window(const char string[], uint8_t start_lcd_addr,
                             uint8_t max_lcd_addr);

void days_since_watered();

void light_read_display();
void read_display_soil();

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
//-----------------------------------------------------------------------------
// Define global variables and structures here.
// NOTE: when possible avoid using global variables
//-----------------------------------------------------------------------------
bool g_pb2_pressed = false;
bool g_pb1_pressed = false;

extern volatile uint8_t ones_seconds;
extern volatile uint8_t tens_seconds;

// Define a structure to hold different data types
typedef enum { LIGHT_READING = 0, MOISTURE_READING } sensor_t;

int main(void) {
  // Configure the LaunchPad board
  clock_init_40mhz();
  launchpad_gpio_init();
  dipsw_init();
  I2C_mstr_init();
  lcd1602_init();
  UART_init(115200);
  seg7_init();
  TIMG8_config(LOAD_VALUE, COMPARE_VALUE);
  TIMG8_enable_interrupt();
  TIMG8_enable();
  ADC0_init(ADC12_MEMCTL_VRSEL_VDDA_VSSA);
  ADC1_init(ADC12_MEMCTL_VRSEL_INTREF_VSSA);

  TIMA0_C0_init();
  TIMA0_C0_pwm_init(4000, 0);
  TIMA0_C0_pwm_enable();

  // Enable interrupts
  __enable_irq();
  config_pb1_interrupt();
  config_pb2_interrupt();

  // enter your code here
  run_monitoring_system();
  // run_monitoring_system();

  // Endless loop to prevent program from ending
  while (1)
    ;

} /* main */

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

// -----------------------------------------------------------------------------
// DESCRIPTION
//    This function writes a string to the Serial terminal
//
//
// INPUT PARAMETERS:
//    string    - A pointer (address) to the null-terminated string to be
//                written to the serial terminal
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//    none
// -----------------------------------------------------------------------------
void UART_write_string(const char *string) {
  // for each character in string, write it to the UART module
  while (*string != '\0') {
    UART_out_char(*string++);
  } /* while */

} /* UART_write_string */

void run_monitoring_system() {

  bool done = false;
  sensor_t reading = LIGHT_READING;

  while (!done) {

    switch (reading) {
    case LIGHT_READING:
      light_read_display();
      seg7_hex(ones_seconds, SEG7_DIG3_ENABLE_IDX);
      msec_delay(5);
      seg7_hex(tens_seconds, SEG7_DIG2_ENABLE_IDX);
      msec_delay(2);
      break;

    case MOISTURE_READING:
      read_display_soil();
      seg7_hex(ones_seconds, SEG7_DIG3_ENABLE_IDX);
      msec_delay(5);
      seg7_hex(tens_seconds, SEG7_DIG2_ENABLE_IDX);
      msec_delay(2);
      break;
    }

    if (g_pb1_pressed) {
      if (reading == LIGHT_READING) {
        reading = MOISTURE_READING;
      } else {
        reading--;
      }
      g_pb1_pressed = false;
    }

    if (g_pb2_pressed) {
      if (reading == MOISTURE_READING) {
        reading = LIGHT_READING;
      } else {
        reading++;
      }
      g_pb2_pressed = false;
    }
  }
}

void watering_system(bool moisture_level) {
  if (!moisture_level) {

    lcd_set_ddram_addr(LCD_LINE1_ADDR);
    lcd_write_string("Water lvl: ");
    lcd_set_ddram_addr(LCD_LINE1_ADDR + LCD_CHAR_POSITION_12);
    lcd_write_string("LOW");

    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    lcd_write_string("Watering Plant");

    water_plant();

  } else {

    lcd_clear();
    lcd_set_ddram_addr(LCD_LINE1_ADDR);
    lcd_write_string("Water lvl: ");
    lcd_set_ddram_addr(LCD_LINE1_ADDR + LCD_CHAR_POSITION_12);
    lcd_write_string("High");

    scroll_message("Watering Not Needed", LCD_LINE2_ADDR);
  }
}

void water_plant() {

  uint8_t count = 0;

  motor0_init();
  motor0_pwm_init(4000, 0);

  while (count < 4) {
    motor0_set_pwm_count(SERVO_MIN_COUNT);
    motor0_pwm_enable();
    msec_delay(SERVO_PWM_UPDATE_DELAY);
    msec_delay(500);
    motor0_set_pwm_count(SERVO_MAX_COUNT);
    msec_delay(SERVO_PWM_UPDATE_DELAY);
    msec_delay(500);

    count++;
  }
  motor0_pwm_disable();
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

void days_since_watered() {
  timerA_config(LOAD_VALUE, COMPARE_VALUE);
  timerA_enable_interrupt();
  timerA_enable();

  bool donee = false;

  while (!donee) {

    seg7_hex(ones_seconds, SEG7_DIG3_ENABLE_IDX);
    msec_delay(5);
    seg7_hex(tens_seconds, SEG7_DIG2_ENABLE_IDX);
    msec_delay(5);
  }
}
