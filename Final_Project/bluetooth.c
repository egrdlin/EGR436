/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "msp.h"
#include "bluetooth.h"
#include "spi.h"

static int ble_buffer_index; // Current place in buffer
static char ble_buffer[BUFFER_SIZE];

/*
 * Initialize settings for bluetooth module
 */
void Init_Bluetooth(){
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
 * Transmit a string of data over Bluetooth to module
 * MUST END IN '\0'. Will not transmit '\0'
 */
void ble_data_TX(char *data){

    int i;
    for(i=0; i<strlen(data); i++){

        // Wait for TX buffer to be ready for new data
        while(!(UCA2IFG & UCTXIFG));

        // Push data to TX buffer
        UCA2TXBUF = data[i];
    }

    // Wait until the last byte is completely sent
    while(UCA2STATW & UCBUSY);
}

/*
 * EUSCI A2 UART ISR - Get characters received over Bluetooth from phone
 * P3.2 and P3.3 for RX and TX
 */
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

    // Commands
    const char TEST_COMMAND[] = "TEST";
    const char CLEAR_COMMAND[] = "CLR"; // Clear FRAM
    const char SET_DATE_COMMAND[] = "SET"; // Set date and time
    const char GET_DATE_COMMAND[] = "GET"; // Get date and time
    const char STOP_RECORDING_COMMAND[] = "STP"; // Get date and time

//    const char SLEEP_COMMAND[] = "SLP"; // Enter sleep mode
//    const char WAKE_COMMAND[] = "WAK"; // Wake from sleep mode

    ble_buffer[ble_buffer_index] = '\0'; // Temporarily make buffer a string to use with strcmp

    if(!strcmp(TEST_COMMAND,ble_buffer)){
        char data[20];
        float num = 3.4567;
        sprintf(data, "This is a number %0.2f!", num);
        ble_data_TX(data);
        //ble_data_TX("AT+ADDR?"); // Example AT command
        ble_reset_transmission();

    }else if(!strcmp(CLEAR_COMMAND,ble_buffer)){
        Clear_FRAM();
        char data[20];
        sprintf(data, "FRAM successfully cleared.");
        ble_data_TX(data);
        ble_reset_transmission();

    }else if(!strcmp(SET_DATE_COMMAND,ble_buffer)){
        char data[20];
        sprintf(data, "Enter in format: MM/DD/YYYY HH:MM");

//        ble_reset_transmission();
//        //while(ble_buffer_index < 16); // Wait for valid number of characters
//
//        char date[17];
//        memcpy(date, data, 16);
//        date[16] = '\0';

        // Validate date entry

        ble_data_TX(data);
        ble_reset_transmission();

    }else if(!strcmp(GET_DATE_COMMAND,ble_buffer)){
        char data[50];
        sprintf(data, "The date is: %x/%x/%x %x:%x", RTCMON, RTCDAY, RTCYEAR, RTCHOUR, RTCMIN);
        ble_data_TX(data);
        ble_reset_transmission();

    }else if(!strcmp(STOP_RECORDING_COMMAND,ble_buffer)){
        char data[20];
        sprintf(data, "Recording paused.");
        ble_data_TX(data);
        ble_reset_transmission();
    }
}

/*
 * Reset transmission variables indicating a transmission has been received
 * so a new transmission can be received.
 */
void ble_reset_transmission(){
    ble_buffer[0] = '\0';
    ble_buffer_index = 0;
}
