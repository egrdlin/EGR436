#include "uart.h"
#include "driverlib.h"
#include "timers.h"

#define BUFFER_SIZE 100

volatile int buffer_index; // Current place in buffer

volatile uint8_t received_data = 0;

volatile char buffer[BUFFER_SIZE];

volatile int blink_rate; // Blink rate in blinks per minute

int i;

const int DEFAULT_BLINK_RATE = 60;

//void UART2_init() /* Initialize UART */
//
//{
//
//    EUSCI_A2 -> CTLW0 |= 1; /* put in reset mode for config*/
//    EUSCI_A2 -> MCTLW = 0;  /* disable over-sampling */
//    EUSCI_A2 -> CTLW0 = 0x0081; /* 1 stop bit, no parity, SMCLK, 8 bit data*/
//    EUSCI_A2 -> BRW = 26; /* 3,000,000 Hz / 115200 (Baud rate) = 26 */
//
//    P3->SEL0 |= 0x0C; /*conf P3.2 and P3.3 for UART*/
//    P3->SEL1 &=~ 0x0C;
//
//    EUSCI_A2 -> CTLW0 &= ~ 1; /* take UART out of reset mode*/
//
//    /* Enabling interrupts (UART) */
//    MAP_UART_enableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
//
//    MAP_Interrupt_enableInterrupt(INT_EUSCIA2);
//
//    MAP_Interrupt_enableMaster();
//
//    /*clear buffer*/
//    for (i=0;i<BUFFER_SIZE;i++)
//    {
//        buffer[i]=0;
//    }
//
//    // Set initial blink rate
//    blink_rate = 60;
//}

//void EUSCIA2_IRQHandler(void)
//{
//    uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A2_BASE);
//
//    MAP_UART_clearInterruptFlag(EUSCI_A2_BASE, status);
//
//    if(status & EUSCI_A_UART_RECEIVE_INTERRUPT)
//
//    {
//        // Toggle LED to show data was received
//
//        //P1OUT ^= BIT0;
//
//        received_data = MAP_UART_receiveData(EUSCI_A2_BASE);
//
//        // Restart buffer if reached maximum
//
//        if(buffer_index>BUFFER_SIZE-1) {
//
//            buffer_index=0;
//        }
//
//        // Store data in buffer.
//
//        // Not using a buffer causes problems with data being received too quickly.
//
//        buffer[buffer_index++]= received_data;
//
//        MAP_UART_transmitData(EUSCI_A2_BASE, received_data);  // send byte out UART2 port
//
//    }
//}

//void check_buffer(void)
//{
//    if(buffer[buffer_index] == '\n')
//    {
//
//        if(buffer_index==1)
//        {
//            switch(buffer[0])
//            {
//            case 'd':
//            /* baud rate decreases by 2bpm */
//            /* LED blink rate decreases by 2bpm */
//                blink_rate -= 2;
//                break;
//            case 'u':
//                /* baud rate increases by 2bpm */
//                /* LED blink rate increases by 2bpm */
//                blink_rate += 2;
//                break;
//            case 'r':
//                /*reset blink rate to 60 bpm */
//                blink_rate = 60;
//                break;
//            }
//        }
//
//        /* clear buffer*/
//        for (i=0;i<=buffer_index;i++)
//        {
//            buffer[i]=0;
//        }
//
//        buffer_index=0;
//    }
//
//}

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
    //P1->SEL0 |= BIT2 | BIT3;                // set 2-UART pin as secondary function
    P3->SEL0 |= 0x0C;                       // P3.3 and P3.2 configured for UART2
    P3->SEL1 &= ~0x0C;
    // Configure UART
    EUSCI_A2->CTLW0 |= EUSCI_A_CTLW0_SWRST; // Put eUSCI in reset STATE
    EUSCI_A2->CTLW0 = EUSCI_A_CTLW0_SWRST | // Remain eUSCI in reset
            EUSCI_B_CTLW0_SSEL__SMCLK;      // Configure eUSCI clock source for SMCLK, 1 stop bit, no parity, 8-bit data
    /*Baud Rate calculation
    // 12000000/(16*9600) = 78.125
    // Fractional portion = 0.125
    // User's Guide Table 21-4: UCBRSx = 0x10
    //UCBRFx = int ( (78.125-78)*16) = 2
     */

    EUSCI_A2->BRW = 78;                     // 12000000/16/9600
    EUSCI_A2->MCTLW = (2 << EUSCI_A_MCTLW_BRF_OFS) |
            EUSCI_A_MCTLW_OS16;  // enable over sampling mode

    EUSCI_A2->CTLW0 &= ~EUSCI_A_CTLW0_SWRST; // take UART2 out of reset mode
    EUSCI_A2->IFG &= ~EUSCI_A_IFG_RXIFG;    // Clear eUSCI RX interrupt flag
    EUSCI_A2->IE |= EUSCI_A_IE_RXIE;        // Enable USCI_A0 RX interrupt

    // Enable eUSCIA0 interrupt in NVIC module
    NVIC->ISER[0] = 1 << ((EUSCIA0_IRQn) & 31);

    clear_buffer_full();

    // Set initial blink rate
    blink_rate = DEFAULT_BLINK_RATE;
}

/*
 * EUSCI A0 UART interrupt service routine
 * Code used from Resource Explorer example: msp432p401x_euscia0_uart_01 - eUSCI_A0 UART echo at 9600 baud using BRCLK = 12MHz
 */
void EUSCIA2_IRQHandler(void)
{
    if (EUSCI_A2->IFG & EUSCI_A_IFG_RXIFG)
    {
        // Check if the TX buffer is empty first
        while(!(EUSCI_A2->IFG & EUSCI_A_IFG_TXIFG));

        // Get character from RX buffer
        uint16_t received_char = EUSCI_A2->RXBUF;

        // Place received character into local buffer
        buffer[buffer_index]= received_char;

        // Increment buffer position or reset if it will go out of bounds
        (buffer_index>BUFFER_SIZE-1) ? buffer_index=0 : buffer_index++;

        // Update blink rate
        update_blink_rate(received_char);

        // Echo the received character back
        EUSCI_A2->TXBUF = EUSCI_A2->RXBUF;
    }
}

/*
 * Update the blink rate based on input character.
 * @param input_char User input to change blink rate
 *      d: Decrease blink rate by 2bpm
 *      u: Increase blink rate by 2pbm
 *      r: Reset blink rate to 60bpm
 * @return Status of update
 *      0: Error occurred, could not update
 *      1: Successfully updated
 */
int update_blink_rate(uint16_t input_char){
    int update_status = 0;

    switch(input_char){
        case 'd':
            /* baud rate decreases by 2bpm */
            /* LED blink rate decreases by 2bpm */
            (blink_rate >= 2) ?  (blink_rate -= 2) : (blink_rate = 0);
            update_status = 1;
            reset_count();
            break;
        case 'u':
            /* baud rate increases by 2bpm */
            /* LED blink rate increases by 2bpm */
            blink_rate += 2;
            update_status = 1;
            reset_count();
            break;
        case 'r':
            /*reset blink rate to 60 bpm */
            blink_rate = DEFAULT_BLINK_RATE;
            update_status = 1;
            reset_count();
            break;
    }

    clear_buffer_at_index();

    return update_status;
}

/*
 * Clear the local buffer used to store received UART data only up to the current index.
 */
void clear_buffer_at_index(){
if(buffer_index == BUFFER_SIZE-1){

    for (i=0;i<=buffer_index;i++)
    {
        buffer[i]=0;
    }

    buffer_index=0;
}
}

/*
 * Clear the entire local buffer used to store received UART data.
 */
void clear_buffer_full(){
    for (i=0;i<BUFFER_SIZE;i++)
    {
        buffer[i]=0;
    }

    buffer_index=0;
}
