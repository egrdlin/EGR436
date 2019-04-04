#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

void Reset_Counts();
void Init_ADC();
void Sample_ADC();
uint16_t Get_Out_Count();
uint16_t Get_In_Count();

#endif
