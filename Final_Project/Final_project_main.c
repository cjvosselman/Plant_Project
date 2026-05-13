//*****************************************************************************
//*****************************    C Source Code    ***************************
//*****************************************************************************
//  DESIGNER NAME:  Joshua Carlson and Casey Vosselman
//
//       PROJECT NAME:  Plant Monitoring System
//
//      FILE NAME:  Final_project_main.c
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//    This project runs on the LP_MSPM0G3507 LaunchPad board interfacing to
//    the CSC202 Expansion board.
//
//    This code gives the user a proper plant monitoring system that allows the
//    user to view the readings of the plants water and light values. This
//    program will automatically adjust so that the plant is receiving proper
//    treatment and the perfect amount of water and light.
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
#include "light_sensor.h"
#include "soil_sensor.h"
#include "spi.h"
#include "timer.h"
#include "uart.h"
#include <ti/devices/msp/msp.h>

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

#define step_size 450
#define light_threshold 1800
#define dark_threshold 1200
#define optimal_value 1600

#define adc12_max 4096
#define soil_threshold 2200 // Dry is above | Wet is below
#define adc_to_tens(adc_val) ((adc_val) * (10 / adc12_max))

#define tick_to_sec(tick_counter) (tick_counter / 10)

#define buzzer_enable (LP_RGB_BLU_LED_IDX)

#define MAX_DUTY_CYCLE 100
#define MIN_DUTY_CYCLE 0

#define SECOND_DELAY (1000)
#define HALF_SECOND_DELAY (SECOND_DELAY / 2)
#define ONES_DELAY (5)
#define TENS_DELAY (2)

//-----------------------------------------------------------------------------
// Define global variables and structures here.
// NOTE: when possible avoid using global variables
//-----------------------------------------------------------------------------
bool g_pb2_pressed = false;
bool g_pb1_pressed = false;
extern uint8_t duty_cycle;

// Takes in a global value from timer file for ones and tens place in the
// counter
extern volatile uint8_t ones_seconds;
extern volatile uint8_t tens_seconds;

volatile uint8_t tick_counter;

// Define a structure to hold different data types
typedef enum {
  STANDBY,
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
  TIMG8_config(LOAD_VALUE);
  TIMG8_enable_interrupt();
  TIMG8_enable();
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

  // Endless loop to prevent program from ending
  while (1)
    ;

} /* main */

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    This function holds the high-level portion of the monitoring system, which
//    uses a state machine to assert different states. It constantly takes in
//    the readings of light and soil, and runs different states according to
//    those values, allowing the plant to constantly monitored
//
//    States:
//      STANDBY - The initial (standby) state that displays that the monitoring
//      system is active
//
//      LIGHT_DISPLAY - Displays the light adc value, as well as whether the
//      light is too bright, too dark, or perfect
//
//      MOISTURE_DISPLAY - Displays the moisture value taken from the moisture
//      sensor and whether it is dry or wet
//
//      MOISTURE_ALERT - If the plant is dry, this state runs, which alerts the
//      user via buzzer, and opens the water pump (motor) to water the plant.
//      This state also resets the days since watered timer on the seg7
//
//      LIGHT_CORRECTION - If the plant is too dark or too bright, this state
//      automatically adjusts the light, incrementing or decrementing the led to
//      show the adjustment
//
// INPUT PARAMETERS:
//    none
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//    none
// -----------------------------------------------------------------------------
void run_monitoring_system() {

  // Initializes the done and state flags to ensure the program keeps running
  // and that the states shifted through correctly
  bool done = false;
  PLNT_STS_t state = STANDBY;

  // While not done
  while (!done) {

    // Initializes the dry and soil and light value variables to constantly read
    // the light and moisture levels of the plant
    bool dry = false;
    uint16_t soil_value = ADC0_in(5);
    uint16_t light_value = ADC1_in(6);

    // If pushbutton 1 is pressed, change state to MOISTURE DISPLAY unless
    // already in another state other than STANDBY. Clears lcd
    if (g_pb1_pressed) {
      if (state == STANDBY) {
        state = MOISTURE_DISPLAY;
      } else {
        state = 0;
      }
      lcd_clear();
      g_pb1_pressed = false;
    }

    // If pushbutton 2 is pressed, change state to LIGHT DISPLAY unless
    // already in another state other than STANDBY. Clears lcd
    if (g_pb2_pressed) {
      if (state == STANDBY) {
        state = LIGHT_DISPLAY;
      } else {
        state = 0;
      }
      lcd_clear();
      g_pb2_pressed = false;
    }

    // Checks if the soil is dry. If it's dry, go to MOISTURE_ALERT state and
    // water the plant
    if (soil_value > (uint16_t)dry_soil_value) {
      state = MOISTURE_ALERT;
      dry = true;
    }

    // Checks if the light value is too dark or too bright. If it is, go to
    // LIGHT_CORRECTION state and adjust the light
    if ((light_value > light_threshold) || (light_value < dark_threshold)) {
      state = LIGHT_CORRECTION;
    }

    // Constantly counts the days since watered on the seg7
    seg7_hex(ones_seconds, SEG7_DIG3_ENABLE_IDX);
    msec_delay(ONES_DELAY);
    seg7_hex(tens_seconds, SEG7_DIG2_ENABLE_IDX);
    msec_delay(TENS_DELAY);

    // Start of switch statement
    switch (state) {
    case (STANDBY):

      // Displays that the monitoring system is active
      lcd_set_ddram_addr(LCD_LINE1_ADDR);
      lcd_write_string("Plant Monitoring");
      lcd_set_ddram_addr(LCD_LINE2_ADDR);
      lcd_write_string("     Active     ");

      break;

    case (LIGHT_DISPLAY):

      // Runs the light_display function to display light levels
      light_display(light_value);

      break;

    case (MOISTURE_DISPLAY):

      // Runs the display_soil function to display soil moisture levels
      display_soil(soil_value);

      break;

    case (MOISTURE_ALERT):

      // Clears the lcd and starts the interrupt_buzz and watering_system
      // function in order to alert the user and water the plant. Resets the
      // timer, and the dry and state values
      lcd_clear();
      interrupt_buzz();
      watering_system(dry);
      dry = 0;
      state = 0;
      ones_seconds = 0;
      tens_seconds = 0;

      break;

    case (LIGHT_CORRECTION):

      // Runs the light_correction value that takes in the light_value and
      // adjusts it until it is perfect
      lcd_set_ddram_addr(LCD_LINE1_ADDR + LCD_CHAR_POSITION_13);
      lcd_write_string("    ");
      light_correction(light_value);

      //Resets state back to standby
      state = 0;
      break;
    }
  }
}

// Utility Functions

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    This function detects interrupts from GPIOA and GPIOB, under the Group 1
//    interrupt, in order to detect when PB1 and PB2 have been pressed. If PB1
//    or PB2 are pressed, set their boolean flags to true.
//
// INPUT PARAMETERS:
//    none
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//    none
// -----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    This function utilizes the LP_LED pin to enable the active buzzer
//    repeatedly, allowing an alarm like noise to be played. This is to alert
//    the user that the plant needs more water
//
// INPUT PARAMETERS:
//    duty_cycle - an 8-bit value that represents the desired duty cycle
//                 percentage (0-100) used to calculate the timer's threshold.
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//    none
// -----------------------------------------------------------------------------
void interrupt_buzz() {
  uint8_t beeps = 0;

  while (beeps < 2) {

    lp_leds_on(buzzer_enable);
    msec_delay(100);
    lp_leds_off(buzzer_enable);

    beeps++;
  }
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    This function displays whether or not the water level is Low (dry) or high
//    (wet), and activates the water_plant function if plant is dry
//
// INPUT PARAMETERS:
//    moisture_level - boolean value that determines if the plant is dry or wet,
//    with false meaning wet and true meaning true
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//    none
// -----------------------------------------------------------------------------
void watering_system(bool moisture_level) {
  // If statement that checks if the soil is dry or wet
  if (moisture_level == true) {

    // Displays the water status onto the lcd
    lcd_set_ddram_addr(LCD_LINE1_ADDR);
    lcd_write_string("Water level:LOW ");

    // Since water is wet, display that it will start watering the plant
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    lcd_write_string("Watering Plant");

    // Runs the water_plant function
    water_plant();

  } else {

    lcd_clear();
    lcd_set_ddram_addr(LCD_LINE1_ADDR);
    lcd_write_string("Water level:HIGH");

    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    lcd_write_string("Plant Wet");
  }
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    This function controls the servo motor using the PWM. By controlling the
//    count of the PWM, it turns the servo motor to different angles. This
//    function starts the motor at the furthest degree to start (-90 degrees),
//    turns the motor to the opposite side (90 degrees), and the turns it back
//    to the start
//
// INPUT PARAMETERS:
//    none
//
// OUTPUT PARAMETERS:
//    none
//
// RETURN:
//    none
// -----------------------------------------------------------------------------
void water_plant() {

  // Initializes the motor
  motor0_init();
  motor0_pwm_init(4000, 0);

  // Sets the servo motors position to -90 degrees (water pump closed) and
  // enables the motor
  motor0_set_pwm_count(SERVO_MIN_COUNT);
  motor0_pwm_enable();

  // Delay to ensure proper update of PWM count
  msec_delay(SERVO_PWM_UPDATE_DELAY);
  msec_delay(HALF_SECOND_DELAY);

  // Sets the servo motors position to 90 degrees (Water Pump Open)
  motor0_set_pwm_count(SERVO_MAX_COUNT);

  // Delay to ensure proper update of PWM count
  msec_delay(SERVO_PWM_UPDATE_DELAY);
  msec_delay(HALF_SECOND_DELAY);

  // Resets the servo motors position to -90 degrees (Water pump closed)
  motor0_set_pwm_count(SERVO_MIN_COUNT);

  // Delays the disabling of the motor
  msec_delay(SERVO_PWM_UPDATE_DELAY);
  motor0_pwm_disable();
  msec_delay(SECOND_DELAY);
}
