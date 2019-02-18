/*
 * Don Lin and Ariel Magyar
 * Lab 3: Bluetooth Low Energy
 * EGR 436 101
 * 2/19/2019
 */

#include "msp.h"
#include "spi.h"
#include "uart.h"
#include "bluetooth.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer

    SPI_FRAM_init();
    UART_init();
    Init_bluetooth();

    while (1){
        uart_check_command();
    }
}
