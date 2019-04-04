#include "msp.h"
#include "spi.h"
#include "timer.h"
#include "adc.h"
#include <stdbool.h>

volatile uint16_t inCount;  // Number of bees that entered
volatile uint16_t outCount; // Number of bees that left

// Array to keep track of Memory Control Registers to map to each sensor
uint32_t sensor_memory_ctrl_reg[8];

void Init_ADC(){

    // GPIO Setup
    //P1->OUT &= ~BIT0;                       // Clear LED to start
    //P1->DIR |= BIT0;                        // Set P/;1.0/LED to output

    // Configure P5.4 for ADC
//    P5->SEL1 |= BIT4;
//    P5->SEL0 |= BIT4;


    /*
     * Sensor Configuration
     *
     * Sensor 0 | P4.5 (A8)
     * Sensor 1 | P4.4 (A9)
     * Sensor 2 | P4.3 (A10)
     * Sensor 3 | P4.2 (A11)
     * Sensor 4 | P4.1 (A12)
     * Sensor 5 | P4.0 (A13)
     * Sensor 6 | P6.1 (A14)
     * Sensor 7 | P6.0 (A15)
     *
     * IN   Sensor 0    Sensor 2     Sensor 4    Sensor 6
     * OUT  Sensor 1    Sensor 3     Sensor 5    Sensor 7
     */

    // Configure Sensors 0-5 (P4.5, P4.4, P4.3, P4.2, P4.1, P4.0) for ADC
//    P4->SEL1 |= BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0;
//    P4->SEL0 |= BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0;

    P4->SEL1 |= BIT5 | BIT4 | BIT3 | BIT2;
    P4->SEL0 |= BIT5 | BIT4 | BIT3 | BIT2;

    // Configure Sensors 6-7 (P6.1, P6.0) for ADC
//    P6->SEL1 |= BIT1 | BIT0;
//    P6->SEL0 |= BIT1 | BIT0;

    /*
     * Current and Voltage Measurement Configuration
     *
     * I_BAT_NEG | P5.0 (
     * I_BAT_POS | P5.1
     * I_OUT_NEG | P5.2
     * I_OUT_POS | P5.3
     * I_SOLAR_OUT_NEG | P5.4
     * I_SOLAR_OUT_POS | P5.5
     * V_SOLAR_IN | P8.3
     * V_SOLAR_OUT | P8.5
     * V_SOLAR_BATTERY | P8.6
     */

    // Configure Current and Voltage Measurements for ADC
//    P5->SEL1 |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5;
//    P5->SEL0 |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5;
//    P8->SEL1 |= BIT3 | BIT5 | BIT6;
//    P8->SEL0 |= BIT3 | BIT5 | BIT6;


    // Enable ADC interrupt in NVIC module
    NVIC->ISER[0] = 1 << ((ADC14_IRQn) & 31);

    // Sampling time, S&H=16, ADC14 on
    ADC14->CTL0 = ADC14_CTL0_SHT0_2 | ADC14_CTL0_SHP | ADC14_CTL0_ON;
    ADC14->CTL1 = ADC14_CTL1_RES_2;         // Use sampling timer, 12-bit conversion results


    // Set initial sensor to tie to ADC
    ADC14->MCTL[0] = sensor_memory_ctrl_reg[0];

    // Map ADC Memory Control Registers to Sensors 0-7
//    ADC14->MCTL[0] |= ADC14_MCTLN_INCH_8;  // Sensor 0
//    ADC14->MCTL[1] |= ADC14_MCTLN_INCH_9;  // Sensor 1
//    ADC14->MCTL[2] |= ADC14_MCTLN_INCH_10; // Sensor 2
//    ADC14->MCTL[3] |= ADC14_MCTLN_INCH_11; // Sensor 3
//    ADC14->MCTL[4] |= ADC14_MCTLN_INCH_12; // Sensor 4
//    ADC14->MCTL[5] |= ADC14_MCTLN_INCH_13; // Sensor 5
//    ADC14->MCTL[6] |= ADC14_MCTLN_INCH_14; // Sensor 6
//    ADC14->MCTL[7] |= ADC14_MCTLN_INCH_15; // Sensor 7

    // Map ADC Memory Control Registers to Current and Voltage Pins
//    ADC14->MCTL[8]  |= ADC14_MCTLN_INCH_5;  // I_BAT_NEG
//    ADC14->MCTL[9]  |= ADC14_MCTLN_INCH_4;  // I_BAT_POS
//    ADC14->MCTL[10] |= ADC14_MCTLN_INCH_3;  // I_OUT_NEG
//    ADC14->MCTL[11] |= ADC14_MCTLN_INCH_2;  // I_OUT_POS
//    ADC14->MCTL[12] |= ADC14_MCTLN_INCH_1;  // I_SOLAR_OUT_NEG
//    ADC14->MCTL[13] |= ADC14_MCTLN_INCH_0;  // I_SOLAR_OUT_POS
//    ADC14->MCTL[14] |= ADC14_MCTLN_INCH_22; // V_SOLAR_IN
//    ADC14->MCTL[15] |= ADC14_MCTLN_INCH_20; // V_SOLAR_OUT
//    ADC14->MCTL[16] |= ADC14_MCTLN_INCH_19; // V_SOLAR_BATTERY


    // Enable ADC conversion complete interrupt for memory register 0
    ADC14->IER0 |= ADC14_IER0_IE0;

    // Enable ADC conversion complete interrupt for sensors 0-3
    //ADC14->IER0 |= ADC14_IER0_IE0 | ADC14_IER0_IE1 | ADC14_IER0_IE2 | ADC14_IER0_IE3;

//    // Enable ADC conversion complete interrupt for sensors 0-7
//    ADC14->IER0 |= ADC14_IER0_IE0 | ADC14_IER0_IE1 | ADC14_IER0_IE2 | ADC14_IER0_IE3
//            | ADC14_IER0_IE4 | ADC14_IER0_IE5 | ADC14_IER0_IE6 | ADC14_IER0_IE7;

    SCB->SCR &= ~SCB_SCR_SLEEPONEXIT_Msk;   // Wake up on exit from ISR

    Reset_Counts();

    // Map memory control registers to sensors
    sensor_memory_ctrl_reg[0] = ADC14_MCTLN_INCH_8;
    sensor_memory_ctrl_reg[1] = ADC14_MCTLN_INCH_9;
    sensor_memory_ctrl_reg[2] = ADC14_MCTLN_INCH_10;
    sensor_memory_ctrl_reg[3] = ADC14_MCTLN_INCH_11;
//    sensor_memory_ctrl_reg[4] = ADC14_MCTLN_INCH_12;
//    sensor_memory_ctrl_reg[5] = ADC14_MCTLN_INCH_13;
//    sensor_memory_ctrl_reg[6] = ADC14_MCTLN_INCH_14;
//    sensor_memory_ctrl_reg[7] = ADC14_MCTLN_INCH_15;
}

int sampleIndex = 0;

/*
 * Start the ADC sampling/conversion
 */
void Sample_ADC(){

//    sampleIndex = (sampleIndex + 1) % 4; // Increment measurement to be sampled, wrap around at 8
//    ADC14->MCTL[0] = sensor_memory_ctrl_reg[sampleIndex]; // Set measurement to be sampled

    // ADC enable and start conversion
    ADC14->CTL0 |= ADC14_CTL0_ENC | ADC14_CTL0_SC;
}

uint16_t Get_Out_Count(){
    return outCount;
}

uint16_t Get_In_Count(){
    return inCount;
}

void Reset_Counts(){
    outCount = 0;
    inCount = 0;
}

//const int NUM_SENSORS = 8;
const int TOLERANCE = 0x470;

int isCovered = 0;

// ADC14 interrupt service routine
void ADC14_IRQHandler(void) {

    /****** Sensor test ******/
    // Also in rtc.c
    // Check sensor 0
    if (ADC14->MEM[0] < TOLERANCE){  // Bee in between LED and sensor

      if(isCovered == 0){
          inCount++;
          //fprintf(stderr,"In: %i\n",inCount);
      }

      P1->OUT |= BIT0;                      // LED on
      isCovered = 1;

    }else{
      P1->OUT &= ~BIT0;                     // LED off
      isCovered = 0;
    }
    /*************************/
//
//    // Array to keep track of sensor interrupt pending flags for each sensor
//    uint32_t sensor_interrupt_pending_flag[8];
//    sensor_interrupt_pending_flag[0] = ADC14_IFGR0_IFG0;
//    sensor_interrupt_pending_flag[1] = ADC14_IFGR0_IFG1;
//    sensor_interrupt_pending_flag[2] = ADC14_IFGR0_IFG2;
//    sensor_interrupt_pending_flag[3] = ADC14_IFGR0_IFG3;
//    sensor_interrupt_pending_flag[4] = ADC14_IFGR0_IFG4;
//    sensor_interrupt_pending_flag[5] = ADC14_IFGR0_IFG5;
//    sensor_interrupt_pending_flag[6] = ADC14_IFGR0_IFG6;
//    sensor_interrupt_pending_flag[7] = ADC14_IFGR0_IFG7;
//
//    // Arrays to keep track of in/out sensor states (covered or clear)
//    bool outReading[4], inReading[4];
//    bool lastInReading[4] = {false};
//    bool lastOutReading[4] = {false};
//
//
//    // Arrays to keep track of in/out sensor trigger times
//    uint32_t inReadingTime[4], inReadingTimeHigh[4], outReadingTime[4], outReadingTimeHigh[4];
//    uint32_t lastInReadingTime[4] = {0};
//    uint32_t lastOutReadingTime[4] = {0};
//
//    uint32_t currentTime; // Current system time
//    const int debounce = 120; // Debounce time in ms
//
//    // Cycle through ADC for each IR sensor
//    int i,j=0;
//    for(i=0; i<8; i++){
//
//        // Check for interrupt pending at sensor i
//        if(ADC14->IFGR0 & sensor_interrupt_pending_flag[i]){
//
//            // Read register
//            uint32_t data = ADC14->MEM[i];
//
//            // Check if even sensor to determine if in or out measurement
//            // In reading
//            if(i%2){
//
//                // Determine if covered or clear
//                inReading[j] = data >= TOLERANCE;
//
//                // Check for change in sensor status
//                if(inReading[j] != lastInReading[j]){
//
//                    // Check for debounce
//                    if(currentTime - lastInReadingTime[j] > debounce){
//
//                        // Check if bee left sensor
//                        if(inReading[j] == false){
//
//                            // Find how long bee was on sensor
//                            lastInReadingTime[j] = currentTime;
//                            inReadingTimeHigh[j] = currentTime - inReadingTime[j];
//
//                            // Check that movement was recent
//                            if(currentTime - lastOutReadingTime[i] < 150){
//                                if(inReadingTimeHigh[j] < 350 || outReadingTimeHigh[j] < 350){
//
//                                    // Increment out count
//                                    outCount++;
//                                }
//                            }
//
//                        // Bee on sensor
//                        }else{
//
//                        }
//
//                    }
//                    inReadingTime[j] = currentTime;
//                    lastInReading[j] = inReading[j];
//
//                }
//
//            // Out reading
//            }else{
//
//                // Determine if covered or clear
//                outReading[j] = data >= TOLERANCE;
//
//                // Check for change in sensor status
//                if(outReading[j] != lastOutReading[j]){
//
//                    // Check for debounce
//                    if(currentTime - lastOutReadingTime[j] > debounce){
//
//                        // Check if bee left sensor
//                        if(outReading[j] == false){
//
//                            // Find how long bee was on sensor
//                            lastOutReadingTime[j] = currentTime;
//                            outReadingTimeHigh[j] = currentTime - outReadingTime[j];
//
//                            // Check that movement was recent
//                            if(currentTime - lastInReadingTime[i] < 150){
//                                if(inReadingTimeHigh[j] < 350 || outReadingTimeHigh[j] < 350){
//
//                                    // Increment in count
//                                    inCount++;
//                                }
//                            }
//
//                        // Bee on sensor
//                        }else{
//
//                        }
//
//                    }
//                    outReadingTime[j] = currentTime;
//                    lastOutReading[j] = outReading[j];
//
//                }
//
//                j++;
//            }
//        }
//    }
}
