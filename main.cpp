#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>

// Global variable
bool global_running = false;
HANDLE hComm = INVALID_HANDLE_VALUE;
HWND hComboBoxPort, hComboBoxBaudRate, hComboBoxDataBits, hComboBoxParity, hComboBoxStopBits;
HWND hButtonOpenPort, hCommandInput, hButtonSendCommand;
HWND hDataOutput, hDataInput, hButtonSendData, hStatus;
bool continueReading = true;
HANDLE hThread;


// Function prototypes
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void PopulateComPorts(HWND hwndComboBox);
void PopulateBaudRates(HWND hwndComboBox);
void PopulateDataBits(HWND hwndComboBox);
void PopulateParity(HWND hwndComboBox);
void PopulateStopBits(HWND hwndComboBox);
void SetDefaultValues();
void OnOpenPort(HWND hwnd);
void OnSendCommand(HWND hwnd);
void OnSendData(HWND hwnd);
void ReadFromPort();



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = WndProc;
    WindowClass.lpszClassName = "ComPortMonitor";
    WindowClass.hInstance = hInstance;

    if(RegisterClassA(&WindowClass))
    {
        HWND WindowHandle = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Com Port Terminal",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            0, 0, hInstance, 0
        );

        if(WindowHandle)
        {
            MSG Message;
            global_running = true;
            while(global_running)
            {
                while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
            }
        }
        else
        {
            // TODO: Logging
        }
    }
    else
    {
        // TODO: Logging
    }

    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;
    switch(uMsg)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        {
            global_running = false;
        }
        break;

    case WM_CREATE:
        {
            // Create settings panel (left side)
            CreateWindow("STATIC", "Port Settings", WS_CHILD | WS_VISIBLE, 20, 20, 150, 20, hWnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "COM Port:", WS_CHILD | WS_VISIBLE, 20, 50, 150, 20, hWnd, NULL, NULL, NULL);
            hComboBoxPort = CreateWindow("COMBOBOX", NULL, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE | WS_VSCROLL, 20, 70, 150, 200, hWnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Baud Rate:", WS_CHILD | WS_VISIBLE, 20, 110, 150, 20, hWnd, NULL, NULL, NULL);
            hComboBoxBaudRate = CreateWindow("COMBOBOX", NULL, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE | WS_VSCROLL, 20, 130, 150, 200, hWnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Data Bits:", WS_CHILD | WS_VISIBLE, 20, 170, 150, 20, hWnd, NULL, NULL, NULL);
            hComboBoxDataBits = CreateWindow("COMBOBOX", NULL, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE | WS_VSCROLL, 20, 190, 150, 200, hWnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Parity:", WS_CHILD | WS_VISIBLE, 20, 230, 150, 20, hWnd, NULL, NULL, NULL);
            hComboBoxParity = CreateWindow("COMBOBOX", NULL, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE | WS_VSCROLL, 20, 250, 150, 200, hWnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Stop Bits:", WS_CHILD | WS_VISIBLE, 20, 290, 150, 20, hWnd, NULL, NULL, NULL);
            hComboBoxStopBits = CreateWindow("COMBOBOX", NULL, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE | WS_VSCROLL, 20, 310, 150, 200, hWnd, NULL, NULL, NULL);

            hButtonOpenPort = CreateWindow("BUTTON", "Open Port", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 20, 350, 150, 30, hWnd, (HMENU)1, NULL, NULL);

            CreateWindow("STATIC", "Command/Data Input:", WS_CHILD | WS_VISIBLE, 20, 390, 150, 20, hWnd, NULL, NULL, NULL);
            hCommandInput = CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 20, 410, 150, 20, hWnd, NULL, NULL, NULL);
            hButtonSendCommand = CreateWindow("BUTTON", "Send Command/Data", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 20, 440, 150, 30, hWnd, (HMENU)2, NULL, NULL);

            // Create data input/output area (right side)
            hDataOutput = CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 200, 20, 360, 400, hWnd, NULL, NULL, NULL);

            // Create status box
            hStatus = CreateWindow("STATIC", "Status: Ready", WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 480, 540, 20, hWnd, NULL, NULL, NULL);

            // Populate combo boxes with options
            PopulateComPorts(hComboBoxPort);
            PopulateBaudRates(hComboBoxBaudRate);
            PopulateDataBits(hComboBoxDataBits);
            PopulateParity(hComboBoxParity);
            PopulateStopBits(hComboBoxStopBits);

            // Set default values
            SetDefaultValues();

            return 0;
        }
        break;

    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case 1: // Open Port button
                    OnOpenPort(hWnd);
                    break;
                case 2: // Send Command/Data button
                    OnSendCommand(hWnd);
                    break;
            }
        }
        break;

    case WM_SIZE:
        {

        }
        break;


    case WM_ACTIVATEAPP:
        {

        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(hWnd, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;

            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            static DWORD Operation = WHITENESS;

            PatBlt(DeviceContext, X, Y, Width, Height, Operation);

            if(Operation == WHITENESS)
            {
                Operation = BLACKNESS;
            }
            else
            {
                Operation = WHITENESS;
            }

            EndPaint(hWnd, &Paint);
        }
        break;

    default:
        {
            Result = DefWindowProcA(hWnd, uMsg, wParam, lParam);
        }
        break;
    }
    return Result;
}


void PopulateComPorts(HWND hwndComboBox) {
    // Example COM ports
    std::vector<std::string> comPorts = {"COM1", "COM2", "COM3", "COM4"};
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
    // Set default selections for each combo box
    SendMessage(hComboBoxPort, CB_SETCURSEL, 0, 0); // Default to COM1
    SendMessage(hComboBoxBaudRate, CB_SETCURSEL, 0, 0); // Default to 9600
    SendMessage(hComboBoxDataBits, CB_SETCURSEL, 3, 0); // Default to 8 data bits
    SendMessage(hComboBoxParity, CB_SETCURSEL, 0, 0); // Default to No Parity
    SendMessage(hComboBoxStopBits, CB_SETCURSEL, 0, 0); // Default to 1 Stop Bit
}

void OnOpenPort(HWND hwnd) {
    char portName[256], baudRate[256], dataBits[256], parity[256], stopBits[256];

    // Get settings from combo boxes
    GetWindowText(hComboBoxPort, portName, sizeof(portName));
    GetWindowText(hComboBoxBaudRate, baudRate, sizeof(baudRate));
    GetWindowText(hComboBoxDataBits, dataBits, sizeof(dataBits));
    GetWindowText(hComboBoxParity, parity, sizeof(parity));
    GetWindowText(hComboBoxStopBits, stopBits, sizeof(stopBits));

    // Open and configure the COM port
    hComm = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hComm == INVALID_HANDLE_VALUE) {
        SetWindowText(hStatus, "Status: Error Opening Port");
        return;
    }

    DCB dcb;
    ZeroMemory(&dcb, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(hComm, &dcb)) {
        SetWindowText(hStatus, "Status: Error Getting Port State");
        return;
    }

    dcb.BaudRate = std::stoi(baudRate);
    dcb.ByteSize = std::stoi(dataBits);
    dcb.Parity = (parity == std::string("None")) ? NOPARITY : ((parity == std::string("Odd")) ? ODDPARITY : ((parity == std::string("Even")) ? EVENPARITY : ((parity == std::string("Mark")) ? MARKPARITY : SPACEPARITY)));
    dcb.StopBits = (stopBits == std::string("1")) ? ONESTOPBIT : ((stopBits == std::string("1.5")) ? ONE5STOPBITS : TWOSTOPBITS);

    if (!SetCommState(hComm, &dcb)) {
        SetWindowText(hStatus, "Status: Error Setting Port State");
        CloseHandle(hComm);
        return;
    }

    SetWindowText(hStatus, "Status: Port Opened and Configured");

    // Start the thread to read from the port
    continueReading = true;
    readThread = std::thread(ReadFromPort);
}

void OnSendCommand(HWND hwnd) {
    if (hComm == INVALID_HANDLE_VALUE) {
        SetWindowText(hStatus, "Status: Port Not Open");
        return;
    }

    // Get the command/data from the input field
    char command[256];
    GetWindowText(hCommandInput, command, sizeof(command));
    DWORD bytesWritten;

    // Send the command/data to the device
    if (!WriteFile(hComm, command, strlen(command), &bytesWritten, NULL)) {
        SetWindowText(hStatus, "Status: Error Sending Command/Data");
    } else {
        SetWindowText(hStatus, "Status: Command/Data Sent");
    }
}

void ReadFromPort() {
    char buffer[256];
    DWORD bytesRead;

    while (continueReading) {
        if (ReadFile(hComm, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            buffer[bytesRead] = '\0';
            SendMessage(hDataOutput, EM_REPLACESEL, 0, (LPARAM)buffer);
        } else {
            SetWindowText(hStatus, "Status: Error Reading from Port");
            continueReading = false;
        }
    }
}
