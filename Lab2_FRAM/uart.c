#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <string.h>
#include "uart.h"
#include "msp.h"
#include "spi.h"

#define BUFFER_SIZE 200

volatile int buffer_index; // Current place in buffer

char buffer[BUFFER_SIZE];

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

    P1->DIR |= BIT0; /* use P1.6 to manually select chip*/
    P1->OUT |= BIT0; /* pull low to start, pull high to end*/
}

bool received_transmission = false;

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

        if(!received_transmission){
            // Get character from RX buffer
            uint16_t received_char = EUSCI_A0->RXBUF;

            // Place received character into local buffer
            buffer[buffer_index]= received_char;

            if(received_char == '\0'){
                received_transmission = true;
            }

            // Increment buffer position or reset if it will go out of bounds
            (buffer_index>BUFFER_SIZE-1) ? buffer_index=0 : buffer_index++;
        }
        EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;
    }
}

/*
 * Check user input for known commands
 */
void check_command(){

    // Check if received three character string
    if(received_transmission){

        if(!strcmp(buffer,"DIR")){
            char directory[100];
            Directory_TX(directory);
            data_TX(directory);

        }else if(!strcmp(buffer,"MEM")){
            char data[20];
            Get_Size(data);
            data_TX(data);

        }else if(buffer[0] == 'D' && buffer[1] == 'E' && buffer[2] == 'L'){
            int ind = buffer[3] - '0';
            Delete_Poem(ind);

        }else if(buffer[0] == 'R' && buffer[1] == 'E' && buffer[2] == 'D'){
            char poem[100] = {0};
            int ind = buffer[3] - '0';
            Get_Poem(ind, poem);
            data_TX(poem);

        }else if(!strcmp(buffer,"CLR")){
            Clear_FRAM();

        }else if(buffer[0] = 'S' && buffer[1] == 'T' && buffer[2] == 'O'){
            Store_Poem(buffer+3);

        }

        reset_transmission();
    }
}

/*
 * Reset transmission variables indicating a transmission has been received
 * so a new transmission can be received.
 */
void reset_transmission(){
    received_transmission = false;
    buffer_index = 0;
}

/*
 * Clear the entire local buffer used to store received UART data.
 */
void clear_buffer(){

    int i;
    for (i=0;i<BUFFER_SIZE;i++)
    {
        buffer[i]=0;
    }

    buffer_index=0;
}

/*
 * Transmit string over UART
 */
void String_TX(char *data){

    char data_string[10];
    sprintf(data_string,"%04i", strlen(data)+1);
    reset_transmission();

    // Transmit length of string
    data_TX(data_string);

    // Wait for RX transmission
    while(!received_transmission);

    // Verify transmission data
    while(strcmp(buffer,"START")){
        reset_transmission();
        while(!received_transmission);
    }

    // Transmit string
    data_TX(data);
}

/*
 * Transmit a string of data over UART (end character included)
 */
void data_TX(char *data){
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
