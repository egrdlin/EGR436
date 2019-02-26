#include <spi.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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

    //Get_Directory();
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
 * Read the status register
 * @return Status register
 */
uint8_t Read_Status_Register(void)
{
    uint8_t RX_Data;

    P1->OUT &= ~BIT6; /* Pull low to select the chip*/

    SPI_tx(OPCODE_RDSR);

    SPI_tx(0);
    RX_Data = SPI_rx();

    P1->OUT |= BIT6;

    return RX_Data;

}

/*
 * Set the status register
 * @return Status register
 */
void Set_Status_Register(uint8_t value)
{
    P1->OUT &= ~BIT6; /* Pull low to select the chip*/

    SPI_tx(OPCODE_RDSR);
    SPI_tx(value);

    P1->OUT |= BIT6;
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
    P1->OUT &= ~BIT6; /* Pull low to select the chip*/

    SPI_tx(OPCODE_WRITE);
//    SPI_tx((uint8_t)(addr >> 24));
//    SPI_tx((uint8_t)(addr >> 16));
    SPI_tx((uint8_t)(addr >> 8));
    SPI_tx((uint8_t)(addr & 0xFF));
    SPI_tx(value);

    P1->OUT |= BIT6;
}

/*
 * Write count bytes starting at a specific FRAM address
 * @param addr The 32-bit address to write to in FRAM memory
 *        values The pointer to an array of 8-bit values to write starting at addr
 *        count The number of bytes to write
 */
void Write(uint32_t addr, const uint8_t *values, size_t count)
{
    P1->OUT &= ~BIT6; /* Pull low to select the chip*/

    SPI_tx(OPCODE_WRITE);
    Write_Address(addr);

    size_t i;
    for (i = 0; i < count; i++)
    {
        SPI_tx(values[i]);
    }

    P1->OUT |= BIT6;
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
//    if (_nAddressSizeBytes>3)
//      SPItransfer((uint8_t)(addr >> 24));
//    if (_nAddressSizeBytes>2)
//      SPItransfer((uint8_t)(addr >> 16));
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

void Write_Address(uint32_t addr)
{
  if (_nAddressSizeBytes>3)
      SPI_tx((uint8_t)(addr >> 24));
  if (_nAddressSizeBytes>2)
      SPI_tx((uint8_t)(addr >> 16));
  SPI_tx((uint8_t)(addr >> 8));
  SPI_tx((uint8_t)(addr & 0xFF));
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

/*
 * Test the FRAM address size to find storage capacity
 * @addrSize Address size to test
 */
bool Test_Address_Size(uint8_t addrSize) {
  Set_Address_Size(addrSize);
  if (Read_Back(4, 0xbeefbead) == 0xbeefbead)
    return true;
  return false;
}

/*
 * Set the address size
 * @nAddressSize Address size to set
 */
void Set_Address_Size(uint8_t nAddressSize)
{
  _nAddressSizeBytes = nAddressSize;
}


int32_t Read_Back(uint32_t addr, int32_t data) {
  int32_t check = !data;
  int32_t wrapCheck, backup;

  // Read from test address
  Read(addr, (uint8_t*)&backup, sizeof(int32_t));

  // Write to test address
  Write_Enable(true);
  Write(addr, (uint8_t*)&data, sizeof(int32_t));
  Write_Enable(false);

  // Read from test address
  Read(addr, (uint8_t*)&check, sizeof(int32_t));

  // Read from starting address (0)
  Read(0, (uint8_t*)&wrapCheck, sizeof(int32_t));

  // Write original value to test address
  Write_Enable(true);
  Write(addr, (uint8_t*)&backup, sizeof(int32_t));
  Write_Enable(false);

  // Check for warparound, address 0 will work anyway
  if (wrapCheck==check)
    check = 0;
  return check;
}

void Get_Address_Size(){
    uint8_t addrSizeInBytes = 2; //Default to address size of two bytes
    uint32_t memSize;

    if (Test_Address_Size(2))
      addrSizeInBytes = 2;
    else if (Test_Address_Size(3))
      addrSizeInBytes = 3;
    else if (Test_Address_Size(4))
      addrSizeInBytes = 4;
    else {
      fprintf(stderr,"SPI FRAM can not be read/written with any address size\r\n");
      while (1);
    }

    memSize = 0;
    while (Read_Back(memSize, memSize) == memSize) {
      memSize += 256;
      //Serial.print("Block: #"); Serial.println(memSize/256);
    }

    fprintf(stderr, "\nSPI FRAM address size is %i bytes.", addrSizeInBytes);
    fprintf(stderr,"\nSPI FRAM capacity appears to be %i bytes.", memSize);
}



/************************** Poem Functions **************************************/

// Create a struct variable to keep track of poems in memory
struct poem{
    char name[28];
    int start_index;
    int length;
};

struct poem poem_array[20]; // Array to store 20 poem structs

int fram_index = 0; // Address of FRAM memory last wrote

int poem_index = 0; // Index to keep track of how many poems saved

/*
 * Store a poem into FRAM memory
 * @param buffer The pointer to the RX buffer containing the poem title, \n, then the rest of the poem
 *        length The number of elements in the RX buffer
 */
void Store_Poem(const char *buffer){

    int length = strlen(buffer);

    // Set start index
    poem_array[poem_index].start_index = fram_index;

    // Set end index
    poem_array[poem_index].length = length;

    // Get and store name of poem
    int i=0;
    int read_index = 0;
    while(buffer[i] != '\n' && i<length){

        // Store name in poem array
        poem_array[poem_index].name[read_index++] = buffer[i];

        // Write name to FRAM
        Write_Enable(true);
        Write8(fram_index++, buffer[i]);
        Write_Enable(false);

        i++;
    }

    poem_array[poem_index].name[read_index] = '\0';

    // Get and store rest of poem
    for( ; i<length; i++){
        Write_Enable(true);
        Write8(fram_index++, buffer[i]);
        Write_Enable(false);
    }
    poem_index++;
    Save_Directory();
}

/*
 * Retrieve a poem from FRAM memory
 * @param index Index of poem to get
 */
void Get_Poem(int index, char *poem){
    char test[1000] = {0}; // Test variable to see if data read correctly
    // Decrement index (poem #1 is at index 0, etc.)
    index--;

    if(index < poem_index){
    int start = poem_array[index].start_index;
    int length = poem_array[index].length;



    int i,index_2=0;
    char temp;
    for(i=start; i<start+length; i++){
        temp = Read8(i);

        poem[index_2++] = temp;
        // transmit over uart
    }
    poem[index_2] = '\0';

    }else{
        strcpy(poem, "Invalid poem index.");
    }
    strcpy(test,poem);
}

/*
 * Delete a poem from FRAM memory
 * @param index Index of poem to delete
 */
void Delete_Poem(int index){

    // Decrement index (poem #1 is at index 0, etc.)
    index--;

    if(index < poem_index){

        int start = poem_array[index].start_index;
        int length = poem_array[index].length;
        //int difference = (end - start);
        int i;
        char rewrite_value = 0;

        if(index == poem_index-1){

            for(i=start; i<start+length;i++){
                Write8(i,rewrite_value);
            }
        }else{
            // Move all poems behind deleted one up in poem array and adjust start values
            for(i=index;i<poem_index;i++){
                if(i<poem_index - 1){

                    strcpy(poem_array[i].name,poem_array[i+1].name);
                    poem_array[i].start_index = poem_array[i+1].start_index;
                    poem_array[i].length = poem_array[i+1].length;
                    poem_array[i].start_index -= length;
                }
            }

            //uint8_t test_end = poem_array[poem_index-1].end_index;
            for(i=start;i<poem_array[poem_index-1].start_index+poem_array[poem_index-1].length;i++){
                uint8_t temp;
                temp = Read8(i+length);
                Write_Enable(true);
                Write8(i,temp);
                Write_Enable(false);
            }

            // Fill in not used characters with blank spaces. Start at end of last poem and go until length of deleted poem
            for(i=poem_array[poem_index-1].start_index+poem_array[poem_index-1].length; i<poem_array[poem_index-1].start_index+poem_array[poem_index-1].length + length;i++){
                Write_Enable(true);
                Write8(i,rewrite_value);
                Write_Enable(false);
            }
        }

        fram_index -= length;
        poem_index--;
    }
    Save_Directory();
}

/*
 * Clears all FRAM memory spaces to 0s
 */
void Clear_FRAM(){

    fram_index = 0;
}

/*
 * Transmit items for directory. Format is in poem1_name.txt,poem1_length,poem2_name.txt,poem2_length etc..
 */
void Directory_TX(char *buffer){
    int i;
    buffer[0] = '\0';
    for(i=0; i<poem_index; i++){
        strcat(buffer, poem_array[i].name);
        strcat(buffer, ",");
        char int_string[10];
        sprintf(int_string,"%i",poem_array[i].length);
        strcat(buffer, int_string);
        if(i!= poem_index-1)
            strcat(buffer, ",");
    }

}

void Save_Directory(){
    char dir_buffer[299];
    int i;
    dir_buffer [0] = '\0';
    // Iterate through poem array structs
    for(i=0; i<poem_index; i++){
        strcat(dir_buffer, poem_array[i].name);
        strcat(dir_buffer, ",");
        char int_string[10];
        sprintf(int_string,"%i",poem_array[i].start_index);
        strcat(dir_buffer, int_string);
        strcat(dir_buffer, ",");
        sprintf(int_string,"%i",poem_array[i].length);
        strcat(dir_buffer, int_string);
        if(i!= poem_index-1)
            strcat(dir_buffer, ",");
    }

    int length = strlen(dir_buffer);
    // Iterate through buffer
    for(i=0;i<length;i++){
        Write_Enable(true);
        Write8(i,dir_buffer[i]);
        Write_Enable(false);
    }

    Get_Directory();
}

/*
 * Get the directory from FRAM and fill poem array, set poem index, set FRAM index
 */
void Get_Directory(){
    char dir_buffer[300];
    dir_buffer [0] = '\0';

    char temp;
    int i = 0;
    // Read directory elements stored in memory
    do{
        temp=Read8(i);
        dir_buffer[i] = temp;
        i++;
    }while(temp != '\0');

    // Get info and put in structs
    char *token;
    const char s[2] = ",";
    poem_index=0;
    if(strlen(dir_buffer) != 0){
        token = strtok(dir_buffer, s);

        while(token != NULL){
            strcpy(poem_array[poem_index].name,token);
            token = strtok(NULL, s);
            poem_array[poem_index].start_index = atol(token);
            token = strtok(NULL, s);
            poem_array[poem_index].length = atol(token);
            token = strtok(NULL, s);
            poem_index++;
        }

        fram_index = 300 + poem_array[poem_index-1].start_index + poem_array[poem_index-1].length;
    }else{
        fram_index = 300;
    }

    char test[300];
    strcpy(test, dir_buffer);
    int k;
}

void Test_Fill_Poem_Array(){

    sprintf(poem_array[0].name,"%s", "file1.txt");
    poem_array[0].start_index = 0;
    poem_array[0].length = 10;

    sprintf(poem_array[1].name,"%s", "file2.txt");
    poem_array[1].start_index = 11;
    poem_array[1].length = 28;

    poem_index = 2;
}

void Get_Size(char *data){
    int length=0, i;
    for(i=0;i<poem_index;i++){
        length += poem_array[i].length;
    }
    if(poem_index == 0){
        sprintf(data,"%i%c",0,'\0');
    }else{
        sprintf(data,"%i%c",length,'\0');
    }

}

/****************************** Project Functions *******************************/
void write_date(char *date){

    // Add comma before if not first entry
    if(fram_index != 0){
        Write_Enable(true);
        Write8(fram_index, ',');
        Write_Enable(false);
        fram_index++;
    }

    // Add rest of entry
    int i;
    for(i=fram_index;i<strlen(date);i++){
        Write_Enable(true);
        Write8(fram_index, date[i-fram_index]);
        Write_Enable(false);
        fram_index++;
    }
}

int get_dates(char *date){

    if(fram_index < 500){
        int i;
        for(i=0;i<fram_index;i++){
            date[i] = Read8(i);
        }
    }

    return fram_index;
}
