#ifndef _UART_H
#define _UART_H

#include <stdint.h>

void UART_init();
void check_command();
void reset_transmission();
void clear_buffer();
void String_TX(char *data);
void data_TX(char *data);

#endif
