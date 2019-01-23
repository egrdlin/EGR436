#ifndef _UART_H
#define _UART_H

#include <stdint.h>

//void UART2_init();
int get_blink_rate();
void UART_init();
void update_blink_rate();
void clear_buffer();
void tx_baud_rate();

#endif
