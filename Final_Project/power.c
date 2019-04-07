#include "msp.h"
#include "power.h"

/*
 * Configurations to save power
 */
void Init_Power(){
    LP_boardInit();
    disable_svsm();
    //SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;    // Sleep on exit from ISR
    //__sleep(); // LPM?

    /******** LPM 0 *************/
    //SCB->SCR &=  ~(SCB_SCR_SLEEPDEEP_Msk);                  // Turn off Deep Sleep bit (LPM0)
    /***************************/

    /******** LPM 3 *************/
//    PCM->CTL0 = PCM_CTL0_KEY_VAL | PCM_CTL0_LPMR__LPM3;     // LPM3. Core Voltage setting is 0;
//    SCB->SCR |= (SCB_SCR_SLEEPDEEP_Msk);                    // Turn on Deep Sleep bit
    /***************************/
    //__no_operation();                   // For debugger
}

/*
 * Taken from EGR 436 lecture notes.
 * LP_baordInit()
 * Places GPIO ports in the lowest power configuration. By making all ports output low.
 */
void LP_boardInit()
{
    P1->OUT =  0x00;    P1->DIR =  0xFF;
    P2->OUT =  0x00;    P2->DIR =  0xFF;
    P3->OUT =  0x00;    P3->DIR =  0xFF;
    P4->OUT =  0x00;    P4->DIR =  0xFF;
    P5->OUT =  0x00;    P5->DIR =  0xFF;
    P6->OUT =  0x00;    P6->DIR =  0xFF;
    P7->OUT =  0x00;    P7->DIR =  0xFF;
    P8->OUT =  0x00;    P8->DIR =  0xFF;
    P9->OUT =  0x00;    P9->DIR =  0xFF;
    P10->OUT = 0x00;    P10->DIR = 0xFF;
    PJ->OUT =  0x00;    PJ->DIR =  0xFF;
}

/*
 * Taken from EGR 436 lecture notes.
 * disable_svsm()
 * from pp. 412 of slau346h (MSP432P4xx Technical Users Manual)
 *
 * The SVSMHis capable of either supervising or monitoring the high side voltage,VCC.
 * The SVSMHcan be disabled using the SVSMHOFFbit to save power if the functionality is
 * not needed.By default,the SVSMHis enabled in full performancemode.In low power
 * modes the SVSMHcan also be taken out of full performance mode by setting the SVSMHLP bit
 * to 1 resulting in lower power but slowerresponsetime.The SVSMHLPbit is effective only
 * in LPM3,LPM4,LPM3.5,and LPM4.5modes
 */
void disable_svsm()
{
    PSS->KEY    = PSS_KEY_KEY_VAL;          // Unlocks PSS register so that it can be changed
    PSS->CTL0   |= PSS_CTL0_SVSMHOFF;       // Turn off PSS high-side supervisor
    PSS->KEY    = 0;                        // Lock PSS register
}

/*
 * Taken from EGR 436 lecture notes.
 * sram_bank_config()
 * from pp. 272-3 of slau346h (MSP432P4xx Technical Users Manual)
 *
 * The application can optimize the power consumption of the SRAM.To enable this optimization,
 * the SRAM memory is divided into different banks that can be powereddownindividually.Banks
 * that are powered down remain powered down in both active and low-power modes of operation,
 * there by limiting any unnecessary in rush current when the device transitions between active
 * and retention-based low-powermodes.The application can also disable one (or more) banks for
 * a certain stage in the processing and re-enable it for another stage. When a particular bank
 * is disabled,reads to its address space return 0h, and writes are discarded. To prevent 'holes'
 * in the memory map, if a particular bank is enabled, all of the lower banks are also forced
 * to enabled state. This ensures a contiguous memory map though the set of enabled banks instead
 * of allowing a disabled bank to appear between enabled banks. For example:
 * •  If there are eight banks in the device,values of 00111111and 00000111 are acceptable.
 * •  Values like 00010111 are not valid,and the resultant bank configuration is automatically
 *      set to00011111.
 * •  For example,for a 8-bankSRAM,the only allowed values are 00000001,00000011,00000111,00001111,
 *      00011111,00111111,01111111and 11111111.
 * Bank0 of SRAM is always enabled and cannot be disabled. For all other banks,any enable or disable
 * change results in the SRAM_RDYbit of the SYS_SRAM_BANKENregisterbeingset to 0 until the
 * configuration change is effective.Any accesses to the SRAMis stalled during this time,and access
 * resumes only after the SRAMbanksare ready for read or write operations. This is handled transparently
 * and does not require any code intervention.
 *
 * The application can optimize the leakage power consumption of the SRAMin LPM3 and LPM4 modes
 * of operation.To enable this, each SRAM bank can be individually configured for retention.
 * Banks that are enabled for retention retain their data through the LPM3 and LPM4 modes.The
 * application can also retain a subset of the enabled banks.  The Bank Retention Configuration
 * for any bank have an effect only when that bank is enabled using the SRAM Bank Enable
 * Configuration is set.
 *
 * For example,the application may need 32KB of SRAM for its processing needs (4 banks are kept
 * enabled). However,of these four banks, only one bank may contain critical data that must be
 * retained in LPM3 or LPM4, while the rest are powered off completely to minimize power consumption.
 * See SYS_SRAM_BANKRET register for details on how individual banks can be controlled by the application.
 */
void sram_bank_config()
{
    SYSCTL->SRAM_BANKEN  = SYSCTL_SRAM_BANKEN_BNK7_EN;      //Enable all banks (all banks enabled below 7
    SYSCTL->SRAM_BANKRET = SYSCTL_SRAM_BANKRET_BNK7_RET;    //Retain all banks
}
