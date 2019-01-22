#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include <conio.h>

#define DEFAULT_BLINK_RATE 60

bool tx_set_blink_rate();
bool tx_get_blink_rate();
void get_rx_data();

    HANDLE	hCom; //Handle variable
	DWORD	rwlen; // read/write length
int blink_rate;

int main()
{


    char port_name[20];
    int port;
    char input;


    printf("EGR 436 Lab 1: UART to USB Communication\n\n");

    printf("------Serial Port Setup------\n");
    printf("Serial Port Number of UART Dongle: ");
    scanf("%d",&port);

    /* Open COM Port */
    sprintf( port_name, "\\\\.\\COM%d", port );

	hCom = CreateFile( port_name, GENERIC_READ|GENERIC_WRITE,
					0, 0, OPEN_EXISTING, 0, 0 );

    if( hCom==INVALID_HANDLE_VALUE ){
        printf( "\tError: COM%d is not available.\n", port );
        return -2;
    }

    /* Configure DCB Settings to Set 9600 Baud Rate */
    DCB dcbSerialParams = {0};

    dcbSerialParams.DCBlength=sizeof(dcbSerialParams);

    if (!GetCommState(hCom, &dcbSerialParams)){
        printf("\tWarning: Unable to get DCB state of serial port\n");
    }

    if(!SetCommState(hCom, &dcbSerialParams)){
        printf("\tWarning: Unable to set serial port DCB settings\n");
    }


    /* Set COM Timeouts */
    COMMTIMEOUTS timeouts={0};

    timeouts.ReadIntervalTimeout=50;
    timeouts.ReadTotalTimeoutConstant=50;
    timeouts.ReadTotalTimeoutMultiplier=10;
    timeouts.WriteTotalTimeoutConstant=50;
    timeouts.WriteTotalTimeoutMultiplier=10;

    if(!SetCommTimeouts(hCom, &timeouts)){
        printf("Warning: Unable to set serial port COM timeout settings\n");
    }

    printf("COM%d Opened Successfully.\n\n",port);




    /* Main Program */

    // Set default blink rate and send to MSP
    blink_rate = DEFAULT_BLINK_RATE;
    printf("Setting initial blink rate of %ibps\n", blink_rate);
    tx_set_blink_rate();
    get_rx_data();

    //printf("The current rate is %ibpm\n", blink_rate);

    while(1){
        printf("\nEnter a Command\n");
        printf("  d\tDecrease Blink Rate by 2bpm\n");
        printf("  u\tIncrease Blink Rate by 2bpm\n");
        printf("  r\tReset Blink Rate to 60bpm\n");
        printf("  q\tQuit program\n\n");
        fflush(stdin);
        input = getch(); // Need to error check input
        switch(input){
            case 'd':
                /* baud rate decreases by 2bpm */
                /* LED blink rate decreases by 2bpm */
                (blink_rate >= 2) ? (blink_rate -= 2) : (blink_rate = 0);
                tx_set_blink_rate();
                get_rx_data();
                break;
            case 'u':
                /* baud rate increases by 2bpm */
                /* LED blink rate increases by 2bpm */
                blink_rate += 2;
                tx_set_blink_rate();
                get_rx_data();
                break;
            case 'r':
                /*reset blink rate to 60 bpm */
                blink_rate = DEFAULT_BLINK_RATE;
                tx_set_blink_rate();
                get_rx_data();
                break;
            case 'q':
                CloseHandle( hCom );//close the handle
                exit(0);
        }


        printf("The current rate is now %ibps\n", blink_rate);
    }

    CloseHandle( hCom );//close the handle
    return 0;
}

const char end_char = '\0';

bool tx_set_blink_rate(){
    char tx_buffer[32] = {0};

    int txlen; // Length of data to be transmitted (bytes)

    // Indicate message is to set blink rate
    //tx_buffer[0] = 's';

    // Put blink rate into transmit buffer array
    sprintf(tx_buffer, "%i%c", blink_rate, end_char);

    // Get number of bytes to send
    txlen = strlen(tx_buffer)+1;

    // Send data through serial port
    WriteFile( hCom, tx_buffer, txlen, &rwlen, 0 );

    //printf("%ld bytes of Data transmitted successfully\n",rwlen);
    printf("Transmitted data: %s\n", tx_buffer);

    return (txlen == rwlen) ? true : false;
}

void get_rx_data(){
    char rx_buffer[10] = {0};
    int index = 0;

    do{
        ReadFile( hCom, rx_buffer, sizeof(rx_buffer), &rwlen, 0 ); // read data from the serial port buffer of the OS
        //printf("%ld of out of %d bytes read from port and data is %s\n",rwlen,sizeof(rx_buffer),rx_buffer);
        index++;
    } while(rx_buffer[index] != '\0' || index < 9);

    printf("Received data: %s\n\n", rx_buffer);
}
