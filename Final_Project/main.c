/*
 * Don Lin and Ariel Magyar
 * Final Project
 * EGR 436 101
 * 4/2019
 */

#include "adc.h"
#include "rtc.h"
#include "msp.h"
#include "spi.h"
#include "uart.h"
#include "bluetooth.h"
#include "timer.h"
#include "power.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer

    Init_Power();
    Init_RTC();
    Init_SPI_FRAM();
    Init_UART();
    Init_ADC();
    Init_Timer();
    Init_Bluetooth();

//    SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;    // Sleep on exit from ISR
//
//    __sleep();
//    __no_operation();                   // For debugger

    while (1){

        if(millis()%1000 == 0){
            uart_check_command();
            ble_check_command();
        }

        if(millis()%1000 == 0){
            Sample_ADC();
        }

    }

    /****** FRAM and UART test ******/
//    Init_SPI_FRAM();
//    Init_UART();
//    Init_Bluetooth();
//    Init_Timer();
//
//    load_fram();
//
//    while (1){
//        uart_check_command();
//        ble_check_command();
//
//    }
    /********************************/
}
