#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include <conio.h>

HANDLE	hCom; //Handle variable
DWORD	rwlen; // read/write length

int main()
{
    char port_name[20];
    int port;

    printf("EGR 436 Lab 2: Data Storage and Retrieval\n\n");

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

    char input;
    do{
        printf("\nInput: ");
        fflush(stdin);
        input = getch(); // Need to error check input
        printf("%c\n",input);
        char data[] = "A";
        char rx_buffer[10] = {0};

        WriteFile( hCom, data, strlen(data), &rwlen, 0 );

        printf("%ld bytes of Data transmitted successfully\n",rwlen);
        printf("Transmitted data: %s\n", data);

        ReadFile( hCom, rx_buffer, strlen(data), &rwlen, 0 );

        printf("Received data: %c\n\n", rx_buffer[0]);
    }while(input != 'q');


    CloseHandle( hCom );//close the handle
    return 0;
}

