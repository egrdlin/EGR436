#include <stdio.h>
#include <string.h>
#include <string.h>
#include "uart.h"
#include "msp.h"
#include "spi.h"

#define BUFFER_SIZE 1000

static int uart_buffer_index; // Current place in buffer
static char uart_buffer[BUFFER_SIZE];

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

    uart_clear_buffer();

    P1->DIR |= BIT0; /* use P1.6 to manually select chip*/
    P1->OUT |= BIT0; /* pull low to start, pull high to end*/
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
        while(!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG) && (EUSCI_A0->STATW & EUSCI_A_STATW_BUSY));

        // Get character from RX buffer
        uint16_t received_char = EUSCI_A0->RXBUF;

        // Place received character into local buffer
        uart_buffer[uart_buffer_index]= received_char;

        // Increment buffer position or reset if it will go out of bounds
        uart_buffer_index = (uart_buffer_index+1) % BUFFER_SIZE;

        EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;
    }
}

/*
 * Check user input for known commands
 */
void uart_check_command(){
    // Commands
    const char DELETE_COMMAND[] = "DELETE ";
    const char STORE_COMMAND[] = "STORE ";
    const char READ_COMMAND[] = "READ ";
    const char DIR_COMMAND[] = "DIR";
    const char MEM_COMMAND[] = "MEM";
    const char CLEAR_COMMAND[] = "CLEAR";

    if(uart_comp_command(DELETE_COMMAND)){
        int ind = uart_buffer[strlen(DELETE_COMMAND)] - '0';
        Delete_Poem(ind);
        uart_reset_transmission();

    }else if(uart_comp_command(STORE_COMMAND)){
        while(uart_buffer[uart_buffer_index-1] != '\0'); // Wait for termination character
        int length = strlen(uart_buffer);
        Store_Poem(uart_buffer+strlen(STORE_COMMAND));
        uart_reset_transmission();

    }else if(uart_comp_command(READ_COMMAND)){
        char poem[1000] = {0};
        int ind = uart_buffer[strlen(READ_COMMAND)] - '0';
        Get_Poem(ind, poem);
        uart_data_TX(poem);
        uart_reset_transmission();

    }else if(uart_comp_command(DIR_COMMAND)){
        char directory[1000];
        Directory_TX(directory);
        uart_data_TX(directory);
        uart_reset_transmission();

    }else if(uart_comp_command(MEM_COMMAND)){
        char data[20];
        Get_Size(data);
        uart_data_TX(data);
        uart_reset_transmission();

    }else if(uart_comp_command(CLEAR_COMMAND)){
        Clear_FRAM();
        uart_reset_transmission();
    }
}

/*
 * Reset transmission variables indicating a transmission has been received
 * so a new transmission can be received.
 */
void uart_reset_transmission(){
    //received_transmission = false;
    uart_buffer_index = 0;
}

/*
 * Clear the entire local buffer used to store received UART data.
 */
void uart_clear_buffer(){

    int i;
    for (i=0;i<BUFFER_SIZE;i++)
    {
        uart_buffer[i]=0;
    }

    uart_buffer_index=0;
}

/*
 * Transmit a string of data over UART (end character included)
 */
void uart_data_TX(char *data){
    int tx_index = 0;

    int length = (strlen(data)+1);
    while(tx_index <= (strlen(data)+1)){

        // Wait for TX buffer to be ready for new data
        while(!(UCA0IFG & UCTXIFG));

        // Push data to TX buffer
        UCA0TXBUF = data[tx_index];

        // Increment index
        tx_index++;
    }

    // Wait until the last byte is completely sent
    while(UCA0STATW & UCBUSY);
}

bool uart_comp_command(const char *checkCommand){
    bool compare = true;

    // Check command length
    if(uart_buffer_index < strlen(checkCommand) + 1){
        compare = false;
    }else{
        int i;
        for(i=0; i<strlen(checkCommand); i++){
            if(checkCommand[i] != uart_buffer[i]){
                compare = false;
                break;
            }
        }
    }

    return compare;
}
