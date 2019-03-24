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
uint8_t SPI_rx();
void Write_Enable(bool enable);
void Write8(uint32_t addr, char value);
void Write(uint32_t addr, const uint8_t *values, size_t count);
char Read8(uint32_t addr);
void Read(uint32_t addr, uint8_t *values, size_t count);
void SPI_tx(uint8_t TXData);
void Write_Address(uint32_t addr);

// Custom Functions
void Get_Fram_Index();
void Set_Fram_Index();
void Store_Time();
int Get_Time(int index, char *buffer);
void Clear_FRAM();
void Get_Fram(char *buffer);

#endif /* SPI_H_ */
