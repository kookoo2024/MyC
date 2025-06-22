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
#include <vector> // For std::vector
#include <vector> // For std::vector

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

    // User-requested presets
    // C-C = ctrl+c
    wcscpy(presets[presetCount].name, L"C-C");
    wcscpy(presets[presetCount].keys, L"ctrl+c");
    presetCount++;

    // C-V = ctrl+v
    wcscpy(presets[presetCount].name, L"C-V");
    wcscpy(presets[presetCount].keys, L"ctrl+v");
    presetCount++;

    // B-1 = 0+0+0+tab
    wcscpy(presets[presetCount].name, L"B-1");
    wcscpy(presets[presetCount].keys, L"0+0+0+tab");
    presetCount++;
}

// Parse key combination string (e.g., "1+2+a+3")
void ParseKeyCombo(const wchar_t* combo, wchar_t* keys, int maxKeys, int* keyCount) {
    *keyCount = 0;
    int len = wcslen(combo);
    int i = 0;

    while (i < len && *keyCount < maxKeys) {
        // Skip delimiters
        while (i < len && (combo[i] == L'+' || combo[i] == L' ')) {
            i++;
        }
        if (i >= len) break;

        // Check for multi-character key names
        if (wcsncmp(&combo[i], L"ctrl", 4) == 0) {
            keys[*keyCount] = L'c'; // Internal representation for VK_CONTROL
            i += 4;
        } else if (wcsncmp(&combo[i], L"alt", 3) == 0) {
            keys[*keyCount] = L'a'; // Internal representation for VK_MENU
            i += 3;
        } else if (wcsncmp(&combo[i], L"tab", 3) == 0) {
            keys[*keyCount] = L't'; // Internal representation for VK_TAB
            i += 3;
        } else if (wcsncmp(&combo[i], L"space", 5) == 0) {
            keys[*keyCount] = L' '; // Internal representation for VK_SPACE
            i += 5;
        } else {
            // Single character key
            keys[*keyCount] = combo[i];
            i++;
        }
        (*keyCount)++;
    }
}

// Update preset combo box
void UpdatePresetCombo() {
    SendMessage(hComboPresets, CB_RESETCONTENT, 0, 0);
    SendMessage(hComboPresets, CB_ADDSTRING, 0, (LPARAM)L"选择预设...");

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
        case L' ': return VK_SPACE;
        case L't': return VK_TAB;
        case L'c': return VK_CONTROL;
        case L'a': return VK_MENU;
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
        AddLog(L"[错误] 不支持的按键字符");
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

// Simulate key sequence (individual keys with delays, handles modifiers)
void SimulateKeySequence(const wchar_t* combo) {
    wchar_t parsedKeys[20];
    int keyCount = 0;

    ParseKeyCombo(combo, parsedKeys, 20, &keyCount);

    if (keyCount == 0) {
        AddLog(L"[错误] 序列中没有有效按键");
        return;
    }

    std::vector<WORD> modifierKeys;
    std::vector<WORD> regularKeys;

    // Separate modifier keys from regular keys
    for (int i = 0; i < keyCount; i++) {
        WORD vk = GetVirtualKeyFromChar(parsedKeys[i]);
        if (vk == VK_CONTROL || vk == VK_MENU || vk == VK_SHIFT) { // VK_MENU is Alt
            modifierKeys.push_back(vk);
        } else {
            regularKeys.push_back(vk);
        }
    }

    // Press down all modifier keys
    for (WORD vk : modifierKeys) {
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;
        input.ki.dwFlags = 0; // Key down
        SendInput(1, &input, sizeof(INPUT));
        wchar_t logMsg[100];
        if (vk == VK_CONTROL) wsprintf(logMsg, L"[序列] 按下修饰键: Ctrl");
        else if (vk == VK_MENU) wsprintf(logMsg, L"[序列] 按下修饰键: Alt");
        else if (vk == VK_SHIFT) wsprintf(logMsg, L"[序列] 按下修饰键: Shift");
        else wsprintf(logMsg, L"[序列] 按下修饰键: %d", vk); // Fallback for other modifiers
        AddLog(logMsg);
    }

    // Simulate regular keys with delays
    for (size_t i = 0; i < regularKeys.size() && isAutoZeroRunning; i++) {
        WORD vk = regularKeys[i];
        if (vk == 0) {
            wchar_t errorMsg[100];
            // For error, try to show original character if possible, or a generic error
            wsprintf(errorMsg, L"[错误] 不支持的按键: '%c'", parsedKeys[i]);
            AddLog(errorMsg);
            continue;
        }

        if (!isAutoZeroRunning) break;

        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;

        // Press down
        input.ki.dwFlags = 0;
        SendInput(1, &input, sizeof(INPUT));

        // Random key hold duration
        Sleep(GetRandomKeyDuration());

        // Release
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));

        wchar_t keyLog[100];
        // Improved logging for regular keys
        if (vk == VK_TAB) wsprintf(keyLog, L"[序列] 按下按键 'Tab' (%d/%d)", (int)i + 1, (int)regularKeys.size());
        else if (vk == VK_SPACE) wsprintf(keyLog, L"[序列] 按下按键 'Space' (%d/%d)", (int)i + 1, (int)regularKeys.size());
        else wsprintf(keyLog, L"[序列] 按下按键 '%c' (%d/%d)", parsedKeys[i], (int)i + 1, (int)regularKeys.size());
        AddLog(keyLog);

        // Interruptible delay before next key (30-400ms), except for the last key
        if (i < regularKeys.size() - 1) {
            int delayTime = GetRandomSequenceDelay();
            if (!InterruptibleDelay(delayTime)) {
                AddLog(L"[序列] 在序列中被热键停止");
                break;
            }
        }
    }

    // Release all modifier keys
    for (WORD vk : modifierKeys) {
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;
        input.ki.dwFlags = KEYEVENTF_KEYUP; // Key up
        SendInput(1, &input, sizeof(INPUT));
        wchar_t logMsg[100];
        // Improved logging for modifier keys release
        if (vk == VK_CONTROL) wsprintf(logMsg, L"[序列] 释放修饰键: Ctrl");
        else if (vk == VK_MENU) wsprintf(logMsg, L"[序列] 释放修饰键: Alt");
        else if (vk == VK_SHIFT) wsprintf(logMsg, L"[序列] 释放修饰键: Shift");
        else wsprintf(logMsg, L"[序列] 释放修饰键: %d", vk); // Fallback
        AddLog(logMsg);
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
            wsprintf(statusText, L"[单次] 按键 %s 完成", keyName);
            AddLog(statusText);
            break;
        } else if (mode == 1) { // Multiple
            wchar_t statusText[150];
            wsprintf(statusText, L"[多次] 按下 %s %d/5 次", keyName, pressCount);
            AddLog(statusText);
        } else { // Infinite
            if (pressCount % 10 == 1 || pressCount <= 5) {
                wchar_t statusText[150];
                wsprintf(statusText, L"[无限] 按下 %s %d 次", keyName, pressCount);
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
        wsprintf(finalText, L"[多次] 5 次 %s 按键完成", keyName);
        AddLog(finalText);
    } else if (mode == 2 && !isAutoZeroRunning) {
        wchar_t finalStatus[150];
        wsprintf(finalStatus, L"[无限] 停止 %s, 总计: %d 次按键", keyName, pressCount);
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
        wcscpy(startMsg, L"[单次] 开始按键...");
    } else if (currentMode == 1) {
        wcscpy(startMsg, L"[多次] 开始 5 次按键...");
    } else {
        wcscpy(startMsg, L"[无限] 开始连续按键...");
    }
    AddLog(startMsg);

    int* modePtr = new int(currentMode);
    hAutoZeroThread = CreateThread(NULL, 0, ContinuousKeyThread, modePtr, 0, NULL);

    if (!hAutoZeroThread) {
        AddLog(L"[错误] 创建线程失败");
        isAutoZeroRunning = false;
        delete modePtr;
    }
}

// Stop key simulation
void StopKeySimulation() {
    if (!isAutoZeroRunning) return;

    isAutoZeroRunning = false;
    AddLog(L"[停止] 按键模拟已停止");

    if (hAutoZeroThread) {
        // 等待线程结束，如果超时就强制终止
        DWORD waitResult = WaitForSingleObject(hAutoZeroThread, 1000);
        if (waitResult == WAIT_TIMEOUT) {
            AddLog(L"[警告] 线程超时, 强制终止");
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
        SetWindowText(hButtonTopmost, L"设为置顶");
        AddLog(L"[置顶] 窗口置顶已禁用");
        isTopmost = false;
    } else {
        SetWindowPos(hMainWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowText(hButtonTopmost, L"取消置顶");
        AddLog(L"[置顶] 窗口置顶已启用");
        isTopmost = true;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        {
            hMainWindow = hwnd;
            
            if (!RegisterHotKey(hwnd, HOTKEY_TOGGLE, 0, VK_OEM_3)) {
                MessageBox(hwnd, L"注册全局热键失败!", L"错误", MB_OK | MB_ICONERROR);
            }

            // Log Area
            CreateWindow(L"STATIC", L"运行时日志:",
                WS_CHILD | WS_VISIBLE,
                10, 10, 80, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            hListBox = CreateWindow(L"LISTBOX", NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_NOTIFY,
                10, 30, 360, 80, hwnd, (HMENU)ID_LISTBOX, GetModuleHandle(NULL), NULL);

            // Control Panel
            CreateWindow(L"BUTTON", L"控制面板",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                10, 120, 360, 80, hwnd, NULL, GetModuleHandle(NULL), NULL);

            // First row: Preset and Topmost
            CreateWindow(L"STATIC", L"预设:",
                WS_CHILD | WS_VISIBLE,
                20, 140, 40, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            hComboPresets = CreateWindow(L"COMBOBOX", NULL,
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                65, 138, 220, 150, hwnd, (HMENU)ID_COMBO_PRESETS, GetModuleHandle(NULL), NULL);
            hButtonTopmost = CreateWindow(L"BUTTON", L"置顶",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                290, 138, 60, 22, hwnd, (HMENU)ID_BUTTON_TOPMOST, GetModuleHandle(NULL), NULL);

            // Second row: Single key and mode
            CreateWindow(L"STATIC", L"按键:",
                WS_CHILD | WS_VISIBLE,
                20, 170, 40, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            hEditKeyInput = CreateWindow(L"EDIT", L"0",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER,
                65, 168, 50, 22, hwnd, (HMENU)ID_EDIT_KEY_INPUT, GetModuleHandle(NULL), NULL);

            CreateWindow(L"STATIC", L"模式:",
                WS_CHILD | WS_VISIBLE,
                125, 170, 40, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            hRadioSingle = CreateWindow(L"BUTTON", L"单次",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                165, 168, 55, 22, hwnd, (HMENU)ID_RADIO_SINGLE, GetModuleHandle(NULL), NULL);
            hRadioMulti = CreateWindow(L"BUTTON", L"5次",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                220, 168, 45, 22, hwnd, (HMENU)ID_RADIO_MULTI, GetModuleHandle(NULL), NULL);
            hRadioInfinite = CreateWindow(L"BUTTON", L"无限",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                270, 168, 55, 22, hwnd, (HMENU)ID_RADIO_INFINITE, GetModuleHandle(NULL), NULL);
            SendMessage(hRadioInfinite, BM_SETCHECK, BST_CHECKED, 0);

            // Custom Preset Area
            CreateWindow(L"BUTTON", L"自定义预设",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                10, 210, 360, 50, hwnd, NULL, GetModuleHandle(NULL), NULL);
            CreateWindow(L"STATIC", L"名称:",
                WS_CHILD | WS_VISIBLE,
                20, 230, 40, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            hEditPresetName = CreateWindow(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                65, 228, 60, 22, hwnd, (HMENU)ID_EDIT_PRESET_NAME, GetModuleHandle(NULL), NULL);
            CreateWindow(L"STATIC", L"按键:",
                WS_CHILD | WS_VISIBLE,
                130, 230, 40, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            hEditPresetKeys = CreateWindow(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                175, 228, 125, 22, hwnd, (HMENU)ID_EDIT_PRESET_KEYS, GetModuleHandle(NULL), NULL);
            hButtonAddPreset = CreateWindow(L"BUTTON", L"添加",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                305, 228, 50, 22, hwnd, (HMENU)ID_BUTTON_ADD_PRESET, GetModuleHandle(NULL), NULL);

            // Advanced Settings Area
            CreateWindow(L"BUTTON", L"高级设置",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                10, 270, 360, 50, hwnd, NULL, GetModuleHandle(NULL), NULL);
            CreateWindow(L"STATIC", L"按键时长:",
                WS_CHILD | WS_VISIBLE,
                20, 290, 80, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            hEditKeyDuration = CreateWindow(L"EDIT", L"500",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                105, 288, 50, 22, hwnd, (HMENU)ID_EDIT_KEY_DURATION, GetModuleHandle(NULL), NULL);
            CreateWindow(L"STATIC", L"毫秒 (10-2000)",
                WS_CHILD | WS_VISIBLE,
                160, 290, 80, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            CreateWindow(L"STATIC", L"延迟: 单次 400-900ms, 序列 30-400ms",
                WS_CHILD | WS_VISIBLE,
                245, 290, 120, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

            // Initialize presets and update combo
            InitializePresets();
            UpdatePresetCombo();

            // Add initialization info
            AddLog(L"=== MN按键模拟器 v1.1.0 ===");
            AddLog(L"热键: 按 ~ 键开始/停止模拟");
            AddLog(L"功能: 预设序列和单键模拟");
            AddLog(L"模式: 单次/5次/无限");
            AddLog(L"时长: 可配置 10-2000毫秒 (默认 500毫秒)");
            AddLog(L"准备就绪, 等待操作...");
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
                    AddLog(L"[模式] 已选择: 单次模式");
                } else if (mode == 1) {
                    AddLog(L"[模式] 已选择: 多次模式 (5 次)");
                } else {
                    AddLog(L"[模式] 已选择: 无限模式");
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
                    wsprintf(logText, L"[预设] 已选择: %s = %s",
                             presets[sel - 1].name, presets[sel - 1].keys);
                    AddLog(logText);
                } else {
                    AddLog(L"[预设] 已清除选择");
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
                    wsprintf(logText, L"[预设] 已添加: %s = %s", name, keys);
                    AddLog(logText);
                } else {
                    AddLog(L"[错误] 无效预设或达到限制 (最多 10 个)");
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
                    wsprintf(logText, L"[按键] 已更改为: '%c'", keyText[0]);
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
                    wsprintf(logText, L"[时长] 按键保持时间: 10-%d毫秒", duration);
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
            AddLog(L"[热键] ~ 键被按下");
            ToggleKeySimulation();
        }
        break;

    case WM_SIZE:
        {
            RECT rect;
            GetClientRect(hwnd, &rect);
            // Adjust log area size to fit window
            SetWindowPos(hListBox, NULL, 10, 30, rect.right - 20, 80, SWP_NOZORDER);
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
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 370,
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
