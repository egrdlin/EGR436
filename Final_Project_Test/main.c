#include "rtc.h"
#include "msp.h"
#include "spi.h"
#include "uart.h"
//#include "sensor.h"
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
    //Init_bluetooth();
    init_rtc();
    //init_sensor();

    load_fram();

//    int num = Get_Num_Entries(), i;
//    char buffer[50];
//    for(i=1;i<num+1;i++){
//        Get_Time(i, buffer);
//    }

    while (1){



            //Get_Time(0,buffer);

        uart_check_command();
        //ble_check_command();
//        sample_adc();
//        Get_Fram(buffer);
//
//        Get_Time(1, buffer2);
//        Get_Time(2, buffer3);

    }
}
