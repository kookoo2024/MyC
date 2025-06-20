#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <cstdlib>
#include <ctime>

// Control IDs
#define ID_LISTBOX 1001
#define ID_RADIO_SINGLE 1002
#define ID_RADIO_MULTI 1003
#define ID_RADIO_INFINITE 1004
#define ID_EDIT_KEY_INPUT 1005
#define ID_BUTTON_TOPMOST 1006

// Hotkey ID
#define HOTKEY_TOGGLE 1

// Global variables
HWND hMainWindow;
HWND hListBox;
HWND hRadioSingle, hRadioMulti, hRadioInfinite;
HWND hEditKeyInput, hButtonTopmost;

bool windowVisible = true;
bool isAutoZeroRunning = false;
bool isTopmost = false;
HANDLE hAutoZeroThread = NULL;
int currentMode = 2; // Default infinite mode

// Fixed delay range (400-900ms)
const int MIN_DELAY = 400;
const int MAX_DELAY = 900;

void AddLog(const wchar_t* logText) {
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)logText);
    int count = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    SendMessage(hListBox, LB_SETTOPINDEX, count - 1, 0);
}

// Generate random delay (fixed 400-900ms range)
int GetRandomDelay() {
    return MIN_DELAY + (rand() % (MAX_DELAY - MIN_DELAY + 1));
}

// Get virtual key code from character
WORD GetVirtualKeyFromChar(wchar_t ch) {
    if (ch >= L'A' && ch <= L'Z') {
        return ch;
    } else if (ch >= L'a' && ch <= L'z') {
        return ch - L'a' + L'A';
    } else if (ch >= L'0' && ch <= L'9') {
        return ch;
    }

    // Special characters
    switch (ch) {
        case L' ': return VK_SPACE;
        case L'.': return VK_OEM_PERIOD;
        case L',': return VK_OEM_COMMA;
        case L';': return VK_OEM_1;
        case L'/': return VK_OEM_2;
        case L'`': return VK_OEM_3;
        case L'[': return VK_OEM_4;
        case L'\\': return VK_OEM_5;
        case L']': return VK_OEM_6;
        case L'\'': return VK_OEM_7;
        case L'-': return VK_OEM_MINUS;
        case L'=': return VK_OEM_PLUS;
        default: return 0;
    }
}

// Get current key to simulate
wchar_t GetCurrentKey() {
    wchar_t keyText[10];
    GetWindowText(hEditKeyInput, keyText, 10);
    if (wcslen(keyText) > 0) {
        return keyText[0]; // Return first character
    }
    return L'0'; // Default to '0'
}

// Simulate single key press
void SimulateSingleKey() {
    wchar_t key = GetCurrentKey();
    WORD vk = GetVirtualKeyFromChar(key);

    if (vk == 0) {
        AddLog(L"[Error] Unsupported key character");
        return;
    }

    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;

    // Press down
    input.ki.dwFlags = 0;
    SendInput(1, &input, sizeof(INPUT));

    // Random key hold duration (50-120ms)
    Sleep(50 + (rand() % 70));

    // Release
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

// Get current selected mode
int GetSelectedMode() {
    if (SendMessage(hRadioSingle, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        return 0; // Single
    } else if (SendMessage(hRadioMulti, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        return 1; // Multiple
    } else if (SendMessage(hRadioInfinite, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        return 2; // Infinite
    }
    return 2; // Default infinite
}

// Continuous key pressing thread
DWORD WINAPI ContinuousKeyThread(LPVOID lpParam) {
    int mode = *(int*)lpParam;
    int pressCount = 0;
    int maxPresses = (mode == 1) ? 5 : -1; // Multiple mode: 5 times, infinite mode: -1

    wchar_t currentKey = GetCurrentKey();
    wchar_t keyName[50];
    wsprintf(keyName, L"'%c'", currentKey);

    while (isAutoZeroRunning && (maxPresses == -1 || pressCount < maxPresses)) {
        SimulateSingleKey();
        pressCount++;

        // Update status
        if (mode == 0) { // Single
            wchar_t statusText[100];
            wsprintf(statusText, L"[Single] Key %s press completed", keyName);
            AddLog(statusText);
            break;
        } else if (mode == 1) { // Multiple
            wchar_t statusText[100];
            wsprintf(statusText, L"[Multi] Pressed %s %d/5 times", keyName, pressCount);
            AddLog(statusText);
        } else { // Infinite
            if (pressCount % 10 == 1 || pressCount <= 5) {
                wchar_t statusText[100];
                wsprintf(statusText, L"[Infinite] Pressed %s %d times", keyName, pressCount);
                AddLog(statusText);
            }
        }

        // If single mode, exit after one press
        if (mode == 0) {
            break;
        }

        // Random delay
        Sleep(GetRandomDelay());
    }

    // Final status
    if (mode == 1) {
        wchar_t finalText[100];
        wsprintf(finalText, L"[Multi] 5 %s key presses completed", keyName);
        AddLog(finalText);
    } else if (mode == 2 && !isAutoZeroRunning) {
        wchar_t finalStatus[100];
        wsprintf(finalStatus, L"[Infinite] Stopped %s, total: %d presses", keyName, pressCount);
        AddLog(finalStatus);
    }

    isAutoZeroRunning = false;
    delete (int*)lpParam;
    return 0;
}

// Start key simulation
void StartKeySimulation() {
    if (isAutoZeroRunning) return;
    
    currentMode = GetSelectedMode();
    isAutoZeroRunning = true;
    
    wchar_t startMsg[100];
    if (currentMode == 0) {
        wcscpy(startMsg, L"[Single] Starting key press...");
    } else if (currentMode == 1) {
        wcscpy(startMsg, L"[Multi] Starting 5 key presses...");
    } else {
        wcscpy(startMsg, L"[Infinite] Starting continuous key presses...");
    }
    AddLog(startMsg);
    
    int* modePtr = new int(currentMode);
    hAutoZeroThread = CreateThread(NULL, 0, ContinuousKeyThread, modePtr, 0, NULL);
}

// Stop key simulation
void StopKeySimulation() {
    if (!isAutoZeroRunning) return;
    
    isAutoZeroRunning = false;
    AddLog(L"[Stop] Key simulation stopped");
    
    if (hAutoZeroThread) {
        WaitForSingleObject(hAutoZeroThread, 2000);
        CloseHandle(hAutoZeroThread);
        hAutoZeroThread = NULL;
    }
}

// Toggle key simulation
void ToggleKeySimulation() {
    if (isAutoZeroRunning) {
        StopKeySimulation();
    } else {
        StartKeySimulation();
    }
}

void ToggleWindow() {
    if (windowVisible) {
        ShowWindow(hMainWindow, SW_HIDE);
        windowVisible = false;
    } else {
        ShowWindow(hMainWindow, SW_SHOW);
        SetForegroundWindow(hMainWindow);
        windowVisible = true;
    }
}

// Toggle window topmost
void ToggleTopmost() {
    if (isTopmost) {
        SetWindowPos(hMainWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowText(hButtonTopmost, L"Set Topmost");
        AddLog(L"[Topmost] Window topmost disabled");
        isTopmost = false;
    } else {
        SetWindowPos(hMainWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowText(hButtonTopmost, L"Cancel Topmost");
        AddLog(L"[Topmost] Window topmost enabled");
        isTopmost = true;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        {
            hMainWindow = hwnd;
            
            if (!RegisterHotKey(hwnd, HOTKEY_TOGGLE, 0, VK_OEM_3)) {
                MessageBox(hwnd, L"Failed to register global hotkey!", L"Error", MB_OK | MB_ICONERROR);
            }

            // Create listbox (upper half)
            hListBox = CreateWindow(L"LISTBOX", NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_NOTIFY,
                10, 10, 280, 120, hwnd, (HMENU)ID_LISTBOX, GetModuleHandle(NULL), NULL);

            // Create key input section
            CreateWindow(L"STATIC", L"Key to simulate:",
                WS_CHILD | WS_VISIBLE,
                20, 140, 80, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

            hEditKeyInput = CreateWindow(L"EDIT", L"0",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER,
                110, 138, 30, 22, hwnd, (HMENU)ID_EDIT_KEY_INPUT, GetModuleHandle(NULL), NULL);

            hButtonTopmost = CreateWindow(L"BUTTON", L"Set Topmost",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                160, 138, 80, 22, hwnd, (HMENU)ID_BUTTON_TOPMOST, GetModuleHandle(NULL), NULL);

            // Create radio button group
            hRadioSingle = CreateWindow(L"BUTTON", L"1x",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                20, 170, 50, 25, hwnd, (HMENU)ID_RADIO_SINGLE, GetModuleHandle(NULL), NULL);

            hRadioMulti = CreateWindow(L"BUTTON", L"Multi",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                90, 170, 50, 25, hwnd, (HMENU)ID_RADIO_MULTI, GetModuleHandle(NULL), NULL);

            hRadioInfinite = CreateWindow(L"BUTTON", L"Infinite",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                160, 170, 70, 25, hwnd, (HMENU)ID_RADIO_INFINITE, GetModuleHandle(NULL), NULL);

            // Default select "Infinite"
            SendMessage(hRadioInfinite, BM_SETCHECK, BST_CHECKED, 0);

            // Create delay info (static display)
            CreateWindow(L"STATIC", L"Key interval: 400-900ms (random)",
                WS_CHILD | WS_VISIBLE,
                20, 205, 200, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

            // Add initial instructions
            AddLog(L"=== Enhanced Key Simulator ===");
            AddLog(L"Press ~ key to start/stop key simulation");
            AddLog(L"Enter key to simulate in input box");
            AddLog(L"Select mode: 1x/Multi/Infinite");
            AddLog(L"Fixed key interval: 400-900ms (random)");
            AddLog(L"Use 'Set Topmost' to keep window on top");
            AddLog(L"Ready, waiting for hotkey trigger...");
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_RADIO_SINGLE:
        case ID_RADIO_MULTI:
        case ID_RADIO_INFINITE:
            {
                // If running, stop first
                if (isAutoZeroRunning) {
                    StopKeySimulation();
                }

                // Update mode hint
                int mode = GetSelectedMode();
                if (mode == 0) {
                    AddLog(L"[Mode] Selected: Single mode");
                } else if (mode == 1) {
                    AddLog(L"[Mode] Selected: Multi mode (5 times)");
                } else {
                    AddLog(L"[Mode] Selected: Infinite mode");
                }
            }
            break;

        case ID_BUTTON_TOPMOST:
            ToggleTopmost();
            break;

        case ID_EDIT_KEY_INPUT:
            if (HIWORD(wParam) == EN_CHANGE) {
                // Key input changed
                wchar_t keyText[10];
                GetWindowText(hEditKeyInput, keyText, 10);
                if (wcslen(keyText) > 0) {
                    wchar_t logText[100];
                    wsprintf(logText, L"[Key] Changed to: '%c'", keyText[0]);
                    AddLog(logText);
                }
            }
            break;

        case ID_LISTBOX:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                // Handle listbox selection event here if needed
            }
            break;
        }
        break;

    case WM_HOTKEY:
        if (wParam == HOTKEY_TOGGLE) {
            AddLog(L"[Hotkey] ~ key pressed");
            ToggleKeySimulation();
        }
        break;

    case WM_SIZE:
        {
            RECT rect;
            GetClientRect(hwnd, &rect);
            // Adjust listbox size
            SetWindowPos(hListBox, NULL, 10, 10, rect.right - 20, 120, SWP_NOZORDER);
        }
        break;

    case WM_DESTROY:
        if (isAutoZeroRunning) {
            StopKeySimulation();
        }
        UnregisterHotKey(hwnd, HOTKEY_TOGGLE);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
        }
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize random seed
    srand((unsigned int)GetTickCount());

    const wchar_t CLASS_NAME[] = L"FinalSimpleUIKeyboardSimulator";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"MN",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 320, 260,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
