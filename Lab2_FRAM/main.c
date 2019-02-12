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

    /*********** FRAM Read IDs Test **********/
    uint16_t manufacturerID, productID;

    Read_ID(&manufacturerID, &productID);
//    fprintf(stderr, "Manufacturer ID: 0x%04x\n", manufacturerID);
//    fprintf(stderr, "Product ID: 0x%04x\n", productID);

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

    /*********** Poem Functions Test ***************/
    uint8_t test_doc1[] = {'1','o','e','m','.','t','x','t','\n','T','h','i','s',' ','i','s',' ','a',' ','p','o','e','m','.'};
    uint8_t test_doc2[] = {'2','b','c','d','.','t','x','t','\n','T','h','i','s',' ','i','s',' ','a',' ','p','o','e','m','.'};
    uint8_t test_doc3[] = {'3','f','g','h','.','t','x','t','\n','T','h','i','s',' ','i','s',' ','a',' ','p','o','e','m','.'};
    uint8_t test_doc4[] = {'4','b','c','d','.','t','x','t','\n','T','h','i','s',' ','i','s',' ','a',' ','p','o','e','m','.'};
    uint8_t test_doc5[] = {'5','f','g','h','.','t','x','t','\n','T','h','i','s',' ','i','s',' ','a',' ','p','o','e','m','.'};
//    char *str1 = "I love EGR 436 ^J ";
//    char str[80];
//    strcpy(str, "The truth is ");
//    strcat(str, str1);
    Store_Poem(test_doc1, 24);
    Store_Poem(test_doc2, 24);
    Store_Poem(test_doc3, 24);
    Store_Poem(test_doc4, 24);
    Store_Poem(test_doc5, 24);
   // Clear_FRAM();
    uint8_t str_poem [100] = {0};
    //Get_Poem(str_poem, 3); // str_poem reads the poem from the index 3
    Delete_Poem(2);

    Get_Poem(str_poem, 3);
    while (1){

    }
}
