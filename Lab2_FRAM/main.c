/*
 * Don Lin and Ariel Magyar
 * Lab 2: Data Storage and Retrieval
 * EGR 436 101
 * 2/12/2019
 */

#include "msp.h"
#include "spi.h"
#include "uart.h"
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

    while (1){
        check_command();
    }
}
