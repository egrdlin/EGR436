#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "spi.h"
#include "msp.h"

/************************** General FRAM Functions **************************************/

void SPI_FRAM_init(void)
{
    SPI_A1_pin_init();

    EUSCI_A1->CTLW0 |= EUSCI_A_CTLW0_SWRST; // Put eUSCI in reset
    EUSCI_A1->CTLW0 = EUSCI_A_CTLW0_SWRST; /* Disable UCA1 during configuration*/
    /* SPI through EUSCI_A1:
     * clock phase/polarity: 11,    MSB first,  8-bit,  master mode,
     * 4-pin SPI,   STE Low active,     sychronous mode,    use SMCLK
     * as clock source, STE for slave enable*/
    //EUSCI_A1->CTLW0 = 0xEDC3;

    /* SPI through EUSCI_A1:
     * SPI mode 0, MSB first,  8-bit,  master mode,
     * 3-pin SPI, sychronous mode, SMCLK */
    EUSCI_A1->CTLW0 = 0x69C1;

    EUSCI_A1->BRW = 1; /* 3MHz / 1 = 3MHz*/

    EUSCI_A1->CTLW0 &= ~ EUSCI_A_CTLW0_SWRST; /* Enable UCA1 after configuration*/


    EUSCI_A1->IFG &= ~EUSCI_A_IFG_RXIFG;
    //EUSCI_A1->IE |= EUSCI_A_IE_RXIE;        // Enable USCI_A0 RX interrupt

    __enable_irq();

    // Enable eUSCIA1 interrupt in NVIC module
    NVIC->ISER[0] = 1 << ((EUSCIA1_IRQn) & 17); /* enable IRQ 17 => EUSCIA1*/

    //Get_Fram_Index();
    Clear_FRAM();

}

/*
 *  P2.5 = UCA1STE - slave transmit enable
 *  P2.3 = UCA1CLK - clock
 *  P2.6 = UCA1SOMI - slave out master in
 *  P2.4 = UCA1SIMO - slave in master out
 * */
void SPI_A1_pin_init(void)
{
    PMAP->KEYID = 0x2D52; /* Unlock PMAP*/
    P2MAP->PMAP_REGISTER[1] = 0x0800; /* Map P2.3 to PM_UCA1CLK*/
    //P2MAP->PMAP_REGISTER[2] = 0x070A; /* Map P2.4 to PM_UCA1_SIMO, Map P2.5 to PM_UCA1STE */
    P2MAP->PMAP_REGISTER[2] = 0x000A; /* Map P2.4 to PM_UCA1_SIMO */
    P2MAP->PMAP_REGISTER[3] = PMAP_UCA1SOMI; /* Map P2.6 to PM_UCA1SOMI*/

    //P2->SEL0 |= 0x78;               /* set alternate function to pin map */
    //P2->SEL1 &= ~0x78;              /* for P2.3, P2.4, P2.5, P2.6 */

    P2->SEL0 |= 0x58;               /* set alternate function to pin map */
    P2->SEL1 &= ~0x58;              /* for P2.3, P2.4, P2.6 */

    PMAP->CTL = 0;                  /* lock PMAP */
    PMAP->KEYID = 0;

    P1->DIR |= BIT6; /* use P1.6 to manually select chip*/
    P1->OUT |= BIT6; /* pull low to start, pull high to end*/

}

/*
 * Reads the Manufacturer ID and the Product ID from the IC
 * @param manufacturerID
 *          The 8-bit manufacturer ID (Fujitsu = 0x047F)
 *        productID
 *          The memory density (bytes 15..8) and proprietary
            Product ID fields (bytes 7..0) (Should = 0x0302)
 */
void Read_ID(uint16_t *manufacturerID, uint16_t *productID)
{
    uint8_t RX_Buf[4] = {0};

    P1->OUT &= ~BIT6; /* Pull low to select the chip*/

    SPI_tx(OPCODE_RDID);

    SPI_tx(0);
    RX_Buf[0] = SPI_rx();

    SPI_tx(0);
    RX_Buf[1] = SPI_rx();

    SPI_tx(0);
    RX_Buf[2] = SPI_rx();

    SPI_tx(0);
    RX_Buf[3] = SPI_rx();

    P1->OUT |= BIT6;

    *manufacturerID = (RX_Buf[0] << 8 ) | (RX_Buf[1]);
    *productID = (RX_Buf[2] << 8 ) | (RX_Buf[3]);

}

/*
 * Receive SPI data from RX buffer
 * @return RX data
 */
uint8_t SPI_rx(){
    uint8_t data;
    while (EUSCI_A1->STATW & EUSCI_A_STATW_BUSY && !(EUSCI_A1->IFG & EUSCI_A_IFG_RXIFG));
    data = EUSCI_A1->RXBUF;
    EUSCI_A1->IFG &= ~EUSCI_A_IFG_RXIFG;
    return data;
}

/*
 * Enables or disables writing
 * @param enable
 *          True enables writes, false disables writes
 */
void Write_Enable(bool enable)
{
    P1->OUT &= ~BIT6; /* Pull low to select the chip*/

    if(enable)
        SPI_tx(OPCODE_WREN); // Set write enable
    else
        SPI_tx(OPCODE_WRDI); // Reset write enable

    P1->OUT |= BIT6;

}

/*
 * Write a byte at a specific FRAM address
 * @param addr The 32-bit address to write to in FRAM memory
 *        value The 8-bit value to write to at addr
 */
void Write8(uint32_t addr, char value)
{
    Write_Enable(true);
    P1->OUT &= ~BIT6; /* Pull low to select the chip*/

    SPI_tx(OPCODE_WRITE);
    SPI_tx((uint8_t)(addr >> 8));
    SPI_tx((uint8_t)(addr & 0xFF));
    SPI_tx(value);

    P1->OUT |= BIT6;
    Write_Enable(false);
}

/*
 * Write count bytes starting at a specific FRAM address
 * @param addr The 32-bit address to write to in FRAM memory
 *        values The pointer to an array of 8-bit values to write starting at addr
 *        count The number of bytes to write
 */
void Write(uint32_t addr, const uint8_t *values, size_t count)
{
    Write_Enable(true);
    P1->OUT &= ~BIT6; /* Pull low to select the chip*/

    SPI_tx(OPCODE_WRITE);
    Write_Address(addr);

    size_t i;
    for (i = 0; i < count; i++)
    {
        SPI_tx(values[i]);
    }

    P1->OUT |= BIT6;
    Write_Enable(false);
}

/*
 * Reads an 8-bit value from the specified FRAM address
 * @param addr The 32-bit address to read from in FRAM memory
 * @return The 8-bit value retrieved from addr
 */
char Read8(uint32_t addr)
{
    uint8_t RX_Data;

    P1->OUT &= ~BIT6; /* Pull low to select the chip*/

    SPI_tx(OPCODE_READ);
    SPI_tx((uint8_t)(addr >> 8));
    SPI_tx((uint8_t)(addr & 0xFF));

    SPI_tx(0);

    RX_Data = SPI_rx();

    P1->OUT |= BIT6;

    return RX_Data;
}

/*
 * Read count bytes starting at the specific FRAM address
 * @param addr The 32-bit address to read from in FRAM memory
 *        values The pointer to an array of 8-bit values to read starting at addr
 *        count The number of bytes to read
 */
void Read(uint32_t addr, uint8_t *values, size_t count)
{
    P1->OUT &= ~BIT6; /* Pull low to select the chip*/

    SPI_tx(OPCODE_READ);
    Write_Address(addr);

    size_t i;
    for (i = 0; i < count; i++)
    {
        SPI_tx(0);
        values[i] = SPI_rx();
    }

    P1->OUT |= BIT6;
}

/*
 * Transmit a byte of data over SPI
 * @param TXData The byte of data to transmit
 */
void SPI_tx(uint8_t TXData)
{
    while (!(EUSCI_A1->IFG & EUSCI_A_IFG_TXIFG)); // Wait for TX buffer empty

    EUSCI_A1->TXBUF = TXData;           // Transmit characters TXData

    while(EUSCI_A1->STATW & 1 && !(EUSCI_A1->IFG & EUSCI_A_IFG_TXIFG)); /* Wait for transmit done */

}

void Write_Address(uint32_t addr)
{
  if (_nAddressSizeBytes>3)
      SPI_tx((uint8_t)(addr >> 24));
  if (_nAddressSizeBytes>2)
      SPI_tx((uint8_t)(addr >> 16));
  SPI_tx((uint8_t)(addr >> 8));
  SPI_tx((uint8_t)(addr & 0xFF));
}

/************************** Custom Functions **************************************/
int fram_index; // Address of FRAM memory last wrote
const int index_offset = 2; // Number of bytes index is offset (due to index being written into memory)

/*
 * Get the fram index stored in the first 2 bytes of memory
 */
void Get_Fram_Index(){
    fram_index = (Read8(0) << 8) | Read8(1);
}

/*
 * Set the fram index stored in the first 2 bytes of memory
 */
void Set_Fram_Index(){
    Write8(0,fram_index >> 8);
    Write8(1,fram_index & 0xFF);
}

/*
 * Store the time of a bee entry/exit in memory
 * Time is stored in four 16-bit registers. An extra byte of data is stored to indicate entry or exit.
 * Format:  Byte 1: 1 (entry) or 0 (exit)
 *          Byte 2-3: Year
 *          Byte 4: Month
 *          Byte 5: Day
 *          Byte 6: Day of Week
 *          Byte 7: Hour
 *          Byte 8: Minute
 *          Byte 9: Seconds
 * @param Variables for each of the registers
 */
void Store_Time(){

    uint8_t month = RTCMON;
    uint8_t day =  RTCDAY;
    uint16_t year = RTCYEAR;
    uint8_t hour = RTCHOUR;
    uint8_t minute = RTCMIN;
    uint8_t second = RTCSEC;

    Write8(fram_index++, RTCMON); // month
    Write8(fram_index++, RTCDAY); // byte day
    Write8(fram_index++, RTCYEAR >> 8); // byte year 1
    Write8(fram_index++, RTCYEAR & 0xFF); // byte year 2
    Write8(fram_index++, RTCHOUR); // byte hour
    Write8(fram_index++, RTCMIN); // byte minute
    Write8(fram_index++, RTCSEC); // byte second

    Set_Fram_Index();

}

int data_size = 7; //Bytes of data stored for each time entry

/*
 * Get the time for a specific index stored in FRAM
 * @param index of entry to read (first entry is 1)
 *        buffer array to store data retrieved (9 bytes of data)
 * @return 1 if data successfully retrieved, 0 if index is out of bounds
 */
int Get_Time(int index, char *buffer){
    // Check if index is out of bounds
    if(index <= ((fram_index-index_offset) + 1) / data_size){

        int i;
        char temp;
        int start = index_offset + (data_size * (index-1));
        int buffer_index = 0;
        int iter = 0;

        // Get data_size bytes of data stored at index
        for(i = start; i < start + data_size; i++){
            switch(iter++){
                case 0: // Month
                    temp = Read8(i);
                    buffer[buffer_index++] = (temp >> 4) + '0';
                    buffer[buffer_index++] = (temp & 0xF) + '0';
                    buffer[buffer_index++] = '/';
                break;

                case 1: // Day
                    temp = Read8(i);
                    buffer[buffer_index++] = (temp >> 4) + '0';
                    buffer[buffer_index++] = (temp & 0xF) + '0';
                    buffer[buffer_index++] = '/';
                break;

                case 2: // Year
                    temp = Read8(i);
                    buffer[buffer_index++] = (temp >> 4) + '0'; // First Digit (2)
                    buffer[buffer_index++] = (temp & 0xF) + '0'; // Second Digit (0)
                break;

                case 3: // Year
                    temp = Read8(i);
                    buffer[buffer_index++] = (temp >> 4) + '0'; // Third Digit (1)
                    buffer[buffer_index++] = (temp & 0xF) + '0'; // Fourth Digit (9)
                    buffer[buffer_index++] = ' ';
                break;

                case 4: // Hour
                    temp = Read8(i);
                    buffer[buffer_index++] = (temp >> 4) + '0';
                    buffer[buffer_index++] = (temp & 0xF) + '0';
                    buffer[buffer_index++] = ':';
                break;

                case 5: // Minute
                    temp = Read8(i);
                    buffer[buffer_index++] = (temp >> 4) + '0';
                    buffer[buffer_index++] = (temp & 0xF) + '0';
                    buffer[buffer_index++] = ':';
                break;

                case 6: // Second
                    temp = Read8(i);
                    buffer[buffer_index++] = (temp >> 4) + '0';
                    buffer[buffer_index++] = (temp & 0xF) + '0';
                    buffer[buffer_index++] = ',';
                break;
            }
        }

        return 1;

    }else{

        return 0;
    }
}

void Get_Fram(char *buffer){
    int i;
    char c;
    for(i=0;i<fram_index;i++){
        c = Read8(i);
        buffer[i] = c;
    }
}

/*
 * Clears all FRAM memory spaces to 0s
 */
void Clear_FRAM(){

    Get_Fram_Index();

   // Wipe memory
    int i;
   for(i=0; i<fram_index;i++){
       Write8(i,'0');
   }

   // Reset fram index
   fram_index = index_offset;
   Set_Fram_Index();
   //Get_Fram_Index();
}
