#include "uart.h"
#include "msp.h"
#include "timers.h"

// Keeps track of how many times the Timer A0 ISR is hit. Resets when it reaches the desired
// maximum value determined by blink rate.
extern volatile int count = 0;
extern volatile int LED_flag = 0;
/*
 * This function initializes Timer A0 in Up mode and ties it to ACLK (32768Hz).
 */
void Init_TA0(){
    // Enable global interrupt
    __enable_irq();

    // Enable the TA0_0_IRQHandler
    NVIC->ISER[0] = 1 << ((TA0_0_IRQn) & 31);

    // Configure register values
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
    TIMER_A0->CCTL[0] = TIMER_A_CCTLN_CCIE; // TACCR0 interrupt enabled
    TIMER_A0->CCR[0] = 32768.0/60.0; // Set the increment value
    TIMER_A0->CTL = TIMER_A_CTL_SSEL__ACLK | // ACLK, UP mode
            TIMER_A_CTL_MC__UP;
}

/*
 * This function is the Timer A0 interrupt service routine. It is called every time
 * Timer A's TAR value hits CCR0 (overflow trigger). The LED output is adjusted based
 * on how many times (count) the CCR0 value is hit. The LED is turned on slightly before
 * the blink_rate value is hit so that it turns on for the same amount of time regardless
 * of blink_rate.
 */
void TA0_0_IRQHandler() {
    // Clear the compare interrupt flag
//    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
//    count++;
//    int blink_rate = get_blink_rate();
//    if(count == blink_rate - 5 || blink_rate <= 5){
//        P1->OUT |= BIT0;    // Turn on P1.0 LED
//    }else if (count == blink_rate){
//        P1->OUT &= ~BIT0;    // Turn off P1.0 LED
//        count = 0;          // Reset count
//    }
    int blink_rate = get_blink_rate();
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

    if(!LED_flag)
    {
    P1->OUT &= ~BIT0;   // Turn on P1.0 LED
    TIMER_A0->CCR[0] = ((32768.0*60.0)/(blink_rate)) - 1000.0;
    LED_flag = 1;
    }
    else{
    P1->OUT |= BIT0;    // Turn on P1.0 LED
    TIMER_A0->CCR[0] = 1000;
    LED_flag = 0;
    }


}

/*
 * Reset the count value when the blink rate is updated.
 */
void reset_count(){
    count = 0;
}
