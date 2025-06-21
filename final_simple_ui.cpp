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
#define ID_COMBO_PRESETS 1007
#define ID_BUTTON_ADD_PRESET 1008
#define ID_EDIT_PRESET_NAME 1009
#define ID_EDIT_PRESET_KEYS 1010
#define ID_EDIT_KEY_DURATION 1011

// Hotkey ID
#define HOTKEY_TOGGLE 1

// Preset structure for key combinations
struct KeyPreset {
    wchar_t name[32];
    wchar_t keys[128];
};

// Global variables
HWND hMainWindow;
HWND hListBox;
HWND hRadioSingle, hRadioMulti, hRadioInfinite;
HWND hEditKeyInput, hButtonTopmost;
HWND hComboPresets, hButtonAddPreset, hEditPresetName, hEditPresetKeys;
HWND hEditKeyDuration;

bool windowVisible = true;
bool isAutoZeroRunning = false;
bool isTopmost = false;
HANDLE hAutoZeroThread = NULL;
int currentMode = 2; // Default infinite mode

// Fixed delay range for single keys (400-900ms)
const int MIN_DELAY = 400;
const int MAX_DELAY = 900;

// Shorter delay range for key sequences (30-400ms)
const int MIN_SEQUENCE_DELAY = 30;
const int MAX_SEQUENCE_DELAY = 400;

// Preset combinations
KeyPreset presets[10];
int presetCount = 0;

void AddLog(const wchar_t* logText) {
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)logText);
    int count = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    SendMessage(hListBox, LB_SETTOPINDEX, count - 1, 0);
}

// Generate random delay (fixed 400-900ms range)
int GetRandomDelay() {
    return MIN_DELAY + (rand() % (MAX_DELAY - MIN_DELAY + 1));
}

// Generate random delay for key sequences (30-400ms range)
int GetRandomSequenceDelay() {
    return MIN_SEQUENCE_DELAY + (rand() % (MAX_SEQUENCE_DELAY - MIN_SEQUENCE_DELAY + 1));
}

// Get current max key duration from UI
int GetMaxKeyDuration() {
    wchar_t durationText[10];
    GetWindowText(hEditKeyDuration, durationText, 10);
    int duration = _wtoi(durationText);

    // Validate range: minimum 10ms, maximum 2000ms
    if (duration < 10) duration = 10;
    if (duration > 2000) duration = 2000;

    return duration;
}

// Generate random key hold duration (10ms to user-defined max)
int GetRandomKeyDuration() {
    int maxDuration = GetMaxKeyDuration();
    return 10 + (rand() % (maxDuration - 10 + 1));
}

// Interruptible delay that checks running status
bool InterruptibleDelay(int delayMs) {
    const int checkInterval = 10; // Check every 10ms
    int elapsed = 0;

    while (elapsed < delayMs && isAutoZeroRunning) {
        Sleep(checkInterval);
        elapsed += checkInterval;
    }

    return isAutoZeroRunning; // Return current running status
}

// Initialize default presets
void InitializePresets() {
    presetCount = 0;

    // Preset 1: DOA = 1+2+a+3
    wcscpy(presets[presetCount].name, L"DOA");
    wcscpy(presets[presetCount].keys, L"1+2+a+3");
    presetCount++;

    // Preset 2: WASD = w+a+s+d
    wcscpy(presets[presetCount].name, L"WASD");
    wcscpy(presets[presetCount].keys, L"w+a+s+d");
    presetCount++;

    // Preset 3: NUM = 1+2+3+4+5
    wcscpy(presets[presetCount].name, L"NUM");
    wcscpy(presets[presetCount].keys, L"1+2+3+4+5");
    presetCount++;

    // Preset 4: ABC = a+b+c
    wcscpy(presets[presetCount].name, L"ABC");
    wcscpy(presets[presetCount].keys, L"a+b+c");
    presetCount++;
}

// Parse key combination string (e.g., "1+2+a+3")
void ParseKeyCombo(const wchar_t* combo, wchar_t* keys, int maxKeys, int* keyCount) {
    *keyCount = 0;
    int len = wcslen(combo);
    int keyIndex = 0;

    for (int i = 0; i < len && *keyCount < maxKeys; i++) {
        if (combo[i] != L'+' && combo[i] != L' ') {
            keys[*keyCount] = combo[i];
            (*keyCount)++;
        }
    }
}

// Update preset combo box
void UpdatePresetCombo() {
    SendMessage(hComboPresets, CB_RESETCONTENT, 0, 0);
    SendMessage(hComboPresets, CB_ADDSTRING, 0, (LPARAM)L"Select preset...");

    for (int i = 0; i < presetCount; i++) {
        wchar_t displayText[200];
        wsprintf(displayText, L"%s = %s", presets[i].name, presets[i].keys);
        SendMessage(hComboPresets, CB_ADDSTRING, 0, (LPARAM)displayText);
    }

    SendMessage(hComboPresets, CB_SETCURSEL, 0, 0);
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

    // Random key hold duration (10ms to user-defined max)
    Sleep(GetRandomKeyDuration());

    // Release
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

// Simulate key sequence (individual keys with delays)
void SimulateKeySequence(const wchar_t* combo) {
    wchar_t keys[20];
    int keyCount = 0;

    ParseKeyCombo(combo, keys, 20, &keyCount);

    if (keyCount == 0) {
        AddLog(L"[Error] No valid keys in sequence");
        return;
    }

    // Simulate each key individually with random delays
    for (int i = 0; i < keyCount && isAutoZeroRunning; i++) {
        WORD vk = GetVirtualKeyFromChar(keys[i]);
        if (vk == 0) {
            wchar_t errorMsg[100];
            wsprintf(errorMsg, L"[Error] Unsupported key: '%c'", keys[i]);
            AddLog(errorMsg);
            continue;
        }

        // Check if still running before pressing key
        if (!isAutoZeroRunning) break;

        // Simulate single key press
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;

        // Press down
        input.ki.dwFlags = 0;
        SendInput(1, &input, sizeof(INPUT));

        // Random key hold duration (10ms to user-defined max)
        Sleep(GetRandomKeyDuration());

        // Release
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));

        // Log individual key press
        wchar_t keyLog[100];
        wsprintf(keyLog, L"[Sequence] Pressed key '%c' (%d/%d)", keys[i], i + 1, keyCount);
        AddLog(keyLog);

        // Interruptible delay before next key (30-400ms), except for the last key
        if (i < keyCount - 1) {
            int delayTime = GetRandomSequenceDelay();
            if (!InterruptibleDelay(delayTime)) {
                // Stopped by global hotkey
                AddLog(L"[Sequence] Stopped by hotkey during sequence");
                break;
            }
        }
    }
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

// Check if using preset combination
bool IsUsingPresetCombo() {
    int sel = SendMessage(hComboPresets, CB_GETCURSEL, 0, 0);
    return (sel > 0 && sel <= presetCount);
}

// Get current preset combination
void GetCurrentPresetCombo(wchar_t* combo, wchar_t* name) {
    int sel = SendMessage(hComboPresets, CB_GETCURSEL, 0, 0);
    if (sel > 0 && sel <= presetCount) {
        wcscpy(combo, presets[sel - 1].keys);
        wcscpy(name, presets[sel - 1].name);
    } else {
        combo[0] = L'\0';
        name[0] = L'\0';
    }
}

// Continuous key pressing thread
DWORD WINAPI ContinuousKeyThread(LPVOID lpParam) {
    int mode = *(int*)lpParam;
    int pressCount = 0;
    int maxPresses = (mode == 1) ? 5 : -1; // Multiple mode: 5 times, infinite mode: -1

    bool usePreset = IsUsingPresetCombo();
    wchar_t keyName[100];
    wchar_t presetCombo[128];
    wchar_t presetName[32];

    if (usePreset) {
        GetCurrentPresetCombo(presetCombo, presetName);
        wsprintf(keyName, L"'%s(%s)'", presetName, presetCombo);
    } else {
        wchar_t currentKey = GetCurrentKey();
        wsprintf(keyName, L"'%c'", currentKey);
    }

    while (isAutoZeroRunning && (maxPresses == -1 || pressCount < maxPresses)) {
        if (usePreset) {
            SimulateKeySequence(presetCombo);
        } else {
            SimulateSingleKey();
        }
        pressCount++;

        // Update status
        if (mode == 0) { // Single
            wchar_t statusText[150];
            wsprintf(statusText, L"[Single] Key %s press completed", keyName);
            AddLog(statusText);
            break;
        } else if (mode == 1) { // Multiple
            wchar_t statusText[150];
            wsprintf(statusText, L"[Multi] Pressed %s %d/5 times", keyName, pressCount);
            AddLog(statusText);
        } else { // Infinite
            if (pressCount % 10 == 1 || pressCount <= 5) {
                wchar_t statusText[150];
                wsprintf(statusText, L"[Infinite] Pressed %s %d times", keyName, pressCount);
                AddLog(statusText);
            }
        }

        // If single mode, exit after one press
        if (mode == 0) {
            break;
        }

        // Interruptible delay between sequences/keys
        int delayTime = usePreset ? GetRandomDelay() : GetRandomDelay();
        if (!InterruptibleDelay(delayTime)) {
            // Stopped by global hotkey
            break;
        }
    }

    // Final status
    if (mode == 1) {
        wchar_t finalText[150];
        wsprintf(finalText, L"[Multi] 5 %s key presses completed", keyName);
        AddLog(finalText);
    } else if (mode == 2 && !isAutoZeroRunning) {
        wchar_t finalStatus[150];
        wsprintf(finalStatus, L"[Infinite] Stopped %s, total: %d presses", keyName, pressCount);
        AddLog(finalStatus);
    }

    // 不要在线程中设置isAutoZeroRunning = false，让StopKeySimulation来管理
    delete (int*)lpParam;
    return 0;
}

// Start key simulation
void StartKeySimulation() {
    if (isAutoZeroRunning) return;

    // 确保之前的线程已经清理
    if (hAutoZeroThread) {
        CloseHandle(hAutoZeroThread);
        hAutoZeroThread = NULL;
    }

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

    if (!hAutoZeroThread) {
        AddLog(L"[Error] Failed to create thread");
        isAutoZeroRunning = false;
        delete modePtr;
    }
}

// Stop key simulation
void StopKeySimulation() {
    if (!isAutoZeroRunning) return;

    isAutoZeroRunning = false;
    AddLog(L"[Stop] Key simulation stopped");

    if (hAutoZeroThread) {
        // 等待线程结束，如果超时就强制终止
        DWORD waitResult = WaitForSingleObject(hAutoZeroThread, 1000);
        if (waitResult == WAIT_TIMEOUT) {
            AddLog(L"[Warning] Thread timeout, forcing termination");
            TerminateThread(hAutoZeroThread, 0);
        }
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
                10, 10, 280, 100, hwnd, (HMENU)ID_LISTBOX, GetModuleHandle(NULL), NULL);

            // Create preset combination section
            CreateWindow(L"STATIC", L"Preset Combos:",
                WS_CHILD | WS_VISIBLE,
                10, 120, 80, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

            hComboPresets = CreateWindow(L"COMBOBOX", NULL,
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                95, 118, 150, 150, hwnd, (HMENU)ID_COMBO_PRESETS, GetModuleHandle(NULL), NULL);

            hButtonTopmost = CreateWindow(L"BUTTON", L"Topmost",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                255, 118, 50, 22, hwnd, (HMENU)ID_BUTTON_TOPMOST, GetModuleHandle(NULL), NULL);

            // Create key input section
            CreateWindow(L"STATIC", L"Single Key:",
                WS_CHILD | WS_VISIBLE,
                10, 150, 60, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

            hEditKeyInput = CreateWindow(L"EDIT", L"0",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER,
                75, 148, 30, 22, hwnd, (HMENU)ID_EDIT_KEY_INPUT, GetModuleHandle(NULL), NULL);

            // Create custom preset section
            CreateWindow(L"STATIC", L"Name:",
                WS_CHILD | WS_VISIBLE,
                115, 150, 35, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

            hEditPresetName = CreateWindow(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                155, 148, 40, 22, hwnd, (HMENU)ID_EDIT_PRESET_NAME, GetModuleHandle(NULL), NULL);

            CreateWindow(L"STATIC", L"Keys:",
                WS_CHILD | WS_VISIBLE,
                205, 150, 30, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

            hEditPresetKeys = CreateWindow(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                240, 148, 50, 22, hwnd, (HMENU)ID_EDIT_PRESET_KEYS, GetModuleHandle(NULL), NULL);

            hButtonAddPreset = CreateWindow(L"BUTTON", L"Add",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                295, 148, 30, 22, hwnd, (HMENU)ID_BUTTON_ADD_PRESET, GetModuleHandle(NULL), NULL);

            // Create radio button group
            hRadioSingle = CreateWindow(L"BUTTON", L"1x",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                20, 180, 50, 25, hwnd, (HMENU)ID_RADIO_SINGLE, GetModuleHandle(NULL), NULL);

            hRadioMulti = CreateWindow(L"BUTTON", L"Multi",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                90, 180, 50, 25, hwnd, (HMENU)ID_RADIO_MULTI, GetModuleHandle(NULL), NULL);

            hRadioInfinite = CreateWindow(L"BUTTON", L"Infinite",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                160, 180, 70, 25, hwnd, (HMENU)ID_RADIO_INFINITE, GetModuleHandle(NULL), NULL);

            // Default select "Infinite"
            SendMessage(hRadioInfinite, BM_SETCHECK, BST_CHECKED, 0);

            // Create key duration setting
            CreateWindow(L"STATIC", L"Key Duration (10-2000ms):",
                WS_CHILD | WS_VISIBLE,
                20, 215, 120, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

            hEditKeyDuration = CreateWindow(L"EDIT", L"500",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                145, 213, 50, 22, hwnd, (HMENU)ID_EDIT_KEY_DURATION, GetModuleHandle(NULL), NULL);

            CreateWindow(L"STATIC", L"ms",
                WS_CHILD | WS_VISIBLE,
                200, 215, 20, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

            // Create delay info (static display)
            CreateWindow(L"STATIC", L"Single: 400-900ms, Sequence: 30-400ms",
                WS_CHILD | WS_VISIBLE,
                20, 240, 220, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

            // Initialize presets and update combo
            InitializePresets();
            UpdatePresetCombo();

            // Add initial instructions
            AddLog(L"=== Enhanced Key Simulator with Sequences ===");
            AddLog(L"Press ~ key to start/stop key simulation");
            AddLog(L"Use preset sequences or single key input");
            AddLog(L"Sequences: Each key pressed individually with delays");
            AddLog(L"Single key: 400-900ms, Sequences: 30-400ms");
            AddLog(L"Key hold duration: 10ms to user-defined max (default 500ms)");
            AddLog(L"~ key toggles start/stop at any time");
            AddLog(L"Select mode: 1x/Multi/Infinite");
            AddLog(L"Add custom presets: Name + Keys (e.g., a+b+c)");
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

        case ID_COMBO_PRESETS:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                int sel = SendMessage(hComboPresets, CB_GETCURSEL, 0, 0);
                if (sel > 0 && sel <= presetCount) {
                    wchar_t logText[200];
                    wsprintf(logText, L"[Preset] Selected: %s = %s",
                             presets[sel - 1].name, presets[sel - 1].keys);
                    AddLog(logText);
                } else {
                    AddLog(L"[Preset] Cleared selection");
                }
            }
            break;

        case ID_BUTTON_ADD_PRESET:
            {
                wchar_t name[32], keys[128];
                GetWindowText(hEditPresetName, name, 32);
                GetWindowText(hEditPresetKeys, keys, 128);

                if (wcslen(name) > 0 && wcslen(keys) > 0 && presetCount < 10) {
                    wcscpy(presets[presetCount].name, name);
                    wcscpy(presets[presetCount].keys, keys);
                    presetCount++;

                    UpdatePresetCombo();
                    SetWindowText(hEditPresetName, L"");
                    SetWindowText(hEditPresetKeys, L"");

                    wchar_t logText[200];
                    wsprintf(logText, L"[Preset] Added: %s = %s", name, keys);
                    AddLog(logText);
                } else {
                    AddLog(L"[Error] Invalid preset or limit reached (max 10)");
                }
            }
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

        case ID_EDIT_KEY_DURATION:
            if (HIWORD(wParam) == EN_CHANGE) {
                // Key duration changed
                wchar_t durationText[10];
                GetWindowText(hEditKeyDuration, durationText, 10);
                int duration = _wtoi(durationText);
                if (duration >= 10 && duration <= 2000) {
                    wchar_t logText[100];
                    wsprintf(logText, L"[Duration] Key hold time: 10-%dms", duration);
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
            SetWindowPos(hListBox, NULL, 10, 10, rect.right - 20, 100, SWP_NOZORDER);
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
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 350, 320,
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
