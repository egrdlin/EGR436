#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Status register
#define FRAM_WPEN       0x80
#define FRAM_BP1        0x08
#define FRAM_BP0        0x04
#define FRAM_WEL        0x02

#define OPCODE_WREN    (0x06)     /* Write Enable Latch */
#define OPCODE_WRDI    (0x04)     /* Reset Write Enable Latch */
#define OPCODE_RDSR    (0x05)     /* Read Status Register */
#define OPCODE_WRSR    (0x01)     /* Write Status Register */
#define OPCODE_READ    (0x03)     /* Read Memory */
#define OPCODE_WRITE   (0x02)     /* Write Memory */
#define OPCODE_RDID    (0x9F)  /* Read Device ID */

#define FRAM_MAX_TRANSACTION 20 //mem size
#define FRAM_MEM_SIZE   0x8000  //Memory size

uint8_t  _nAddressSizeBytes;

// General FRAM Functions
void SPI_FRAM_init(void);
void SPI_A1_pin_init(void);
void Read_ID(uint16_t *manufacturerID, uint16_t *productID);
uint8_t Read_Status_Register(void);
void Set_Status_Register(uint8_t value);
void Write_Enable(bool enable);
void Write8(uint32_t addr, uint8_t value);
void Write(uint32_t addr, const uint8_t *values, size_t count);
uint8_t Read8(uint32_t addr);
void Read(uint32_t addr, uint8_t *values, size_t count);
void SPI_tx(uint8_t TXData);
uint8_t SPI_rx();
bool Test_Address_Size(uint8_t addrSize);
void Set_Address_Size(uint8_t nAddressSize);
int32_t Read_Back(uint32_t addr, int32_t data);
void Write_Address(uint32_t addr);
void Get_Address_Size();

//Poem Functions
void Store_Poem(const char *buffer);
void Get_Poem(int index, char *poem);
void Delete_Poem(int index);
void Clear_FRAM();
void Directory_TX(char *buffer);
void Test_Fill_Poem_Array();
void Get_Size(char *data);
void Delete_Poem(int index);
void Save_Directory();
void Get_Directory();

#endif /* SPI_H_ */
