#include <windows.h>
#include <vector>
#include <string>
#include <thread>
#include <sstream>

// Function prototypes
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

// Global variables
HANDLE hComm = INVALID_HANDLE_VALUE;
HWND hComboBoxPort, hComboBoxBaudRate, hComboBoxDataBits, hComboBoxParity, hComboBoxStopBits;
HWND hButtonOpenPort, hCommandInput, hButtonSendCommand;
HWND hDataOutput, hDataInput, hButtonSendData, hStatus;
HANDLE hReadThread = NULL;
bool continueReading = true;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    const char CLASS_NAME[] = "ComPortWindowClass";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // Create window
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "COM Port Manager",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 600,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run message loop
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    continueReading = false;
    if (hReadThread != NULL) {
        WaitForSingleObject(hReadThread, INFINITE);
        CloseHandle(hReadThread);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            // Create settings panel (left side)
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

            // Create data input/output area (right side)
            hDataOutput = CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 200, 20, 360, 400, hwnd, NULL, NULL, NULL);

            // Create status box
            hStatus = CreateWindow("STATIC", "Status: Ready", WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 480, 540, 20, hwnd, NULL, NULL, NULL);

            // Populate combo boxes with options
            PopulateComPorts(hComboBoxPort);
            PopulateBaudRates(hComboBoxBaudRate);
            PopulateDataBits(hComboBoxDataBits);
            PopulateParity(hComboBoxParity);
            PopulateStopBits(hComboBoxStopBits);

            // Set default values
            SetDefaultValues();

            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case 1: // Open Port button
                    OnOpenPort(hwnd);
                    break;
                case 2: // Send Command/Data button
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

    // Проверяем порты от COM1 до COM256
    for (int i = 1; i <= 256; ++i) {
        snprintf(portName, sizeof(portName), "\\\\.\\COM%d", i); // Форматируем порт в виде \\.\COMx
        HANDLE hComm = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hComm != INVALID_HANDLE_VALUE) {
            // Порт доступен
            CloseHandle(hComm);
            std::ostringstream oss;
            oss << "COM" << i;
            comPorts.push_back(oss.str());
        }
    }

    // Добавляем найденные порты в комбобокс
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
    hReadThread = CreateThread(NULL, 0, ReadFromPort, NULL, 0, NULL);
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

DWORD WINAPI ReadFromPort(LPVOID lpParam) {
    char buffer[256];
    DWORD bytesRead;

    while (continueReading) {
        if (ReadFile(hComm, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';

            // Replace \n with \r\n to ensure new lines are correctly displayed
            std::string output(buffer);
            size_t pos = 0;
            while ((pos = output.find('\n', pos)) != std::string::npos) {
                output.insert(pos, "\r");
                pos += 2; // Move past the newly inserted characters
            }

            SendMessage(hDataOutput, EM_REPLACESEL, 0, (LPARAM)output.c_str());
        } else {
            SetWindowText(hStatus, "Status: Error Reading from Port");
            continueReading = false;
        }
    }

    return 0;
}
