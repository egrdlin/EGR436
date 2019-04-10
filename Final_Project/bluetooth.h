#ifndef BLUETOOTH_H_
#define BLUETOOTH_H_

#define BUFFER_SIZE 100
#include <stdbool.h>

void Init_Bluetooth();
void ble_data_TX(char *data);
//void ble_Clear_RX_Buffer();
//void Get_RX_Buffer(char *data);
void ble_check_command();
void ble_reset_transmission();
bool ble_comp_command(const char *checkCommand);
bool verify_set_date(char *entry);

void ble_sleep();
void ble_wake();
void ble_name();

#endif /* BLUETOOTH_H_ */
