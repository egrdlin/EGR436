#include "msp.h"
#include "adc.h"

volatile uint32_t tick;

void Init_Timer(){

    // Enable SysTick Module
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk |
            SysTick_CTRL_ENABLE_Msk;

    // Set SysTick period = 0x20000
    //SysTick->LOAD = 0x20000 - 1;

    // SysTick set to MCK = 12MHz
    // Set SysTick period to 1ms = 12,000 cycles
    SysTick->LOAD = 12000 - 1;

    // Clear the SysTick current value register by writing
    // a dummy value
    SysTick->VAL = 0x01;

    // Enable SysTick interrupt
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

    __enable_irq();

    tick = 0;

    /****** Timer test ******/
    // Will not work on custom board
//    P1->DIR |= BIT0;
//    P1->OUT &= ~BIT0;
    /***********************/
}

uint32_t millis(){
    return tick;
}

// Configured to be called every millisecond
void SysTick_Handler(void){
//
//        if(tick%1000 == 0){
//            Sample_ADC();
//        }

    //Sample_ADC();

    // Protect against going out of range
    if(tick < 0xFFFF-1){
        tick++;
    }else{
        tick = 0;
    }

    /****** Timer test ******/
    // Test with 1 second LED blink
//    if(tick%1000 == 0){
//        P1->OUT ^= BIT0;
//    }
    /***********************/

}
