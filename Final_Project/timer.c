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
#include "clock.h"
#include "timer.h"
#include <ti/devices/msp/msp.h>
#include <ti/devices/msp/peripherals/hw_iomux.h>

volatile uint8_t ones_seconds = 0;
volatile uint8_t tens_seconds = 0;

void timerA_config(uint32_t load_value, uint32_t compare_value) {

  IOMUX->SECCFG.PINCM[IOMUX_PINCM32] =
      IOMUX_PINCM32_PF_TIMG8_CCP0 | IOMUX_PINCM_PC_CONNECTED;

  // Reset TIMG8
  TIMG8->GPRCM.RSTCTL =
      (GPTIMER_RSTCTL_KEY_UNLOCK_W | GPTIMER_RSTCTL_RESETSTKYCLR_CLR |
       GPTIMER_RSTCTL_RESETASSERT_ASSERT);

  // Enable power to TIMG8
  TIMG8->GPRCM.PWREN =
      (GPTIMER_PWREN_KEY_UNLOCK_W | GPTIMER_PWREN_ENABLE_ENABLE);

  // Wait for 24 bus cycles
  clock_delay(24);

  // Selects LFCLK as clock source
  TIMG8->CLKSEL =
      (GPTIMER_CLKSEL_BUSCLK_SEL_ENABLE | GPTIMER_CLKSEL_MFCLK_SEL_DISABLE |
       GPTIMER_CLKSEL_LFCLK_SEL_DISABLE);

  // Configures Timer Clock
  TIMG8->CLKDIV = GPTIMER_CLKDIV_RATIO_DIV_BY_4;

  TIMG8->COMMONREGS.CPS = GPTIMER_CPS_PCNT_MASK & TIMER_CPS_PCNT;

  TIMG8->COUNTERREGS.LOAD = GPTIMER_LOAD_LD_MASK & (load_value - 1);

  TIMG8->COMMONREGS.CCLKCTL = GPTIMER_CCLKCTL_CLKEN_ENABLED;

  // Sets TIMG8_C3 as output
  TIMG8->COMMONREGS.CCPD =
      (GPTIMER_CCPD_C0CCP3_INPUT | GPTIMER_CCPD_C0CCP2_INPUT |
       GPTIMER_CCPD_C0CCP1_INPUT | GPTIMER_CCPD_C0CCP0_OUTPUT);

  TIMG8->CPU_INT.IMASK = 0x00000000;

  // When enabled, timer starts at 0, counts up, and then stops

  TIMG8->COUNTERREGS.CTRCTL =
      GPTIMER_CTRCTL_CVAE_ZEROVAL | GPTIMER_CTRCTL_CZC_CCCTL2_ZCOND |
      GPTIMER_CTRCTL_REPEAT_REPEAT_1 | GPTIMER_CTRCTL_CM_UP;

  // Set timer reload value
  TIMG8->COUNTERREGS.LOAD = GPTIMER_LOAD_LD_MASK & (load_value - 1);
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    This function enables the timer to start counting.
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
void timerA_enable(void) {
  TIMG8->COUNTERREGS.CTRCTL |= (GPTIMER_CTRCTL_EN_ENABLED);
} /* timerA_enable */
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    This function disable the timer to start counting.
//
// INPUT PARAMETERS:
//  none
//
// OUTPUT PARAMETERS:
//  none
//
// RETURN:
//  none
// -----------------------------------------------------------------------------
void timerA_disable(void) {
  TIMG8->COUNTERREGS.CTRCTL &= ~(GPTIMER_CTRCTL_EN_ENABLED);
} /* timerA_disable */

//-----------------------------------------------------------------------------
// DESCRIPTION:
//    This function enables interrupts for Timer G8 by first clearing any
//    pending interrupts and then unmasking specific interrupt conditions.
//    It also sets the priority for the Timer G8 interrupt and enables the
//    interrupt in the NVIC (Nested Vectored Interrupt Controller).
//
//    NOTE: ADJUST INTERRUPTS AS NEEDED
//
// INPUT PARAMETERS:
//     None
//
// OUTPUT PARAMETERS:
//     None
//
// RETURN:
//     None
//-----------------------------------------------------------------------------
void timerA_enable_interrupt(void) {
  // Clear all pre-existing interrupts that might be set
  TIMG8->CPU_INT.ICLR =
      GPTIMER_CPU_INT_ICLR_DC_CLR | GPTIMER_CPU_INT_ICLR_REPC_CLR |
      GPTIMER_CPU_INT_ICLR_TOV_CLR | GPTIMER_CPU_INT_ICLR_F_CLR |
      GPTIMER_CPU_INT_ICLR_CCD0_CLR | GPTIMER_CPU_INT_ICLR_CCD1_CLR |
      GPTIMER_CPU_INT_ICLR_CCD2_CLR | GPTIMER_CPU_INT_ICLR_CCD3_CLR |
      GPTIMER_CPU_INT_ICLR_CCD4_CLR | GPTIMER_CPU_INT_ICLR_CCD5_CLR |
      GPTIMER_CPU_INT_ICLR_CCU0_CLR | GPTIMER_CPU_INT_ICLR_CCU1_CLR |
      GPTIMER_CPU_INT_ICLR_CCU2_CLR | GPTIMER_CPU_INT_ICLR_CCU3_CLR |
      GPTIMER_CPU_INT_ICLR_CCU4_CLR | GPTIMER_CPU_INT_ICLR_CCU5_CLR |
      GPTIMER_CPU_INT_ICLR_Z_CLR | GPTIMER_CPU_INT_ICLR_L_CLR;

  // Unmask conditions to allow interrupt
  TIMG8->CPU_INT.IMASK = GPTIMER_CPU_INT_IMASK_L_SET;

  // Set priority and enable
  NVIC_SetPriority(TIMG8_INT_IRQn, 2);
  NVIC_EnableIRQ(TIMG8_INT_IRQn);
} /* timerA_enable_interrupt */

// DESCRIPTION:
//    This is the interrupt handler for Timer G8 (TIMG8). It checks the
//    interrupt index register (IIDX) to identify the type of interrupt event
//    that occurred. Based on the interrupt event, the function processes the
//    event accordingly:
//     - Zero event (Z): Counts the number of time the ISR is called. After
//                       the appropriate delay, the count value is decremented
//                       and sent to the seven-segment display.
//
//    The function continues checking for interrupts until all events are
//    cleared.
//
//    NOTE: ADJUST PROCESSING AS NEEDED
//
// INPUT PARAMETERS:
//     None
//
// OUTPUT PARAMETERS:
//     None
//
// RETURN:
//     None
//-----------------------------------------------------------------------------
void TIMG8_IRQHandler(void) {
  uint32_t timer_iidx;
  static uint16_t isr_call_count = MAX_ISR_COUNT_DELAY;

  

  do {
    timer_iidx = TIMG8->CPU_INT.IIDX;
    switch (timer_iidx) {
    // Check if overflow event
    case (GPTIMER_CPU_INT_IIDX_STAT_TOV):
      break;

    // Check if load event
    case (GPTIMER_CPU_INT_IIDX_STAT_L):
      // isr_call_count++;
      // if (isr_call_count >= 1000)
      // {
      //   isr_call_count = 0;

      // }
      ones_seconds++;
      if (ones_seconds > 9)
      {
        tens_seconds++;
        ones_seconds = 0;
      }
      if (tens_seconds > 9)
      {
        tens_seconds = 0;
      }
      
      

      break;

    // Check if CC event on CCP2 (ECHO)
    case (GPTIMER_CPU_INT_IIDX_STAT_CCU2):
      break;

    // Check if zero event
    case (GPTIMER_CPU_INT_IIDX_STAT_Z):

      break;

    // Check if CC event on CCP3
    case (GPTIMER_CPU_INT_IIDX_STAT_CCU3):
      break;

    // Check if unexpected event
    default:
      break;

    } /* switch */

  } while (timer_iidx != 0);

} /* TIMG8_IRQHandler */

///////////////////////////////////////////////////////////////////////////////


void TIMA0_C0_init(void)
{
  // Set PA28 (LD0) for TIMA0_C0
  IOMUX->SECCFG.PINCM[IOMUX_PINCM19] = IOMUX_PINCM19_PF_TIMA0_CCP0 | 
                                    IOMUX_PINCM_PC_CONNECTED;

} /* TimA0_C0_init */

void TIMA0_C0_pwm_init(uint32_t load_value, uint32_t compare_value)
{
  // Reset TIMA0
  TIMA0->GPRCM.RSTCTL = (GPTIMER_RSTCTL_KEY_UNLOCK_W | 
        GPTIMER_RSTCTL_RESETSTKYCLR_CLR | GPTIMER_RSTCTL_RESETASSERT_ASSERT);

  // Enable power to TIMA0
  TIMA0->GPRCM.PWREN = (GPTIMER_PWREN_KEY_UNLOCK_W | 
        GPTIMER_PWREN_ENABLE_ENABLE);

  clock_delay(24);

  TIMA0->CLKSEL = (GPTIMER_CLKSEL_BUSCLK_SEL_ENABLE | 
        GPTIMER_CLKSEL_MFCLK_SEL_DISABLE | GPTIMER_CLKSEL_LFCLK_SEL_DISABLE);

  TIMA0->CLKDIV = GPTIMER_CLKDIV_RATIO_DIV_BY_8;

  // Set the pre-scale count value that divides selected clock by PCNT+1
  // TimerClock = BusCock / (DIVIDER * (PRESCALER))
  // 200,000 Hz = 40,000,000 Hz / (8 * (24 + 1))
  TIMA0->COMMONREGS.CPS = GPTIMER_CPS_PCNT_MASK & 0x18;

  // Set C3 action for compare 
  // On Zero, set output HIGH; On Compares up, set output LOW
  TIMA0->COUNTERREGS.CCACT_01[0] = (GPTIMER_CCACT_01_FENACT_DISABLED  | 
        GPTIMER_CCACT_01_CC2UACT_DISABLED | GPTIMER_CCACT_01_CC2DACT_DISABLED |
        GPTIMER_CCACT_01_CUACT_CCP_LOW | GPTIMER_CCACT_01_CDACT_DISABLED | 
        GPTIMER_CCACT_01_LACT_DISABLED | GPTIMER_CCACT_01_ZACT_CCP_HIGH);

  // Set timer reload value
  TIMA0->COUNTERREGS.LOAD = GPTIMER_LOAD_LD_MASK & (load_value - 1);

  // Set timer compare value
  TIMA0->COUNTERREGS.CC_01[0] = GPTIMER_CC_01_CCVAL_MASK & compare_value;

  // Set compare control for PWM func with output initially low
  TIMA0->COUNTERREGS.OCTL_01[0] = (GPTIMER_OCTL_01_CCPIV_LOW | 
        GPTIMER_OCTL_01_CCPOINV_NOINV | GPTIMER_OCTL_01_CCPO_FUNCVAL);
  
  // Set to capture mode with writes to CC register has immediate effect 
  TIMA0->COUNTERREGS.CCCTL_01[0] = (GPTIMER_CCCTL_01_CCUPD_IMMEDIATELY |
        GPTIMER_CCCTL_01_COC_COMPARE | 
        GPTIMER_CCCTL_01_ZCOND_CC_TRIG_NO_EFFECT |
        GPTIMER_CCCTL_01_LCOND_CC_TRIG_NO_EFFECT |
        GPTIMER_CCCTL_01_ACOND_TIMCLK | GPTIMER_CCCTL_01_CCOND_NOCAPTURE);

  // When enabled counter is 0, set counter counts up
  TIMA0->COUNTERREGS.CTRCTL = (GPTIMER_CTRCTL_CVAE_ZEROVAL | 
        GPTIMER_CTRCTL_PLEN_DISABLED | GPTIMER_CTRCTL_SLZERCNEZ_DISABLED |
        GPTIMER_CTRCTL_CM_UP | GPTIMER_CTRCTL_REPEAT_REPEAT_1);

  // Enable the clock
  TIMA0->COMMONREGS.CCLKCTL = GPTIMER_CCLKCTL_CLKEN_ENABLED;

  // No interrupt is required
  TIMA0->CPU_INT.IMASK = (GPTIMER_CPU_INT_IMASK_Z_CLR | 
        GPTIMER_CPU_INT_IMASK_L_CLR | GPTIMER_CPU_INT_IMASK_CCD0_CLR |
        GPTIMER_CPU_INT_IMASK_CCD1_CLR | GPTIMER_CPU_INT_IMASK_CCU0_CLR |
        GPTIMER_CPU_INT_IMASK_CCU1_CLR | GPTIMER_CPU_INT_IMASK_F_CLR |
        GPTIMER_CPU_INT_IMASK_TOV_CLR | GPTIMER_CPU_INT_IMASK_DC_CLR | 
        GPTIMER_CPU_INT_IMASK_QEIERR_CLR | GPTIMER_CPU_INT_IMASK_CCD2_CLR |
        GPTIMER_CPU_INT_IMASK_CCD3_CLR | GPTIMER_CPU_INT_IMASK_CCU2_CLR |
        GPTIMER_CPU_INT_IMASK_CCU3_CLR | GPTIMER_CPU_INT_IMASK_CCD4_CLR |
        GPTIMER_CPU_INT_IMASK_CCD5_CLR | GPTIMER_CPU_INT_IMASK_CCU4_CLR |
        GPTIMER_CPU_INT_IMASK_CCU5_CLR | GPTIMER_CPU_INT_IMASK_REPC_CLR);

  // Set TIMA0_C0 as output
  TIMA0->COMMONREGS.CCPD =(GPTIMER_CCPD_C0CCP0_OUTPUT | 
         GPTIMER_CCPD_C0CCP2_INPUT | GPTIMER_CCPD_C0CCP1_INPUT |  
         GPTIMER_CCPD_C0CCP0_INPUT);

} /* TIMA0_C0_pwm_init */

void TIMA0_C0_pwm_enable(void)
{
    TIMA0->COUNTERREGS.CTRCTL |= (GPTIMER_CTRCTL_EN_ENABLED);
} /*TIMA0_C0_pwm_enable */

void TIMA0_C0_set_pwm_dc(uint8_t duty_cycle)
{
  uint32_t threshold = (TIMA0->COUNTERREGS.LOAD * duty_cycle) / 100;

  TIMA0->COUNTERREGS.CC_01[0] = GPTIMER_CC_01_CCVAL_MASK & threshold;
} /* TIMA0_C0_set_pwm */
