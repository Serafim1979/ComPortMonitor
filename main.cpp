#include <windows.h>
#include <iostream>

// Global variable
bool global_running = false;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

    case WM_SIZE:
        {

        }
        break;


    case WM_ACTIVATEAPP:
        {

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
