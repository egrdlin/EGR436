#include "msp.h"

void init_rtc(){
//    WDT_A->CTL = WDT_A_CTL_PW |             // Stop WDT
//            WDT_A_CTL_HOLD;

//    P1->DIR |= BIT0 ;                       // Configure P1.0 LED
//    P1->OUT &= ~( BIT0 );

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

    // TODO: Set date based on user input from app/computer
    RTC_C->YEAR = 0x2016;                   // Year = 0x2016
    RTC_C->DATE = (0x4 << RTC_C_DATE_MON_OFS) | // Month = 0x04 = April
            (0x05 | RTC_C_DATE_DAY_OFS);    // Day = 0x05 = 5th
    RTC_C->TIM1 = (0x01 << RTC_C_TIM1_DOW_OFS) | // Day of week = 0x01 = Monday
            (0x10 << RTC_C_TIM1_HOUR_OFS);  // Hour = 0x10
    RTC_C->TIM0 = (0x32 << RTC_C_TIM0_MIN_OFS) | // Minute = 0x32
            (0x45 << RTC_C_TIM0_SEC_OFS);   // Seconds = 0x45

    // Start RTC calendar mode
    RTC_C->CTL13 = RTC_C->CTL13 & ~(RTC_C_CTL13_HOLD);

    // Lock the RTC registers
    RTC_C->CTL0 = RTC_C->CTL0 & ~(RTC_C_CTL0_KEY_MASK);

    // Enable all SRAM bank retentions prior to going to LPM3 (Deep-sleep)
    SYSCTL->SRAM_BANKRET |= SYSCTL_SRAM_BANKRET_BNK7_RET;

    // Enable global interrupt
    __enable_irq();

    NVIC->ISER[0] = 1 << ((RTC_C_IRQn) & 31);

    // TODO: Optimize code for low-power mode
    //SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;    // Sleep on exit from ISR
}

/*
 * ISR for internal RTC
 */
void RTC_C_IRQHandler(void)
{
    if (RTC_C->CTL0 & RTC_C_CTL0_TEVIFG)
    {
        P1->OUT ^= BIT0;

        // Unlock the RTC module and clear time event interrupt flag
        RTC_C->CTL0 = (RTC_C->CTL0 & ~(RTC_C_CTL0_KEY_MASK |  RTC_C_CTL0_TEVIFG)) | RTC_C_KEY;

        // Re-lock the RTC
        RTC_C->CTL0 = RTC_C->CTL0 & ~(RTC_C_CTL0_KEY_MASK);
    }
}
