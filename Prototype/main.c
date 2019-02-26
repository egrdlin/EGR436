#include <spi.h>
#include "msp.h"
#include "driverlib.h"

#include "RTC.h"
#include "bluetooth.h"
#include "UART.h"
#include "spi.h"

int main(void) {
    volatile unsigned int i;

    WDT_A->CTL = WDT_A_CTL_PW |             // Stop WDT
                 WDT_A_CTL_HOLD;

    // GPIO Setup
    P1->OUT &= ~BIT0;                       // Clear LED to start
    P1->DIR |= BIT0;                        // Set P1.0/LED to output
    P5->SEL1 |= BIT4;                       // Configure P5.4 for ADC
    P5->SEL0 |= BIT4;



    Init_bluetooth();
    SPI_FRAM_init();
    UART_init();
    Init_RTC();

    // Enable global interrupt
    __enable_irq();

    // Enable ADC interrupt in NVIC module
    NVIC->ISER[0] = 1 << ((ADC14_IRQn) & 31);

    // Sampling time, S&H=16, ADC14 on
    ADC14->CTL0 = ADC14_CTL0_SHT0_2 | ADC14_CTL0_SHP | ADC14_CTL0_ON;
    ADC14->CTL1 = ADC14_CTL1_RES_2;         // Use sampling timer, 12-bit conversion results

    ADC14->MCTL[0] |= ADC14_MCTLN_INCH_1;   // A1 ADC input select; Vref=AVCC
    ADC14->IER0 |= ADC14_IER0_IE0;          // Enable ADC conv complete interrupt

    SCB->SCR &= ~SCB_SCR_SLEEPONEXIT_Msk;   // Wake up on exit from ISR

    while (1)
    {
        for (i = 20000; i > 0; i--);        // Delay

        //ble_check_command();
        //uart_check_command();
        // Start sampling/conversion
        ADC14->CTL0 |= ADC14_CTL0_ENC | ADC14_CTL0_SC;
        //__sleep();

       // __no_operation();                   // For debugger
    }
}

int is_covered = 0;

// ADC14 interrupt service routine
void ADC14_IRQHandler(void) {
    if (ADC14->MEM[0] >= 0xF64)             // Bee in between LED and sensor
      P1->OUT |= BIT0;                      // P1.0 = 1
        
        // Check if this is the first time the interrupt is triggered since being uncovered
        if(is_covered == 0){
            char date[8];
            RTC_read(date); // Get time from RTC
            date[7] = '\0'; // Convert time to string
            //write_date(date); // Write date to FRAM
            is_covered = 1;
        }

    else
      P1->OUT &= ~BIT0;                     // P1.0 = 0
      is_covered = 0;
}
