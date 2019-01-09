#include "uart.h"
#include "driverlib.h"


#define BUFFER_SIZE 100


int buffer_index; // Current place in buffer

volatile uint8_t received_data = 0;

volatile uint8_t buffer[BUFFER_SIZE];

int blink_rate;

int i;

void UART2_init() /* Initialize UART */

{
    EUSCI_A2 -> CTLW0 |= 1; /* put in reset mode for config*/
    EUSCI_A2 -> MCTLW = 0;  /* disable over-sampling */
    EUSCI_A2 -> CTLW0 = 0x0081; /* 1 stop bit, no parity, SMCLK, 8 bit data*/
    EUSCI_A2 -> BRW = 26; /* 3,000,000 Hz / 115200 (Baud rate) = 26 */

    P3->SEL0 |= 0x0C; /*conf P3.2 and P3.3 for UART*/
    P3->SEL1 &=~ 0x0C;

    EUSCI_A2 -> CTLW0 &= ~ 1; /* take UART out of reset mode*/

    /* Enabling interrupts (UART) */

        MAP_UART_enableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);

        MAP_Interrupt_enableInterrupt(INT_EUSCIA2);

        MAP_Interrupt_enableMaster();
        /*clear buffer*/

        for (i=0;i<BUFFER_SIZE;i++)
        {
            buffer[i]=0;
        }

}

void EUSCIA2_IRQHandler(void)
{
    uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A2_BASE);

    MAP_UART_clearInterruptFlag(EUSCI_A2_BASE, status);

    if(status & EUSCI_A_UART_RECEIVE_INTERRUPT)

    {
        // Toggle LED to show data was received

        P1OUT ^= BIT0;

        received_data = MAP_UART_receiveData(EUSCI_A2_BASE);

        // Restart buffer if reached maximum

        if(buffer_index>BUFFER_SIZE-1) {

            buffer_index=0;
        }

        // Store data in buffer.

        // Not using a buffer causes problems with data being received too quickly.

        buffer[buffer_index++]= received_data;

        MAP_UART_transmitData(EUSCI_A2_BASE, received_data);  // send byte out UART2 port

    }
}

void check_buffer(void)
{
    if(buffer[buffer_index] == '\n')
    {

        if(buffer_index==1)
        {
            switch(buffer[0])
            {
            case 'd':
            /* baud rate decreases by 2bpm */
            /* LED blink rate decreases by 2bpm */
                blink_rate -= 2;
                break;
            case 'u':
                /* baud rate increases by 2bpm */
                /* LED blink rate increases by 2bpm */
                blink_rate += 2;
                break;
            case 'r':
                /*reset blink rate to 60 bpm */
                blink_rate = 60;
                break;
            }
        }

        /* clear buffer*/
        for (i=0;i<=buffer_index;i++)
        {
            buffer[i]=0;
        }

        buffer_index=0;
    }

}
