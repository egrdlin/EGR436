/*
 * Don Lin and Ariel Magyar
 * Lab 3: Bluetooth Low Energy
 * EGR 436 101
 * 2/19/2019
 */

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

    printf("EGR 436 Lab 2: Data Storage and Retrieval\n\n");

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

    /* Main Program */
    while(1){
        printf("\nEnter a Command\n");
        printf("  \tSTORE <filename>: Store a file in FRAM.\n");
        printf("  \tDIR: Show all files stored in FRAM\n");
        printf("  \tMEM: Display total and available FRAM memory.\n");
        printf("  \tDELETE <number>: Delete an entry from FRAM.\n");
        printf("  \tREAD <number>: Display title and text of entry.\n");
        printf("  \tCLEAR: Wipe all FRAM entries.\n");
        printf("  \tQ: Quit program\n\n");
        fflush(stdin);
        gets(entry);

        int data_length = 50;

        // Check if recognized command received
        if(checkCommand(STORE_COMMAND, entry)){
            char poem_buffer[50] = {0};
            strcpy(poem_buffer,entry + strlen(STORE_COMMAND));

            FILE *fptr;
            fptr = fopen(poem_buffer,"r");
            if (fptr == NULL){
               printf("Error opening file.");

            }else{

                char file_data[1000] = {0};
                strcpy(file_data, "STORE ");

                int i=6;
                while(fscanf(fptr,"%c", file_data+i) != EOF){
                    i++;
                }

                file_data[i] = '\0';
                fclose(fptr);

                transmit_string(file_data);
                Sleep(5000); // Wait for poem to be stored in memory
                printf("\nFile stored.\n");

            }

        }else if(checkCommand(DELETE_COMMAND, entry)){

            transmit_string(entry);
            Sleep(1000);
            printf("\nEntry %i deleted.\n",entry[7]-'0');

        }else if(!strcmp(entry,"DIR")){

            transmit_string(entry);

            get_rx_data(data_length);

            char *token;
            const char s[2] = ",";
            int index = 1;

            printf("----Poem Library----\n");
            if(strlen(rx_buffer) == 0){
                printf("No poems stored.\n\n");
            }else{
                token = strtok(rx_buffer, s);
                bool flag = true;

                while(token != NULL){
                    if(flag){
                        printf("%i. %s\t",index, token);
                    }else{
                        printf("%s bytes\n",token);
                        index++;
                    }
                    token = strtok(NULL, s);
                    flag = !flag;
                }
            }

        }else if(!strcmp(entry,"MEM")){
            transmit_string(entry);
            get_rx_data(data_length);
            printf("%s of 8193 bytes used.\n\n",rx_buffer);

        }else if(checkCommand(READ_COMMAND, entry)){
            /*
            char transmit[50];
            sprintf(transmit, "RED%i",entry[5]-'0');
            transmit_string(transmit);*/

            transmit_string(entry);

            get_rx_data(1000);
            printf("Poem %i is:\n",entry[5]-'0');
            printf("%s\n\n",rx_buffer);

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
    printf("%s\nLength: %i, Transmitted: %i bytes\n",data,strlen(data),rwlen);
}

/*
 * Get RX data from UART
 * @param data_length MAx length of data to expect (will time out if received data less than this length)
 */
void get_rx_data(int data_length){
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
