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
    Init_Bluetooth();
    Init_ADC();
    Init_Timer();
    //__enable_irq ( );


    //uint32_t time
    // Might be moving this to a timer interrupt
    while (1){

        //time = millis();

        Sample_ADC();
//        uart_check_command();
//        ble_check_command();

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
