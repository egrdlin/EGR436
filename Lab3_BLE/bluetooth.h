#ifndef BLUETOOTH_H_
#define BLUETOOTH_H_

#define BUFFER_SIZE 100

void Init_bluetooth();
void ble_data_TX(char *data);
//void ble_Clear_RX_Buffer();
//void Get_RX_Buffer(char *data);
void ble_check_command();
void ble_reset_transmission();
bool ble_comp_command(const char *checkCommand);

#endif /* BLUETOOTH_H_ */
