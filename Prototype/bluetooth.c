/* Standard Includes */
#include <spi.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "msp.h"
#include "bluetooth.h"

static int ble_buffer_index; // Current place in buffer
static char ble_buffer[BUFFER_SIZE];

/*
 * Initialize settings for bluetooth module
 */
void Init_bluetooth(){
    WDT_A->CTL = WDT_A_CTL_PW |             // Stop watchdog timer
            WDT_A_CTL_HOLD;

    CS->KEY = CS_KEY_VAL;                   // Unlock CS module for register access
    CS->CTL0 = 0;                           // Reset tuning parameters
    CS->CTL0 = CS_CTL0_DCORSEL_3;           // Set DCO to 12MHz (nominal, center of 8-16MHz range)
    CS->CTL1 = CS_CTL1_SELA_2 |             // Select ACLK = REFO
            CS_CTL1_SELS_3 |                // SMCLK = DCO
            CS_CTL1_SELM_3;                 // MCLK = DCO
    CS->KEY = 0;                            // Lock CS module from unintended accesses

    // Configure UART pins 3.2 RX, 3.3 TX
    P3->SEL0 |= BIT2 | BIT3;                // set 2-UART pin as secondary function

    // Configure UART
    EUSCI_A2->CTLW0 |= EUSCI_A_CTLW0_SWRST; // Put eUSCI in reset
    EUSCI_A2->CTLW0 = EUSCI_A_CTLW0_SWRST | // Remain eUSCI in reset
            EUSCI_B_CTLW0_SSEL__SMCLK;      // Configure eUSCI clock source for SMCLK
    // Baud Rate calculation
    // 12000000/(16*9600) = 78.125
    // Fractional portion = 0.125
    // User's Guide Table 21-4: UCBRSx = 0x10
    // UCBRFx = int ( (78.125-78)*16) = 2
    EUSCI_A2->BRW = 78;                     // 12000000/16/9600
    EUSCI_A2->MCTLW = (2 << EUSCI_A_MCTLW_BRF_OFS) |
            EUSCI_A_MCTLW_OS16;

    //EUSCI_A2->CTLW0 = ; // Initialize eUSCI
    EUSCI_A2->CTLW0 &= ~EUSCI_A_CTLW0_SWRST; // Initialize eUSCI
    EUSCI_A2->IFG &= ~EUSCI_A_IFG_RXIFG;    // Clear eUSCI RX interrupt flag
    EUSCI_A2->IE |= EUSCI_A_IE_RXIE;        // Enable USCI_A2 RX interrupt

    // Enable global interrupt
    __enable_irq();

    // Enable eUSCIA2 interrupt in NVIC module
    NVIC->ISER[0] = 1 << ((EUSCIA2_IRQn) & 31);

}

/*
 * Transmit a string of data over UART
 */
void ble_data_TX(char *data){
    int tx_index = 0;

    int length = (strlen(data));
    while(tx_index <= (strlen(data)+1)){

        // Wait for TX buffer to be ready for new data
        while(!(UCA2IFG & UCTXIFG));

        // Push data to TX buffer
        UCA2TXBUF = data[tx_index];

        // Increment index
        tx_index++;
    }

    // Wait until the last byte is completely sent
    while(UCA2STATW & UCBUSY);
}

/* EUSCI A2 UART ISR - Echoes data back to PC host */
// P3.2 and P3.3 for RX and TX
void EUSCIA2_IRQHandler(void)
{
    // Check for RX interrupt
    if (EUSCI_A2->IFG & EUSCI_A_IFG_RXIFG)
    {
        // Wait until TX finished
        while(!(EUSCI_A2->IFG & EUSCI_A_IFG_TXIFG) && (EUSCI_A2->STATW & EUSCI_A_STATW_BUSY));

        // Get character from RX buffer
        uint8_t received_char = EUSCI_A2->RXBUF;

        // Place received character into local buffer
        ble_buffer[ble_buffer_index]= received_char;

        // Increment buffer position or reset if it will go out of bounds
        ble_buffer_index = (ble_buffer_index+1) % BUFFER_SIZE;

        // Clear RX interrupt
        EUSCI_A2->IFG &= ~EUSCI_A_IFG_RXIFG;
    }
}

/*
 * Check user input for known commands
 */
void ble_check_command(){


    const char GET_DATES_COMMAND[] = "GET";

    if(ble_comp_command(GET_DATES_COMMAND)){
        char dates[500];
        int length = get_dates(dates);
        dates[length] = '\0';
        ble_data_TX(dates);
        ble_reset_transmission();

    }
}

/*
 * Reset transmission variables indicating a transmission has been received
 * so a new transmission can be received.
 */
void ble_reset_transmission(){
    ble_buffer_index = 0;
}

bool ble_comp_command(const char *checkCommand){
    bool compare = true;

    // Check command length
    if(ble_buffer_index < strlen(checkCommand)){
        compare = false;
    }else{
        int i;
        for(i=0; i<strlen(checkCommand); i++){
            if(checkCommand[i] != ble_buffer[i]){
                compare = false;
                break;
            }
        }
    }

    return compare;
}
