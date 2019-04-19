#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include <conio.h>

bool checkCommand(const char *checkCommand,char *userCommand);
void get_rx_data(int data_length);
void transmit_string(char *data);
void closeCOM();

HANDLE	hMasterCOM; //Handle variable
DWORD	rwlen; // read/write length
DCB dcbMasterInitState;

// Commands
const char DELETE_COMMAND[] = "DELETE ";
const char STORE_COMMAND[] = "STORE ";
const char READ_COMMAND[] = "READ ";
char rx_buffer[200] = {0};

int main()
{
    char port_name[20];
    int port;

    printf("\n****************************************\n");
    printf("\nEGR 436 Final Project: Honey Bee Counter\n");
    printf("\nCreated by Don Lin and Ariel Magyar\n");
    printf("\n****************************************\n\n");

    printf("------Serial Port Setup------\n");
    printf("Serial Port Number of UART Dongle: ");
    scanf("%d",&port);

    /* Open COM Port */
    sprintf( port_name, "\\\\.\\COM%d", port );
/*
	hCom = CreateFile( port_name, GENERIC_READ|GENERIC_WRITE,
					0, 0, OPEN_EXISTING, 0, 0 );*/
	hMasterCOM = CreateFile( port_name, GENERIC_READ|GENERIC_WRITE,
					0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,0);

    if( hMasterCOM==INVALID_HANDLE_VALUE ){
        printf( "\tError: COM%d is not available.\n", port );
        return -2;
    }

    // Purge to clear existing data from COM port
    if(!PurgeComm(hMasterCOM, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR)){
        printf( "\tError: PurgeComm failed.\n");
        return -2;
    }

    if(!GetCommState(hMasterCOM, &dcbMasterInitState)){
        printf("Warning: Unable to get initial COM state.");
    }

    DCB dcbMaster = dcbMasterInitState;
    dcbMaster.BaudRate = 9600;
    dcbMaster.Parity = NOPARITY;
    dcbMaster.ByteSize = 8;
    dcbMaster.StopBits = ONESTOPBIT;
    SetCommState(hMasterCOM, &dcbMaster);
    Sleep(60);

    /* Set COM Timeouts */

    COMMTIMEOUTS timeouts={0};

    timeouts.ReadIntervalTimeout=500;
    timeouts.ReadTotalTimeoutConstant=500;
    timeouts.ReadTotalTimeoutMultiplier=10;
    timeouts.WriteTotalTimeoutConstant=500;
    timeouts.WriteTotalTimeoutMultiplier=10;


    if(!SetCommTimeouts(hMasterCOM, &timeouts)){
        printf("Warning: Unable to set serial port COM timeout settings\n");
    }

    printf("COM%d Opened Successfully.\n\n",port);

    char entry[50] = {0};

    FILE *fptr;
    char name[20];


    /* Main Program */
    while(1){
        printf("Enter a Command:\n");
        printf("  READ: Record bee count data to a csv file.\n");
        printf("  Q:    Quit the program.\n\n");
        printf("Entry: ");
        fflush(stdin);
        gets(entry);

        int data_length = 50;

        // Check if recognized command received
        if(!strcmp(entry,"READ")){
            /*
            char transmit[50];
            sprintf(transmit, "RED%i",entry[5]-'0');
            transmit_string(transmit);*/

            fptr = fopen("data/bee_counter_data.csv", "w");

            if(fptr == NULL){
                printf("Unable to create file\n");
                //exit(EXIT_FAILURE);
            }else{

                transmit_string(entry);

                // Get three bytes of number of entries
                get_rx_data(100);
                //printf("RX Data: %s\n",rx_buffer);

                int entries = atoi(rx_buffer);
                printf("Entries: %i\n",entries);

                // Loop for number of entries
                int i;
                for(i=0; i<entries; i++){
                    transmit_string("READY");
                    get_rx_data(200);
                    printf("RX Data Entry %i: %s\n",i+1,rx_buffer);
                    fputs(rx_buffer, fptr);
                    //PurgeComm(hMasterCOM, PURGE_RXCLEAR);

                }

                printf("Data saved to file.\n\n");
                fclose(fptr);

            }

        }else if(!strcmp(entry,"CLEAR")){
            //transmit_string("CLR");

            transmit_string(entry);
            Sleep(500);
            printf("\nEntries cleared.");


        }else if(!strcmp(entry,"Q")){
            printf("\nProgram exiting.");

            closeCOM(); //close the handle
            exit(0);
        }else{
            printf("\n\nInvalid command received, please try again.");
        }
    }

    closeCOM(); //close the handle
    return 0;
}

/*
 * Transmit a string over UART
 * @param data String to transmit
 */
void transmit_string(char *data){

    WriteFile( hMasterCOM, data, strlen(data)+1, &rwlen, 0 );
    //printf("%s\nLength: %i, Transmitted: %i bytes\n",data,strlen(data),rwlen);
}

/*
 * Get RX data from UART
 * @param data_length MAx length of data to expect (will time out if received data less than this length)
 */
void get_rx_data(int data_length){
    int i;
    for(i=0; i<200; i++){
        rx_buffer[i] = 0;
    }
    Sleep(1000);

    ReadFile( hMasterCOM, rx_buffer, data_length, &rwlen, 0 );

}

/*
 * Check to see if the user command entered is valid (for commands that include a filename after)
 * @param checkCommand Command to check for
 *        userCommand User entered command
 * @return true if matches, false if not
 */
bool checkCommand(const char *checkCommand,char *userCommand){
    bool compare = true;

    // Check command length
    if(strlen(userCommand) < strlen(checkCommand) + 1){
        compare = false;
    }else{
        int i;
        for(i=0; i<strlen(checkCommand); i++){
            if(checkCommand[i] != userCommand[i]){
                compare = false;
                break;
            }
        }
    }

    return compare;
}

void closeCOM(){
    SetCommState(hMasterCOM, &dcbMasterInitState);
    Sleep(60);
    CloseHandle(hMasterCOM);
    hMasterCOM = INVALID_HANDLE_VALUE;
}
