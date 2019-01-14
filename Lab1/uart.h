#ifndef _UART_H
#define _UART_H

#include <stdint.h>

//void UART2_init();
int get_blink_rate();
void UART_init();
int update_blink_rate(uint16_t input_char);
void clear_buffer_at_index();
void clear_buffer_full();

#endif
