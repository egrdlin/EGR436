#include "msp.h"
#include "spi.h"
#include "timer.h"
#include "adc.h"
#include <stdbool.h>

volatile uint16_t inCount;  // Number of bees that entered
volatile uint16_t outCount; // Number of bees that left

// Array to keep track of Memory Control Registers to map to each sensor
uint32_t sensor_memory_ctrl_reg[17];

volatile int sampleIndex;

void Init_ADC(){

    /***************** IR Sensors ************************/

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
    P4->SEL1 |= BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0;
    P4->SEL0 |= BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0;

//    P4->SEL1 |= BIT5 | BIT4 | BIT3 | BIT2;
//    P4->SEL0 |= BIT5 | BIT4 | BIT3 | BIT2;

    // Configure Sensors 6-7 (P6.1, P6.0) for ADC
    P6->SEL1 |= BIT1 | BIT0;
    P6->SEL0 |= BIT1 | BIT0;

    // Map memory control registers to sensors
    sensor_memory_ctrl_reg[0] = ADC14_MCTLN_INCH_8;
    sensor_memory_ctrl_reg[1] = ADC14_MCTLN_INCH_9;
    sensor_memory_ctrl_reg[2] = ADC14_MCTLN_INCH_10;
    sensor_memory_ctrl_reg[3] = ADC14_MCTLN_INCH_11;

    sensor_memory_ctrl_reg[4] = ADC14_MCTLN_INCH_12;
    sensor_memory_ctrl_reg[5] = ADC14_MCTLN_INCH_13;
    sensor_memory_ctrl_reg[6] = ADC14_MCTLN_INCH_14;
    sensor_memory_ctrl_reg[7] = ADC14_MCTLN_INCH_15;

    // Set initial sensor to tie to ADC
    ADC14->MCTL[0] = sensor_memory_ctrl_reg[0];
    sampleIndex = 0;



    // Set initial recording to stopped
    //Stop_Recording();

    Reset_Counts();

    /***************** IR LEDs ************************/
//    P7->DIR |= BIT6 | BIT7; // Set P7.6 and P7.7 to output
//    P7->OUT |= BIT6 | BIT7; // Set P7.6 and P7.7 to high

    /***************** Voltage and Current Measurements ************************/
    /*
     * Current and Voltage Measurement Configuration
     *
     * I_BAT_NEG | P5.0 (A5)
     * I_BAT_POS | P5.1 (A4)
     * I_OUT_NEG | P5.2 (A3)
     * I_OUT_POS | P5.3 (A2)
     * I_SOLAR_OUT_NEG | P5.4 (A1)
     * I_SOLAR_OUT_POS | P5.5 (A0)
     * V_SOLAR_IN | P8.3 (A22)
     * V_SOLAR_OUT | P8.5 (A20)
     * V_SOLAR_BATTERY | P8.6 (A19)
     */

    // Configure Current and Voltage Measurements for ADC
    P5->SEL1 |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5;
    P5->SEL0 |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5;
    P8->SEL1 |= BIT3 | BIT5 | BIT6;
    P8->SEL0 |= BIT3 | BIT5 | BIT6;

//    // Current measurements
//    sensor_memory_ctrl_reg[8] = ADC14_MCTLN_INCH_5;
//    sensor_memory_ctrl_reg[9] = ADC14_MCTLN_INCH_4;
//    sensor_memory_ctrl_reg[10] = ADC14_MCTLN_INCH_3;
//    sensor_memory_ctrl_reg[11] = ADC14_MCTLN_INCH_2;
//    sensor_memory_ctrl_reg[12] = ADC14_MCTLN_INCH_1;
//    sensor_memory_ctrl_reg[13] = ADC14_MCTLN_INCH_0;
//
//    // Voltage measurements
//    sensor_memory_ctrl_reg[14] = ADC14_MCTLN_INCH_22;
//    sensor_memory_ctrl_reg[15] = ADC14_MCTLN_INCH_20;
//    sensor_memory_ctrl_reg[16] = ADC14_MCTLN_INCH_19;

    sensor_memory_ctrl_reg[8] = ADC14_MCTLN_INCH_20;


    /***************** General ADC Configurations ************************/

    // Enable ADC interrupt in NVIC module
    NVIC->ISER[0] = 1 << ((ADC14_IRQn) & 31);

    // Sampling time, S&H=16, ADC14 on
    ADC14->CTL0 = ADC14_CTL0_SHT0_2 | ADC14_CTL0_SHP | ADC14_CTL0_ON;
    ADC14->CTL1 = ADC14_CTL1_RES_2;         // Use sampling timer, 12-bit conversion results

    // Enable ADC conversion complete interrupt for memory register 0
    ADC14->IER0 |= ADC14_IER0_IE0;

    //Sample_ADC();
    //ADC14->MCTL[0] = sensor_memory_ctrl_reg[8];
}


// Number of ADC measurements
const int MAX_ADC = 8;

/*
 * Update the ADC input to be measured
 */
void Update_ADC(){

    ADC14->CTL0 &= ~ADC14_CTL0_ENC; // Disable conversion
    sampleIndex = (sampleIndex + 1) % MAX_ADC; // Increment measurement to be sampled, wrap around at MAX_ADC-1
    ADC14->MCTL[0] = sensor_memory_ctrl_reg[sampleIndex]; // Set measurement to be sampled
    ADC14->CTL0 |= ADC14_CTL0_ENC; // Enable conversion
}

/*
 * Start the ADC sampling/conversion
 */
void Sample_ADC(){

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

bool recordingEnabled, BLErecordingEnabled = true;

bool Get_Recording_Status(){
    return recordingEnabled && BLErecordingEnabled;
}

void Stop_Recording(){
    recordingEnabled = false;
}

void Start_Recording(){
    recordingEnabled = true;
    P7->OUT |= BIT6 | BIT7; // Set IR LEDs P7.6 and P7.7 to high
}


void BLE_Stop_Recording(){
    BLErecordingEnabled = false;
}

void BLE_Start_Recording(){
    BLErecordingEnabled = true;
}

//const int TOLERANCE = 0xD00; // Sensor sensitivity
const int TOLERANCE = 0x250; // Sensor sensitivity
int isCovered = 0;

// Arrays to keep track of in/out sensor states (covered or clear)
bool outReading[8], inReading[8];
bool lastInReading[8] = {false};
bool lastOutReading[8] = {false};

// Current and voltage measurements
uint32_t iBatNeg=0, iBatPos=0, iOutNeg=0, iOutPos=0, iSolarOutNeg=0, iSolarOutPos=0, vSolarIn=0, vSolarOut=0, vSolarBat=0;

// Arrays to keep track of in/out sensor trigger times
uint32_t inReadingTime[8], inReadingTimeHigh[8], outReadingTime[8], outReadingTimeHigh[8];
uint32_t lastInReadingTime[8] = {0};
uint32_t lastOutReadingTime[8] = {0};

//const int DEBOUNCE = 120;
//const int TIME_TOL = 150;

const int DEBOUNCE = 120;
const int TIME_TOL = 150;
const int TIME_TOL_2 = 350;

// ADC14 interrupt service routine
void ADC14_IRQHandler(void) {

    uint32_t data = ADC14->MEM[0];

    /****** Sensor test ******/
    // Also in rtc.c
    // Check sensor 0
//    if (ADC14->MEM[0] < TOLERANCE){  // Bee in between LED and sensor
//
//      if(isCovered == 0){
//          inCount++;
//          //fprintf(stderr,"In: %i\n",inCount);
//      }
//
//      P1->OUT |= BIT0;                      // LED on
//      isCovered = 1;
//
//    }else{
//      P1->OUT &= ~BIT0;                     // LED off
//      isCovered = 0;
//    }
//
//    Update_ADC();
    /*************************/


    // Check if recording is on
    if(Get_Recording_Status()){

        P7->OUT |= BIT6 | BIT7; // Set IR LEDs P7.6 and P7.7 to high

        uint32_t currentTime = millis(); // Current system time
        // Cycle through ADC for each measurement
        //int i,j=0;
        //int i;
        //for(i=0; i<MAX_ADC; i++){

            // Check for interrupt pending at sensor i
            //if(ADC14->IFGR0 && sampleIndex == i){

            // Read register
            uint32_t data = ADC14->MEM[0];

            // Sensor measurement
            if(sampleIndex <= 7){

                    int j = sampleIndex / 2;


                    // Check if even sensor to determine if in or out measurement
                    // In reading
                    //sampleIndex
                    if(sampleIndex%2){

                        // Determine if covered or clear
                        if(data >= TOLERANCE){
                            inReading[j] = true;
                        }else{
                            inReading[j] = false;
                        }

                        // Check for change in sensor status
                        if(inReading[j] != lastInReading[j]){

                            // Check for debounce
                            if(currentTime - lastInReadingTime[j] > DEBOUNCE){

                                // Check if bee left sensor
                                if(inReading[j] == false){

                                    // Find how long bee was on sensor
                                    lastInReadingTime[j] = currentTime;
                                    inReadingTimeHigh[j] = currentTime - inReadingTime[j];

                                    // Check that movement was recent
                                    if(currentTime - lastOutReadingTime[j] < TIME_TOL){
                                        if(inReadingTimeHigh[j] < TIME_TOL_2 || outReadingTimeHigh[j] < TIME_TOL_2){

                                            // Increment out count
                                            outCount++;
                                            fprintf(stderr,"o\n");
                                        }
                                    }

                                // Bee on sensor
                                }else{

                                }

                            }
                            inReadingTime[j] = currentTime;
                            lastInReading[j] = inReading[j];

                        }

                    // Out reading
                    }else{

                        // Determine if covered or clear
                        if(data >= TOLERANCE){
                            outReading[j] = true;
                        }else{
                            outReading[j] = false;
                        }

                        //outReading[j] = data >= TOLERANCE;

                        // Check for change in sensor status
                        if(outReading[j] != lastOutReading[j]){

                            // Check for debounce
                            if(currentTime - lastOutReadingTime[j] > DEBOUNCE){

                                // Check if bee left sensor
                                if(outReading[j] == false){

                                    // Find how long bee was on sensor
                                    lastOutReadingTime[j] = currentTime;
                                    outReadingTimeHigh[j] = currentTime - outReadingTime[j];

                                    // Check that movement was recent
                                    if(currentTime - lastInReadingTime[j] < TIME_TOL){
                                        if(inReadingTimeHigh[j] < 350 || outReadingTimeHigh[j] < 350){

                                            // Increment in count
                                            inCount++;
                                            fprintf(stderr,"i\n");
                                        }
                                    }

                                // Bee on sensor
                                }else{

                                }

                            }
                            outReadingTime[j] = currentTime;
                            lastOutReading[j] = outReading[j];

                        }

                        j++;

                    }

                // Current or voltage measurement
                }else if(sampleIndex == 8){

                    vSolarOut = data;
//                    switch(sampleIndex){
//                        case 8:
//                            iBatNeg = data;
//                        break;
//
//                        case 9:
//                            iBatPos = data;
//                        break;
//
//                        case 10:
//                            iOutNeg = data;
//                        break;
//
//                        case 11:
//                            iOutPos = data;
//                        break;
//
//                        case 12:
//                            iSolarOutNeg = data;
//                        break;
//
//                        case 13:
//                            iSolarOutPos = data;
//                        break;
//
//                        case 14:
//                            vSolarIn = data;
//                        break;
//
//                        case 15:
//                            vSolarOut = data;
//                        break;
//
//                        case 16:
//                            vSolarBat = data;
//                        break;
//
//                        default:
//                            fprintf(stderr, "Error\n");
//                        break;
//                    }
                }else{
                    fprintf(stderr, "Error\n");
                }


    }else{
        // Clear interrupt flag
        // Read register
        uint32_t data = ADC14->MEM[0];

        P7->OUT &= ~BIT6; // Set IR LEDs P7.6 and P7.7 to low
        P7->OUT &= ~BIT7;
    }

    // Measure next ADC channel
    Update_ADC();

    //Sample_ADC();

}

uint32_t Get_iBatNeg(){
    return iBatNeg;
}

uint32_t Get_iBatPos(){
    return iBatPos;
}

uint32_t Get_iOutNeg(){
    return iOutNeg;
}

uint32_t Get_iOutPos(){
    return iOutPos;
}

uint32_t Get_iSolarOutNeg(){
    return iSolarOutNeg;
}

uint32_t Get_iSolarOutPos(){
    return iSolarOutPos;
}

uint32_t Get_vSolarIn(){
    return vSolarIn;
}

uint32_t Get_vSolarOut(){
    return vSolarOut;
}

uint32_t Get_vSolarBat(){
    return vSolarBat;
}

