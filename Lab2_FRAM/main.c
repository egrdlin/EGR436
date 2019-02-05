#include "msp.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

uint8_t RXData = 0;
uint8_t TXData;
// Status register
#define FRAM_WPEN       0x80
#define FRAM_BP1        0x08
#define FRAM_BP0        0x04
#define FRAM_WEL        0x02

#define OPCODE_WREN    (uint16_t)(0x06)     /* Write Enable Latch */
#define OPCODE_WRDI    (uint16_t)(0x04)     /* Reset Write Enable Latch */
#define OPCODE_RDSR    (uint16_t)(0x05)     /* Read Status Register */
#define OPCODE_WRSR    (uint16_t)(0x01)     /* Write Status Register */
#define OPCODE_READ    (uint16_t)(0x03)     /* Read Memory */
#define OPCODE_WRITE   (uint16_t)(0x02)     /* Write Memory */
#define OPCODE_RDID    (uint16_t)(0x9F)     /* Read Device ID */

#define FRAM_MAX_TRANSACTION 20 //mem size
#define FRAM_MEM_SIZE   0x8000  //Memory size


static int spi_init = 0;

void SPI_tx(uint8_t value) // Using EUSCIA1
{

    EUSCI_A1->IFG |= EUSCI_A_IFG_TXIFG; // Clear TXIFG flag
    EUSCI_A1->IE |= EUSCI_A__TXIE;      // Enable TX interrupt

    /* Wait for transmit buffer empty*/
    while (!(EUSCI_A1->IFG & 2 ));


    /* write to SPI transmit buffer */
    EUSCI_A1->TXBUF = value;

    /* Wait for transmit done */
    while(EUSCI_A1->STATW & 1);


}

void SPI_A1_pin_init(void)
{
    PMAP->KEYID = 0x2D52; /* Unlock PMAP*/
    P2MAP->PMAP_REGISTER[1] = 0x0800; /* Map P2.3 to PM_UCA1CLK*/
    P2MAP->PMAP_REGISTER[2] = 0x070A; /* Map P2.4 to PM_UCA1_SIMO, Map P2.5 to PM_UCA1STE */
    P2MAP->PMAP_REGISTER[3] = PMAP_UCA1SOMI; /* Map P2.6 to PM_UCA1SOMI*/

    P2->SEL0 |= 0x78;               /* set alternate function to pin map */
    P2->SEL1 &= ~0x78;              /* for P2.3, P2.4, P2.5, P2.6 */
    PMAP->CTL = 0;                  /* lock PMAP */
    PMAP->KEYID = 0;

    /*
     *  P2.5 = UCA1STE - slave transmit enable
     *  P2.3 = UCA1CLK - clock
     *  P2.6 = UCA1SOMI - slave out master in
     *  P2.4 = UCA1SIMO - slave in master out
     * */
}
void SPI_FRAM_init(void)
{
    SPI_A1_pin_init();


    EUSCI_A1->CTLW0 = 0x0001; /* Disable UCA1 during configuration*/
    /* SPI through EUSCI_A1:
     * clock phase/polarity: 11,    MSB first,  8-bit,  master mode,
     * 4-pin SPI,   STE Low active,     sychronous mode,    use SMCLK
     * as clock source, STE for slave enable*/
    EUSCI_A1->CTLW0 = 0xEDC3;

    EUSCI_A1->BRW = 1; /* 3MHz / 1 = 3MHz*/
    EUSCI_A1->CTLW0 &= ~0x0001; /* Enable UCA1 after configuration*/

    EUSCI_B0->IFG |= EUSCI_B_IFG_TXIFG; // Clear TXIFG flag
    EUSCI_B0->IE |= EUSCI_B__TXIE;      // Enable TX interrupt
}


int FRAM_init(void)
{
    char buf[5] = {0};
    char ID;// id of the slave device
    uint16_t add_id;

    printf("\nDetecting chip...\n");
    buf[0] = OPCODE_RDID; // RDID command reads fixed slave device ID
    SPI_tx(buf[0]); // send buf with RDID command
//    ID = FRAM_read_byte(add_id);
    printf("\n-------------------------------------------\n");
//    printf("[*] Manufacturer ID: %02x%02x Product ID: %02x%02x\n", ID[1], ID[2], ID[3], ID[4]);
    printf("-------------------------------------------\n\n");

    if (buf[1] == 0x04) // if the first byte reads: 0000 0010
    {
        spi_init = 1; //initialized
    }

    return 0;
}

int FRAM_ready(void)
{
    if (spi_init == 1) // if slave is initialized
    {
        printf("READY. SPI/FRAM is initialized\n");
        return 1;
    }
    else
    {

        printf("ERROR. SPI/FRAM is not initialized\n");
        return -1;
    }
}

void FRAM_write_enable(void)
{       // Transfer WREN to enable write
    SPI_tx((uint8_t)OPCODE_WREN);
}

void FRAM_write_disable(void)
{       // Transfer WRDI to disable write
    SPI_tx((uint8_t)(OPCODE_WRDI));
}

int FRAM_read_byte(uint16_t addr)
{

    int err;
    char buf[4] = {0};

    err = FRAM_ready();
    if (err)
    {
        printf("SPI/FRAM is not initialized\n");
        return -1;
    }

    printf("Reading from 0x%04hX\n", addr);

    buf[0] = OPCODE_READ; // start with opcode_read to inform slave
    buf[1] = addr >> 8;
    buf[2] = addr;
    buf[3] = 0x00;
    SPI_tx(buf); // Transmit

    printf("Data: %02hhX\n\n", buf[3]);
    printf("[+] READ OKAY!\n");
    return 0;
}

int FRAM_write_byte(uint16_t addr, uint8_t data) // use this function to send information to slave device
{
    int err;
    char buf[4] = {0};
    /* A quick check to make sure FRAM_init is executed*/
    err = FRAM_ready();
    if (err)
    {
        return -1;
    }
    /* Inform slave device */
    FRAM_write_enable();
    printf("Writing 0x%02hhX to 0x%04hX\n", data, addr); // prompt user

    buf[0] = OPCODE_WRITE; //Inform slave device
    buf[1] = addr >> 8; // 1 byte left of address
    buf[2] = addr;
    buf[3] = data; // information
    SPI_tx(buf); // if we can't transmit all the info in buf, try to send one cell at a time. i.e. 4 times

    FRAM_write_disable(); // disable the writing mode once transmission is completed
    memset(buf, 0, sizeof(buf));

    buf[0] = OPCODE_READ;
    buf[1] = addr >> 8;
    buf[2] = addr;
    buf[3] = 0x00;
    SPI_tx(buf);
    /*reads back from the FRAM*/
    char rx_buf[]= {0};
    /*Comfirm write sucessfully*/
        if (rx_buf[3] != data) // if the written data mismatches, prompt user
        {
            printf("[-] Bad write, data not match! %hhX %hhXx\n", buf[3], data);
            return 1;
        }
        printf("\n[+] WRITE OKAY!\n");
        return 0;
}

int FRAM_erase_all(void)
{
    int err = 0;
    char buf[4];
    uint16_t addr = 0;

    err = FRAM_ready();
    if (err)
    {
//        printfff("SPI/FRAM is not initialized\n");
        return -1;
    }

    printf("Erasing chip...\n");

        do {
            FRAM_write_enable();
            buf[0] = OPCODE_WRITE;
            buf[1] = addr >> 8;
            buf[2] = addr;
            buf[3] = 0x00;
            SPI_tx(buf); // it may have problem sending all information over to slave device, check this in debug

            addr++;
            FRAM_write_disable();
            printf("\rErasing bytes: %d", addr);
        } while (addr < 0x2000); // rewrite all memory address with 0s
        printf("\n[+] ERASE OKAY!\n");
        return 0;
}


/**
 * main.c
 */
void main(void)
{
        char data;
        int i;
        uint16_t addr;

        SPI_FRAM_init();
        // Enable global interrupt
        __enable_irq();

        // Enable eUSCIA3 interrupt in NVIC module
        NVIC->ISER[0] = 1 << ((EUSCIA1_IRQn) & 31);

        SPI_A1_pin_init();

        FRAM_init(); // Initialize FRAM device
//        FRAM_read_byte(addr);   // Read a byte from the address
//        FRAM_write_byte(addr, data); // write data to the given address
//        FRAM_erase_all(); // erase

	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

	while(1)
	{

}
}
	/* system clock at 3 MHz */


void delayMs(int n) {
	    int i, j;

	    for (j = 0; j < n; j++)
	        for (i = 250; i > 0; i--);      /* delay */
	}

void EUSCIA1_IRQHandler(void)
{
    if (EUSCI_A1->IFG & EUSCI_A_IFG_TXIFG)
    {
        // Transmit characters
        EUSCI_A1->TXBUF = TXData;

        // Disable tx interrupt
        EUSCI_A1->IE &= ~EUSCI_A__TXIE;


        // Wait till a character is received
        while (!(EUSCI_A1->IFG & EUSCI_A_IFG_RXIFG));

        // Move data to a temporary buffer
        RXData = EUSCI_A1->RXBUF;

        // Clear the receive interrupt flag
        EUSCI_A1->IFG &= ~EUSCI_A_IFG_RXIFG;
    }
}

