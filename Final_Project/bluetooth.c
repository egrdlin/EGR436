/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "msp.h"
#include "bluetooth.h"
#include "spi.h"
#include "timer.h"
#include "adc.h"

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

uint32_t startTime; // Time first character was entered in buffer
const uint32_t BLE_TIMEOUT = 500; // Time before buffer is reset

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

        // Check if this is first character in buffer
        if(ble_buffer_index == 0){

            startTime = millis();

        // Reset transmission if characters are in buffer for too long
        // (protection against invalid commands messing up buffer)
        }else if(millis() < startTime || millis() - startTime > BLE_TIMEOUT){
            ble_reset_transmission();
            startTime = millis();
        }

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
    //const char SET_DATE_COMMAND[] = "SET"; // Set date and time
    const char GET_DATE_COMMAND[] = "GET"; // Get date and time and recording status
    const char STOP_RECORDING_COMMAND[] = "STP"; // Stop data recording and turn off system
    const char START_RECORDING_COMMAND[] = "STT"; // Start data recording and turn on system (if during recording hours)

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

    }else if(verify_set_date(ble_buffer)){

        // Unlock RTC key protected registers
        // RTC enable, BCD mode, RTC hold
        // enable RTC read ready interrupt
        // enable RTC time event interrupt
        // set time event interrupt to trigger when minute changes
        RTC_C->CTL0 = RTC_C_KEY | RTC_C_CTL0_TEVIE;
        RTC_C->CTL13 = RTC_C_CTL13_HOLD |
                RTC_C_CTL13_MODE |
                RTC_C_CTL13_BCD |
                RTC_C_CTL13_TEV_0;

        uint8_t month = ((ble_buffer[4] - 0x30) << 4) | (ble_buffer[5] - 0x30);
        uint8_t day = ((ble_buffer[7] - 0x30) << 4) | (ble_buffer[8] - 0x30);
        uint8_t year_first = ((ble_buffer[10] - 0x30) << 4) | (ble_buffer[11] - 0x30);
        uint8_t year_second = ((ble_buffer[12] - 0x30) << 4) | (ble_buffer[13] - 0x30);
        uint16_t year_full = year_first << 8  | year_second;

        uint8_t hour = ((ble_buffer[15] - 0x30) << 4) | (ble_buffer[16] - 0x30);
        uint8_t minute = ((ble_buffer[18] - 0x30) << 4) | (ble_buffer[19] - 0x30);

        //month 45
        // day 78
        // year 10 11 12 13

        // hour 15 16
        // minute 18 19

        RTC_C->YEAR = year_full;                   // Year = 0x2019
        RTC_C->DATE = (month << RTC_C_DATE_MON_OFS) | // Month = 0x03 = March
                (day | RTC_C_DATE_DAY_OFS);    // Day = 0x13 = 13th
        RTC_C->TIM1 = (0x01 << RTC_C_TIM1_DOW_OFS) | // Day of week = 0x01 = Monday
                (hour << RTC_C_TIM1_HOUR_OFS);  // Hour = 0x10
        RTC_C->TIM0 = (minute << RTC_C_TIM0_MIN_OFS) | // Minute = 0x32
                (0x00 << RTC_C_TIM0_SEC_OFS);   // Seconds = 0x45

        // Start RTC calendar mode
        RTC_C->CTL13 = RTC_C->CTL13 & ~(RTC_C_CTL13_HOLD);

        // Lock the RTC registers
        RTC_C->CTL0 = RTC_C->CTL0 & ~(RTC_C_CTL0_KEY_MASK);

        char data[50];
        sprintf(data, "Date: %x/%x/%x %x:%x:%x", RTCMON, RTCDAY, RTCYEAR, RTCHOUR, RTCMIN, RTCSEC);
        ble_data_TX(data);

        ble_reset_transmission();

    }else if(!strcmp(GET_DATE_COMMAND,ble_buffer)){
        char data[50];

        sprintf(data, "Date: %x/%x/%x %x:%x:%x Recording: %s In: %hu Out: %hu", RTCMON, RTCDAY, RTCYEAR, RTCHOUR, RTCMIN, RTCSEC, Get_Recording_Status() ? "On" : "Off", Get_In_Count(), Get_Out_Count());

//        if(Get_Recording_Status()){
//            sprintf(data, "Date: %x/%x/%x %x:%x Recording: %s In: %", RTCMON, RTCDAY, RTCYEAR, RTCHOUR, RTCMIN, Get_Recording_Status() ? "On" : "Off", );
//        }else{
//            sprintf(data, "Date: %x/%x/%x %x:%x Recording: Off", RTCMON, RTCDAY, RTCYEAR, RTCHOUR, RTCMIN);
//        }
        ble_data_TX(data);
        ble_reset_transmission();

    }else if(!strcmp(STOP_RECORDING_COMMAND,ble_buffer)){
        BLE_Stop_Recording();
        char data[20];

        if(Get_Recording_Status()){
            sprintf(data, "Recording: On");
        }else{
            sprintf(data, "Recording: Off");
        }

        ble_data_TX(data);
        ble_reset_transmission();

    }else if(!strcmp(START_RECORDING_COMMAND,ble_buffer)){
        BLE_Start_Recording();
        char data[20];
        if(Get_Recording_Status()){
            sprintf(data, "Recording: On");
        }else{
            sprintf(data, "Recording: Off");
        }
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

bool verify_set_date(char *entry){
    const char SET_COMMAND[] = "SET MM/DD/YYYY HH:MM";
    if(strlen(entry) == strlen(SET_COMMAND)
        && entry[0] == SET_COMMAND[0]
        && entry[1] == SET_COMMAND[1]
        && entry[2] == SET_COMMAND[2]
        && entry[3] == SET_COMMAND[3]
        && entry[6] == SET_COMMAND[6]
        && entry[9] == SET_COMMAND[9]
        && entry[14] == SET_COMMAND[14]
        && entry[17] == SET_COMMAND[17]){
        return true;
    }

    return false;
}
