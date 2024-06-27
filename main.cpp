#include <windows.h>
#include <vector>
#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include <regex>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void PopulateComPorts(HWND hwndComboBox);
void PopulateBaudRates(HWND hwndComboBox);
void PopulateDataBits(HWND hwndComboBox);
void PopulateParity(HWND hwndComboBox);
void PopulateStopBits(HWND hwndComboBox);
void SetDefaultValues();
void OnOpenPort(HWND hwnd);
void OnSendCommand(HWND hwnd);
void OnSendData(HWND hwnd);
DWORD WINAPI ReadFromPort(LPVOID lpParam);
DWORD WINAPI WriteToPort(LPVOID lpParam);
std::string RemoveAnsiCodes(const std::string &input);

HANDLE hComm = INVALID_HANDLE_VALUE;
HWND hComboBoxPort, hComboBoxBaudRate, hComboBoxDataBits, hComboBoxParity, hComboBoxStopBits;
HWND hButtonOpenPort, hCommandInput, hButtonSendCommand;
HWND hDataOutput, hRawDataOutput, hDataInput, hButtonSendData, hStatus;
HANDLE hReadThread = NULL, hWriteThread = NULL;
bool continueReading = true;
bool continueWriting = true;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "ComPortWindowClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "COM Port Manager",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 600,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    continueReading = false;
    continueWriting = false;
    if (hReadThread != NULL) {
        WaitForSingleObject(hReadThread, INFINITE);
        CloseHandle(hReadThread);
    }
    if (hWriteThread != NULL) {
        WaitForSingleObject(hWriteThread, INFINITE);
        CloseHandle(hWriteThread);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            CreateWindow("STATIC", "Port Settings", WS_CHILD | WS_VISIBLE, 20, 20, 150, 20, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "COM Port:", WS_CHILD | WS_VISIBLE, 20, 50, 150, 20, hwnd, NULL, NULL, NULL);
            hComboBoxPort = CreateWindow("COMBOBOX", NULL, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE | WS_VSCROLL, 20, 70, 150, 200, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Baud Rate:", WS_CHILD | WS_VISIBLE, 20, 110, 150, 20, hwnd, NULL, NULL, NULL);
            hComboBoxBaudRate = CreateWindow("COMBOBOX", NULL, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE | WS_VSCROLL, 20, 130, 150, 200, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Data Bits:", WS_CHILD | WS_VISIBLE, 20, 170, 150, 20, hwnd, NULL, NULL, NULL);
            hComboBoxDataBits = CreateWindow("COMBOBOX", NULL, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE | WS_VSCROLL, 20, 190, 150, 200, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Parity:", WS_CHILD | WS_VISIBLE, 20, 230, 150, 20, hwnd, NULL, NULL, NULL);
            hComboBoxParity = CreateWindow("COMBOBOX", NULL, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE | WS_VSCROLL, 20, 250, 150, 200, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Stop Bits:", WS_CHILD | WS_VISIBLE, 20, 290, 150, 20, hwnd, NULL, NULL, NULL);
            hComboBoxStopBits = CreateWindow("COMBOBOX", NULL, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE | WS_VSCROLL, 20, 310, 150, 200, hwnd, NULL, NULL, NULL);

            hButtonOpenPort = CreateWindow("BUTTON", "Open Port", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 20, 350, 150, 30, hwnd, (HMENU)1, NULL, NULL);

            CreateWindow("STATIC", "Command/Data Input:", WS_CHILD | WS_VISIBLE, 20, 390, 150, 20, hwnd, NULL, NULL, NULL);
            hCommandInput = CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 20, 410, 150, 20, hwnd, NULL, NULL, NULL);
            hButtonSendCommand = CreateWindow("BUTTON", "Send Command/Data", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 20, 440, 150, 30, hwnd, (HMENU)2, NULL, NULL);

            hDataOutput = CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL, 200, 20, 460, 400, hwnd, NULL, NULL, NULL);

            hRawDataOutput = CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL, 680, 20, 460, 400, hwnd, NULL, NULL, NULL);

            hStatus = CreateWindow("STATIC", "Status: Ready", WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 480, 540, 20, hwnd, NULL, NULL, NULL);

            PopulateComPorts(hComboBoxPort);
            PopulateBaudRates(hComboBoxBaudRate);
            PopulateDataBits(hComboBoxDataBits);
            PopulateParity(hComboBoxParity);
            PopulateStopBits(hComboBoxStopBits);

            SetDefaultValues();
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case 1:
                    OnOpenPort(hwnd);
                    break;
                case 2:
                    OnSendCommand(hwnd);
                    break;
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void PopulateComPorts(HWND hwndComboBox) {
    std::vector<std::string> comPorts;
    char portName[10];
    for (int i = 1; i <= 256; ++i) {
        snprintf(portName, sizeof(portName), "\\\\.\\COM%d", i);
        HANDLE hComm = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hComm != INVALID_HANDLE_VALUE) {
            CloseHandle(hComm);
            std::ostringstream oss;
            oss << "COM" << i;
            comPorts.push_back(oss.str());
        }
    }
    for (const auto& port : comPorts) {
        SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)port.c_str());
    }
}

void PopulateBaudRates(HWND hwndComboBox) {
    std::vector<std::string> baudRates = {"9600", "14400", "19200", "38400", "57600", "115200"};
    for (const auto& rate : baudRates) {
        SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)rate.c_str());
    }
}

void PopulateDataBits(HWND hwndComboBox) {
    std::vector<std::string> dataBits = {"5", "6", "7", "8"};
    for (const auto& bits : dataBits) {
        SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)bits.c_str());
    }
}

void PopulateParity(HWND hwndComboBox) {
    std::vector<std::string> parity = {"None", "Odd", "Even", "Mark", "Space"};
    for (const auto& p : parity) {
        SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)p.c_str());
    }
}

void PopulateStopBits(HWND hwndComboBox) {
    std::vector<std::string> stopBits = {"1", "1.5", "2"};
    for (const auto& bits : stopBits) {
        SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)bits.c_str());
    }
}

void SetDefaultValues() {
    SendMessage(hComboBoxBaudRate, CB_SETCURSEL, 5, 0);
    SendMessage(hComboBoxDataBits, CB_SETCURSEL, 3, 0);
    SendMessage(hComboBoxParity, CB_SETCURSEL, 0, 0);
    SendMessage(hComboBoxStopBits, CB_SETCURSEL, 0, 0);
}

void OnOpenPort(HWND hwnd) {
    if (hComm != INVALID_HANDLE_VALUE) {
        CloseHandle(hComm);
        hComm = INVALID_HANDLE_VALUE;
        SetWindowText(hStatus, "Status: Port closed");
        return;
    }

    char portName[10];
    GetWindowText(hComboBoxPort, portName, sizeof(portName));

    hComm = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (hComm == INVALID_HANDLE_VALUE) {
        SetWindowText(hStatus, "Status: Failed to open port");
        return;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hComm, &dcbSerialParams)) {
        SetWindowText(hStatus, "Status: Failed to get port settings");
        CloseHandle(hComm);
        hComm = INVALID_HANDLE_VALUE;
        return;
    }

    char baudRateStr[10];
    char dataBitsStr[10];
    char parityStr[10];
    char stopBitsStr[10];

    GetWindowText(hComboBoxBaudRate, baudRateStr, sizeof(baudRateStr));
    GetWindowText(hComboBoxDataBits, dataBitsStr, sizeof(dataBitsStr));
    GetWindowText(hComboBoxParity, parityStr, sizeof(parityStr));
    GetWindowText(hComboBoxStopBits, stopBitsStr, sizeof(stopBitsStr));

    int baudRate = atoi(baudRateStr);
    int dataBits = atoi(dataBitsStr);
    int stopBits = atoi(stopBitsStr);

    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = dataBits;
    dcbSerialParams.StopBits = stopBits == 1 ? ONESTOPBIT : stopBits == 2 ? TWOSTOPBITS : ONE5STOPBITS;
    dcbSerialParams.Parity = parityStr[0] == 'N' ? NOPARITY : parityStr[0] == 'O' ? ODDPARITY : parityStr[0] == 'E' ? EVENPARITY : parityStr[0] == 'M' ? MARKPARITY : SPACEPARITY;

    if (!SetCommState(hComm, &dcbSerialParams)) {
        SetWindowText(hStatus, "Status: Failed to set port settings");
        CloseHandle(hComm);
        hComm = INVALID_HANDLE_VALUE;
        return;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hComm, &timeouts)) {
        SetWindowText(hStatus, "Status: Failed to set port timeouts");
        CloseHandle(hComm);
        hComm = INVALID_HANDLE_VALUE;
        return;
    }

    SetWindowText(hStatus, "Status: Port opened");

    continueReading = true;
    hReadThread = CreateThread(NULL, 0, ReadFromPort, NULL, 0, NULL);
}

void OnSendCommand(HWND hwnd) {
    char command[256];
    GetWindowText(hCommandInput, command, sizeof(command));

    continueWriting = true;
    hWriteThread = CreateThread(NULL, 0, WriteToPort, command, 0, NULL);
}

DWORD WINAPI ReadFromPort(LPVOID lpParam) {
    char readBuf[256];
    DWORD bytesRead;
    OVERLAPPED osReader = {0};
    osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (osReader.hEvent == NULL) {
        SetWindowText(hStatus, "Status: Failed to create read event");
        return 1;
    }

    while (continueReading) {
        if (!ReadFile(hComm, readBuf, sizeof(readBuf), &bytesRead, &osReader)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                SetWindowText(hStatus, "Status: Read error");
                continueReading = false;
                break;
            }

            WaitForSingleObject(osReader.hEvent, INFINITE);

            if (!GetOverlappedResult(hComm, &osReader, &bytesRead, FALSE)) {
                SetWindowText(hStatus, "Status: GetOverlappedResult failed");
                continueReading = false;
                break;
            }
        }

        if (bytesRead > 0) {
            readBuf[bytesRead] = '\0';
            std::string data(readBuf);
            std::string cleanedData = RemoveAnsiCodes(data);

            std::ostringstream oss;
            for (DWORD i = 0; i < bytesRead; ++i) {
                oss << std::hex << std::uppercase << (int)readBuf[i] << " ";
            }

            SetWindowText(hDataOutput, cleanedData.c_str());
            SetWindowText(hRawDataOutput, oss.str().c_str());
        }
    }

    CloseHandle(osReader.hEvent);
    return 0;
}

DWORD WINAPI WriteToPort(LPVOID lpParam) {
    char* command = (char*)lpParam;
    DWORD bytesWritten;
    OVERLAPPED osWriter = {0};
    osWriter.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (osWriter.hEvent == NULL) {
        SetWindowText(hStatus, "Status: Failed to create write event");
        return 1;
    }

    if (!WriteFile(hComm, command, strlen(command), &bytesWritten, &osWriter)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            SetWindowText(hStatus, "Status: Write error");
            continueWriting = false;
            CloseHandle(osWriter.hEvent);
            return 1;
        }

        WaitForSingleObject(osWriter.hEvent, INFINITE);

        if (!GetOverlappedResult(hComm, &osWriter, &bytesWritten, FALSE)) {
            SetWindowText(hStatus, "Status: GetOverlappedResult failed");
            continueWriting = false;
            CloseHandle(osWriter.hEvent);
            return 1;
        }
    }

    SetWindowText(hStatus, "Status: Command sent");
    CloseHandle(osWriter.hEvent);
    return 0;
}

std::string RemoveAnsiCodes(const std::string &input) {
    std::regex ansi_regex("\\x1B\\[[0-9;]*[a-zA-Z]");
    return std::regex_replace(input, ansi_regex, "");
}
