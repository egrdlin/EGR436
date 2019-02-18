#ifndef _UART_H
#define _UART_H

#include <stdint.h>
#include <stdbool.h>

void UART_init();
void uart_check_command();
void uart_reset_transmission();
void uart_clear_buffer();
//void uart_String_TX(char *data);
void uart_data_TX(char *data);
bool uart_comp_command(const char *checkCommand);

#endif
