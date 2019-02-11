#include "msp.h"
#include "spi.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer

    SPI_FRAM_init();


    /*********** FRAM Read & Write Test **********/
//    uint8_t test_doc1[] = {'p','o','e','m','.','t','x','t','\n','T','h','i','s',' ','i','s',' ','a',' ','p','o','e','m','.'};
//
//    // Write to FRAM
//    int i;
//    uint8_t temp;
//    for(i=0; i<24;i++){
//        Write_Enable(true);
//        Write8(i, test_doc1[i]);
//        Write_Enable(false);
//
//        temp = Read8(i);
//    }
//
//    uint8_t read_data[100] = {0};
//
//    // Read from FRAM
//    for(i=0; i<24;i++){
//
//        read_data[i] = Read8(i);
//    }

    /*********** Poem Test ***************/
    uint8_t test_doc1[] = {'p','o','e','m','.','t','x','t','\n','T','h','i','s',' ','i','s',' ','a',' ','p','o','e','m','.'};

    Store_Poem(test_doc1, 24);
    Get_Poem(1);

    while (1){

    }
}


//        uint16_t manufacturerID, productID;
//
//        Read_ID(&manufacturerID, &productID);
//        fprintf(stderr, "Manufacturer ID: 0x%02x\n", manufacturerID);
//        fprintf(stderr, "Product ID: 0x%02x\n", productID);

//        Write_Enable(true);
//        Write8(0, test + 1);
//        Write_Enable(false);

//        // dump the entire 8K of memory!
//        uint8_t value;
//        uint16_t a;
//        for (a = 0; a < 8192; a++) {
//          value = Read8(a);
//          if ((a % 32) == 0) {
//            fprintf(stderr, "\n 0x%x: ", a);
//          }
//          fprintf(stderr, "0x");
//          if (value < 0x1)
//            fprintf(stderr, "0");
//          fprintf(stderr, "%x ", value);
//        }
