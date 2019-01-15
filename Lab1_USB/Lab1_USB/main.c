#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>

bool init_UART_dongle();
bool WriteData(HANDLE handle, BYTE* data, DWORD length, DWORD* dwWritten);
bool ReadData(HANDLE handle, BYTE* data, DWORD length, DWORD* dwRead, UINT timeout);
void close_UART_dongle();

int main()
{
    int blink_rate = 60;
    char input;

    printf("EGR 436 Lab 1: UART to USB Communication\n\n");

    init_UART_dongle();

    printf("The current rate is %ibpm\n", blink_rate);

    while(1){
        printf("\nEnter a Command\n");
        printf("  d\tDecrease Blink Rate by 2bpm\n");
        printf("  i\tIncrease Blink Rate by 2bpm\n");
        printf("  r\tReset Blink Rate to 60bpm\n");
        printf("  q\tQuit program\n");
        fflush(stdin);
        input = getch(stdin); // Need to error check input
        if(input == 'q')
            exit(0);
        // Send UART message of char to micro
        // Get blink rate
        // Set blink_rate to blink rate from micro
        printf("The current rate is %ibpm\n", blink_rate);
    }
    return 0;
}
HANDLE hMasterCOM;
DCB dcbMasterInitState;

bool init_UART_dongle(){
    // Open COM3 Port
    hMasterCOM = CreateFile("\\\\.\\COM3",
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        0);

    // Purge COM Port
    PurgeComm(hMasterCOM, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

    // Save the COM port's original state
    GetCommState(hMasterCOM, &dcbMasterInitState);

    // Set up a DCB Structure to Set the New COM State
    DCB dcbMaster = dcbMasterInitState;
    dcbMaster.BaudRate = 57600;
    dcbMaster.Parity = NOPARITY;
    dcbMaster.ByteSize = 8;
    dcbMaster.StopBits = ONESTOPBIT;
    SetCommState(hMasterCOM, &dcbMaster);
    Sleep(60000);

    return 0;
}

bool WriteData(HANDLE handle, BYTE* data, DWORD length, DWORD* dwWritten){
    bool success = false;
    OVERLAPPED o = {0};
    o.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!WriteFile(handle, (LPCVOID)data, length, dwWritten, &o))
    {
        if (GetLastError() == ERROR_IO_PENDING)
            if (WaitForSingleObject(o.hEvent, INFINITE) == WAIT_OBJECT_0)
                if (GetOverlappedResult(handle, &o, dwWritten, FALSE))
                    success = true;
    }
    else
        success = true;
    if (*dwWritten != length)
        success = false;
    CloseHandle(o.hEvent);
    return success;
}

bool ReadData(HANDLE handle, BYTE* data, DWORD length, DWORD* dwRead, UINT timeout){
    bool success = false;
    OVERLAPPED o = {0};

    o.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!ReadFile(handle, data, length, dwRead, &o))
    {
        if (GetLastError() == ERROR_IO_PENDING)
            if (WaitForSingleObject(o.hEvent, timeout) == WAIT_OBJECT_0)
                success = true;
        GetOverlappedResult(handle, &o, dwRead, FALSE);
    }
    else
        success = true;
    CloseHandle(o.hEvent);
    return success;
}

void close_UART_dongle(){
    SetCommState(hMasterCOM, &dcbMasterInitState);
    Sleep(60000);
    CloseHandle(hMasterCOM);
    hMasterCOM = INVALID_HANDLE_VALUE;
}
