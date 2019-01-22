#include <stdio.h>
#include <string.h>
#include "uart.h"
#include "driverlib.h"
#include "timers.h"

#define BUFFER_SIZE 100

const char end_char = '\0';

volatile int buffer_index; // Current place in buffer

volatile uint8_t received_data = 0;

volatile char buffer[BUFFER_SIZE];

volatile int blink_rate; // Blink rate in blinks per minute

int i;

/*
 * Get the blink rate.
 * @return blink rate in blinks per minute
 */
int get_blink_rate(){
    return blink_rate;
}

/*
 * Initialize UART connection (EUSCI A0).
 * Code used from Resource Explorer example: msp432p401x_euscia0_uart_01 - eUSCI_A0 UART echo at 9600 baud using BRCLK = 12MHz
 */
void UART_init(){
    CS->KEY = CS_KEY_VAL;                   // Unlock CS module for register access
    CS->CTL0 = 0;                           // Reset tuning parameters
    CS->CTL0 = CS_CTL0_DCORSEL_3;           // Set DCO to 12MHz (nominal, center of 8-16MHz range)
    CS->CTL1 = CS_CTL1_SELA_2 |             // Select ACLK = REFO = 32.768kHz
            CS_CTL1_SELS_3 |                // SMCLK = DCO
            CS_CTL1_SELM_3;                 // MCLK = DCO
    CS->KEY = 0;                            // Lock CS module from unintended accesses

    // Configure UART pins
    P1->SEL0 |= BIT2 | BIT3;                // set 2-UART pin as secondary function

    // Configure UART
    EUSCI_A0->CTLW0 |= EUSCI_A_CTLW0_SWRST; // Put eUSCI in reset
    EUSCI_A0->CTLW0 = EUSCI_A_CTLW0_SWRST | // Remain eUSCI in reset
            EUSCI_B_CTLW0_SSEL__SMCLK;      // Configure eUSCI clock source for SMCLK
    // Baud Rate calculation
    // 12000000/(16*9600) = 78.125
    // Fractional portion = 0.125
    // User's Guide Table 21-4: UCBRSx = 0x10
    // UCBRFx = int ( (78.125-78)*16) = 2
    EUSCI_A0->BRW = 78;                     // 12000000/16/9600
    EUSCI_A0->MCTLW = (2 << EUSCI_A_MCTLW_BRF_OFS) |
            EUSCI_A_MCTLW_OS16;

    EUSCI_A0->CTLW0 &= ~EUSCI_A_CTLW0_SWRST; // Initialize eUSCI
    EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;    // Clear eUSCI RX interrupt flag
    EUSCI_A0->IE |= EUSCI_A_IE_RXIE;        // Enable USCI_A0 RX interrupt

    // Enable eUSCIA0 interrupt in NVIC module
    NVIC->ISER[0] = 1 << ((EUSCIA0_IRQn) & 31);

    clear_buffer();
}

/*
 * EUSCI A0 UART interrupt service routine
 * Code used from Resource Explorer example: msp432p401x_euscia0_uart_01 - eUSCI_A0 UART echo at 9600 baud using BRCLK = 12MHz
 */
void EUSCIA0_IRQHandler(void)
{
    if (EUSCI_A0->IFG & EUSCI_A_IFG_RXIFG)
    {
        // Check if the TX buffer is empty first
        while(!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));

        // Get character from RX buffer
        uint16_t received_char = EUSCI_A0->RXBUF;

        // Place received character into local buffer
        buffer[buffer_index]= received_char;

        // Increment buffer position or reset if it will go out of bounds
        (buffer_index>BUFFER_SIZE-1) ? buffer_index=0 : buffer_index++;

        // Update blink rate
        update_blink_rate();

        // Echo the received character back
        //EUSCI_A0->TXBUF = EUSCI_A0->RXBUF;
    }
}

/*
 * Update the blink rate based on the received buffer.
 */
void update_blink_rate(){

    int new_blink_rate;

    // Check if received end of message character
    if(buffer_index > 0 && buffer[buffer_index-1] == end_char){

        // Convert character array to integer starting after first character
        sscanf(buffer, "%d", &new_blink_rate);
        blink_rate = new_blink_rate;
        reset_count();
        tx_baud_rate();

        clear_buffer();
    }
}

/*
 * Clear the entire local buffer used to store received UART data.
 */
void clear_buffer(){
    for (i=0;i<BUFFER_SIZE;i++)
    {
        buffer[i]=0;
    }

    buffer_index=0;
}

/*
 * Transmit baud rate string over UART
 */
void tx_baud_rate(){

    char tx_buffer[10] = {0};
    int tx_index = 0;

    // Put blink rate into transmit buffer array
    sprintf(tx_buffer, "%i%c", blink_rate, end_char);

    while(tx_index <= strlen(tx_buffer)){

        // Wait for TX buffer to be ready for new data
        while(!(UCA0IFG & UCTXIFG));

        // Push data to TX buffer
        UCA0TXBUF = tx_buffer[tx_index];

        // Increment index
        tx_index++;
    }

    // Wait until the last byte is completely sent
    while(UCA0STATW & UCBUSY);

}
