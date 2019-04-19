#include "msp.h"
#include "adc.h"
#include "spi.h"
#include "rtc.h"

#include <stdbool.h>

void Init_RTC(){
    PJ->SEL0 |= BIT0 | BIT1;                // set LFXT pin as second function

    CS->KEY = CS_KEY_VAL ;                  // Unlock CS module for register access

    CS->CTL2 |= CS_CTL2_LFXT_EN;            // LFXT on

    // Loop until XT1, XT2 & DCO fault flag is cleared
    do
    {
       // Clear XT2,XT1,DCO fault flags
       CS->CLRIFG |= CS_CLRIFG_CLR_DCOR_OPNIFG | CS_CLRIFG_CLR_HFXTIFG |
               CS_CLRIFG_CLR_LFXTIFG | CS_CLRIFG_CLR_FCNTLFIFG;
       SYSCTL->NMI_CTLSTAT &= ~ SYSCTL_NMI_CTLSTAT_CS_SRC;
    } while ((SYSCTL->NMI_CTLSTAT | SYSCTL_NMI_CTLSTAT_CS_FLG)
            && (CS->IFG & CS_IFG_LFXTIFG)); // Test oscillator fault flag

    // Select ACLK as LFXTCLK
    CS->CTL1 &= ~(CS_CTL1_SELA_MASK) | CS_CTL1_SELA_0;
    CS->KEY = 0;                            // Lock CS module from unintended accesses

    // Configure RTC

    // Unlock RTC key protected registers
    // RTC enable, BCD mode, RTC hold
    // enable RTC read ready interrupt
    // enable RTC time event interrupt
    // set time event interrupt to trigger when minute changes
    RTC_C->CTL0 = RTC_C_KEY | RTC_C_CTL0_TEVIE;
    RTC_C->CTL13 = RTC_C_CTL13_HOLD |
            RTC_C_CTL13_MODE |
            RTC_C_CTL13_BCD |
            RTC_C_CTL13_TEV_0;

    RTC_C->YEAR = 0x2019;                   // Year = 0x2019
    RTC_C->DATE = (0x3 << RTC_C_DATE_MON_OFS) | // Month = 0x03 = March
            (0x13 | RTC_C_DATE_DAY_OFS);    // Day = 0x13 = 13th
    RTC_C->TIM1 = (0x01 << RTC_C_TIM1_DOW_OFS) | // Day of week = 0x01 = Monday
            (0x10 << RTC_C_TIM1_HOUR_OFS);  // Hour = 0x10
    RTC_C->TIM0 = (0x29 << RTC_C_TIM0_MIN_OFS) | // Minute = 0x32
            (0x30 << RTC_C_TIM0_SEC_OFS);   // Seconds = 0x45

    // Start RTC calendar mode
    RTC_C->CTL13 = RTC_C->CTL13 & ~(RTC_C_CTL13_HOLD);

    // Lock the RTC registers
    RTC_C->CTL0 = RTC_C->CTL0 & ~(RTC_C_CTL0_KEY_MASK);

    // Enable all SRAM bank retentions prior to going to LPM3 (Deep-sleep)
    SYSCTL->SRAM_BANKRET |= SYSCTL_SRAM_BANKRET_BNK7_RET;

    // Enable global interrupt
    __enable_irq();

    NVIC->ISER[0] = 1 << ((RTC_C_IRQn) & 31);

    Check_Recording();
}

/*
 * Check to turn sensor system off or on
 * ON: 6:00 AM (sunrise)
 * OFF: 9:00 PM (sunset)
 * Note: RTCHOUR register is in 24 hour format
 */
bool Check_Recording(){
    if(RTCHOUR >= 0x06 && RTCHOUR < 0x21){
        Start_Recording();
        //__sleep();
        //__no_operation();
        return true;
    }else{
        Stop_Recording();
        return false;
        //__sleep();
        //SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;    // Do not wake up on exit from ISR
        //SCB->SCR |= (SCB_SCR_SLEEPDEEP_Msk); // Set the deep sleep bit
        //__sleep(); // Go to LPM 3
    }
}


/*
 * ISR for internal RTC
 * Triggered when minute changes
 * Store in/out count data every 15 minutes
 */
void RTC_C_IRQHandler(void)
{

    /****** Sensor test ******/
    // Trigger every minute
//    if (RTC_C->CTL0 & RTC_C_CTL0_TEVIFG)
//    {
//        uint16_t inCount, outCount;
//        inCount = Get_In_Count();
//        outCount = Get_Out_Count();
//        // Store time and count values, reset counts afterwards
//        Store_Time(inCount, outCount);
//
//        fprintf(stderr,"I: %i\nO: %i\nM: %x\n", inCount, outCount, RTCMIN);
//
//        Reset_Counts();
//    }
//
//    // Unlock the RTC module and clear time event interrupt flag
//    RTC_C->CTL0 = (RTC_C->CTL0 & ~(RTC_C_CTL0_KEY_MASK |  RTC_C_CTL0_TEVIFG)) | RTC_C_KEY;
//
//    // Re-lock the RTC
//    RTC_C->CTL0 = RTC_C->CTL0 & ~(RTC_C_CTL0_KEY_MASK);

    /***********************/


    // Check for minute time event interrupt
    // Check that minute is divisible by 15
    // Check that recording is enabled
    if (RTC_C->CTL0 & RTC_C_CTL0_TEVIFG && (RTCMIN == 0x00 || RTCMIN == 0x15 || RTCMIN == 0x30 || RTCMIN == 0x45) && Get_Recording_Status())
    {

        // Store time and count values, reset counts afterwards
        Store_Time(Get_In_Count(), Get_Out_Count());

        Reset_Counts();
    }

    // Check whether to turn sensor system off or on
    Check_Recording();

    // Unlock the RTC module and clear time event interrupt flag
    RTC_C->CTL0 = (RTC_C->CTL0 & ~(RTC_C_CTL0_KEY_MASK |  RTC_C_CTL0_TEVIFG)) | RTC_C_KEY;

    // Re-lock the RTC
    RTC_C->CTL0 = RTC_C->CTL0 & ~(RTC_C_CTL0_KEY_MASK);
}
