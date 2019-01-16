#include <stdint.h>

#include "msp.h"
#include "timers.h"
#include "uart.h"

int main(void) {
    WDT_A->CTL = WDT_A_CTL_PW |             // Stop WDT
            WDT_A_CTL_HOLD;

    // Initialize UART and Timers
    UART_init();
    Init_TA0();

    // Configure GPIO
    P1->DIR |= BIT0;
    P1->OUT |= BIT0;

    //SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;    // Enable sleep on exit from ISR


    while(1);
    //__sleep();
    //__no_operation();
}


