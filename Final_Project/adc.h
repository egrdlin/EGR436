#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>
#include <stdbool.h>

void Reset_Counts();
void Init_ADC();
void Sample_ADC();
uint16_t Get_Out_Count();
uint16_t Get_In_Count();
bool Get_Recording_Status();
void Stop_Recording();
void Start_Recording();
void BLE_Stop_Recording();
void BLE_Start_Recording();

//uint32_t Get_iBatNeg();
//uint32_t Get_iBatPos();
//uint32_t Get_iOutNeg();
//uint32_t Get_iOutPos();
uint32_t Get_iOut();
//uint32_t Get_iSolarOutPos();
//uint32_t Get_vSolarIn();
uint32_t Get_vSolarOut();
//uint32_t Get_vSolarBat();

#endif
