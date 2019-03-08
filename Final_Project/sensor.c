#include "msp.h"

void init_sensor(){
    // GPIO Setup
    P1->OUT &= ~BIT0;                       // Clear LED to start
    P1->DIR |= BIT0;                        // Set P1.0/LED to output
    P5->SEL1 |= BIT4;                       // Configure P5.4 for ADC
    P5->SEL0 |= BIT4;

    // Enable ADC interrupt in NVIC module
    NVIC->ISER[0] = 1 << ((ADC14_IRQn) & 31);

    // Sampling time, S&H=16, ADC14 on
    ADC14->CTL0 = ADC14_CTL0_SHT0_2 | ADC14_CTL0_SHP | ADC14_CTL0_ON;
    ADC14->CTL1 = ADC14_CTL1_RES_2;         // Use sampling timer, 12-bit conversion results

    ADC14->MCTL[0] |= ADC14_MCTLN_INCH_1;   // A1 ADC input select; Vref=AVCC
    ADC14->IER0 |= ADC14_IER0_IE0;          // Enable ADC conv complete interrupt

    SCB->SCR &= ~SCB_SCR_SLEEPONEXIT_Msk;   // Wake up on exit from ISR
}

/*
 * Start the ADC sampling/conversion
 */
void sample_adc(){
    ADC14->CTL0 |= ADC14_CTL0_ENC | ADC14_CTL0_SC;
}

int is_covered = 0;

// ADC14 interrupt service routine
void ADC14_IRQHandler(void) {

    // TODO: Create way of calibrating sensors
    if (ADC14->MEM[0] >= 0xF64)             // Bee in between LED and sensor
      P1->OUT |= BIT0;                      // P1.0 = 1

        // Check if this is the first time the interrupt is triggered since being uncovered
        if(is_covered == 0){
            //char date[8];
            //RTC_read(date); // Get time from RTC
            //date[7] = '\0'; // Convert time to string
            //write_date(date); // Write date to FRAM
            is_covered = 1;
        }

    else
      P1->OUT &= ~BIT0;                     // P1.0 = 0
      is_covered = 0;
}
