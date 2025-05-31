#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlobj.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

#define PRESET_COUNT 5
#define COMMAND_BUFFER_SIZE 512
#define TASK_NAME L"NetworkOptimizerAutoRun"

#define ID_BALANCED 1001
#define ID_BEST_HIT_REG 1002
#define ID_BETTER_HIT_REG 1003
#define ID_SUMO 1004
#define ID_RESET_DEFAULT 1005
#define ID_STATUS 1006

typedef struct {
    const char* name;
    const char** commands;
    int commandCount;
} Preset;

const char* balancedCommands[] = {
    "netsh int tcp set global autotuninglevel=restricted",
    "netsh int tcp set global chimney=disabled",
    "netsh int tcp set global dca=enabled",
    "netsh int tcp set global ecncapability=enabled",
    "netsh int tcp set global timestamps=disabled",
    "netsh int tcp set global rss=enabled",
    "netsh int tcp set global fastopen=enabled",
    "netsh int tcp set global nonsackrttresiliency=disabled",
    "netsh int tcp set global initialRto=2500",
    "netsh int tcp set global maxsynretransmissions=3",
    "netsh int tcp set supplemental template=Internet congestionprovider=ctcp",
    "netsh int tcp set supplemental template=custom icw=24",
    "netsh int tcp set supplemental template=custom icwndauto=enabled"
};

const char* bestHitRegCommands[] = {
    "netsh int tcp set global autotuninglevel=disabled",
    "netsh int tcp set global chimney=disabled",
    "netsh int tcp set global dca=disabled",
    "netsh int tcp set global ecncapability=disabled",
    "netsh int tcp set global timestamps=disabled",
    "netsh int tcp set global rss=enabled",
    "netsh int tcp set global fastopen=enabled",
    "netsh int tcp set global nonsackrttresiliency=disabled",
    "netsh int tcp set global initialRto=2000",
    "netsh int tcp set global maxsynretransmissions=2",
    "netsh int tcp set global netdma=disabled",
    "netsh int tcp set supplemental template=InternetCustom congestionprovider=newreno",
    "netsh int tcp set supplemental template=custom icw=16",
    "netsh int tcp set supplemental template=custom icwndauto=disabled"
};

const char* betterHitRegCommands[] = {
    "netsh int tcp set global autotuninglevel=restricted",
    "netsh int tcp set global chimney=disabled",
    "netsh int tcp set global dca=disabled",
    "netsh int tcp set global ecncapability=enabled",
    "netsh int tcp set global timestamps=disabled",
    "netsh int tcp set global rss=enabled",
    "netsh int tcp set global fastopen=enabled",
    "netsh int tcp set global nonsackrttresiliency=disabled",
    "netsh int tcp set global initialRto=2000",
    "netsh int tcp set global maxsynretransmissions=3",
    "netsh int tcp set global netdma=disabled",
    "netsh int tcp set supplemental template=Internet congestionprovider=newreno",
    "netsh int tcp set supplemental template=custom icw=24",
    "netsh int tcp set supplemental template=custom icwndauto=disabled"
};

const char* sumoCommands[] = {
    "netsh int tcp set global autotuninglevel=disabled",
    "netsh int tcp set global chimney=disabled",
    "netsh int tcp set global dca=disabled",
    "netsh int tcp set global ecncapability=disabled",
    "netsh int tcp set global timestamps=disabled",
    "netsh int tcp set global rss=enabled",
    "netsh int tcp set global fastopen=enabled",
    "netsh int tcp set global nonsackrttresiliency=disabled",
    "netsh int tcp set global initialRto=2000",
    "netsh int tcp set global maxsynretransmissions=4",
    "netsh int tcp set global netdma=disabled",
    "netsh int tcp set supplemental template=Internet congestionprovider=newreno",
    "netsh int tcp set supplemental template=custom icw=16",
    "netsh int tcp set supplemental template=custom icwndauto=disabled"
};

const char* windowsDefaultCommands[] = {
    "netsh int tcp set global autotuninglevel=normal",
    "netsh int tcp set global chimney=disabled",
    "netsh int tcp set global dca=enabled",
    "netsh int tcp set global ecncapability=disabled",
    "netsh int tcp set global timestamps=disabled",
    "netsh int tcp set global rss=enabled",
    "netsh int tcp set global fastopen=disabled",
    "netsh int tcp set global nonsackrttresiliency=disabled",
    "netsh int tcp set global initialRto=3000",
    "netsh int tcp set global congestionprovider=ctcp",
    "netsh int tcp set supplemental template=internet congestionprovider=ctcp",
    "netsh int ip set global defaultcurhoplimit=128",
    "netsh int ip set global neighborunreachabletime=30",
    "netsh int ip set global routercachelimit=32",
    "netsh int tcp set global hystart=enabled",
    "netsh int tcp set global timestamps=disabled",
    "netsh int tcp set global fastopen=enabled",
    "netsh int tcp set global ecncapability=enabled",
    "netsh int reset"
};

Preset presets[PRESET_COUNT] = {
    {"Balanced Preset", balancedCommands, sizeof(balancedCommands) / sizeof(balancedCommands[0])},
    {"Best Hit Reg Preset", bestHitRegCommands, sizeof(bestHitRegCommands) / sizeof(bestHitRegCommands[0])},
    {"Better Hit Reg", betterHitRegCommands, sizeof(betterHitRegCommands) / sizeof(betterHitRegCommands[0])},
    {"Sumo Preset", sumoCommands, sizeof(sumoCommands) / sizeof(sumoCommands[0])},
    {"Reset to Windows Default", windowsDefaultCommands, sizeof(windowsDefaultCommands) / sizeof(windowsDefaultCommands[0])}
};

HWND hStatus;
HWND hMainWindow;

static BOOL IsRunAsAdmin() {
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation = { 0 };
        DWORD size = sizeof(TOKEN_ELEVATION);

        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isAdmin = elevation.TokenIsElevated;
        }

        CloseHandle(hToken);
    }

    return isAdmin;
}

static BOOL CreateAutoRunTask() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    wchar_t command[1024];
    swprintf_s(command, 1024,
        L"schtasks /create /tn \"%s\" /tr \"\\\"%s\\\"\" /sc onlogon /rl highest /f",
        TASK_NAME, exePath);

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    BOOL success = CreateProcessW(NULL, command, NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (success) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return TRUE;
    }

    return FALSE;
}

static BOOL TaskExists() {
    wchar_t command[512];
    swprintf_s(command, 512, L"schtasks /query /tn \"%s\"", TASK_NAME);

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    BOOL success = CreateProcessW(NULL, command, NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (success) {
        DWORD exitCode;
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return exitCode == 0;
    }

    return FALSE;
}

static void RunAsAdminViaTask() {
    wchar_t command[512];
    swprintf_s(command, 512, L"schtasks /run /tn \"%s\"", TASK_NAME);

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    CreateProcessW(NULL, command, NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (pi.hProcess) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

static void ExecuteCommand(const char* cmd) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    char command[COMMAND_BUFFER_SIZE];

    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    strcpy_s(command, COMMAND_BUFFER_SIZE, cmd);

    if (CreateProcessA(NULL, command, NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

static DWORD WINAPI ApplyPresetThread(LPVOID lpParam) {
    int presetIndex = (int)(intptr_t)lpParam;

    EnableWindow(GetDlgItem(hMainWindow, ID_BALANCED), FALSE);
    EnableWindow(GetDlgItem(hMainWindow, ID_BEST_HIT_REG), FALSE);
    EnableWindow(GetDlgItem(hMainWindow, ID_BETTER_HIT_REG), FALSE);
    EnableWindow(GetDlgItem(hMainWindow, ID_SUMO), FALSE);
    EnableWindow(GetDlgItem(hMainWindow, ID_RESET_DEFAULT), FALSE);

    char statusText[256];
    sprintf_s(statusText, 256, "Applying %s...", presets[presetIndex].name);
    SetWindowTextA(hStatus, statusText);

    for (int i = 0; i < presets[presetIndex].commandCount; i++) {
        ExecuteCommand(presets[presetIndex].commands[i]);
    }

    sprintf_s(statusText, 256, "%s applied successfully!", presets[presetIndex].name);
    SetWindowTextA(hStatus, statusText);

    EnableWindow(GetDlgItem(hMainWindow, ID_BALANCED), TRUE);
    EnableWindow(GetDlgItem(hMainWindow, ID_BEST_HIT_REG), TRUE);
    EnableWindow(GetDlgItem(hMainWindow, ID_BETTER_HIT_REG), TRUE);
    EnableWindow(GetDlgItem(hMainWindow, ID_SUMO), TRUE);
    EnableWindow(GetDlgItem(hMainWindow, ID_RESET_DEFAULT), TRUE);

    return 0;
}

static void ApplyPreset(int presetIndex) {
    HANDLE hThread = CreateThread(NULL, 0, ApplyPresetThread,
        (LPVOID)(intptr_t)presetIndex, 0, NULL);

    if (hThread) {
        CloseHandle(hThread);
    }
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
    {
        HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Segoe UI"));

        int buttonY = 50;
        int buttonHeight = 35;
        int buttonSpacing = 45;

        HWND hButton = CreateWindow(TEXT("BUTTON"), TEXT("Balanced Preset"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, buttonY, 300, buttonHeight, hwnd, (HMENU)ID_BALANCED,
            GetModuleHandle(NULL), NULL);
        SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);

        buttonY += buttonSpacing;
        hButton = CreateWindow(TEXT("BUTTON"), TEXT("Best Hit Reg Preset"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, buttonY, 300, buttonHeight, hwnd, (HMENU)ID_BEST_HIT_REG,
            GetModuleHandle(NULL), NULL);
        SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);

        buttonY += buttonSpacing;
        hButton = CreateWindow(TEXT("BUTTON"), TEXT("Better Hit Reg"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, buttonY, 300, buttonHeight, hwnd, (HMENU)ID_BETTER_HIT_REG,
            GetModuleHandle(NULL), NULL);
        SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);

        buttonY += buttonSpacing;
        hButton = CreateWindow(TEXT("BUTTON"), TEXT("Sumo Preset"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, buttonY, 300, buttonHeight, hwnd, (HMENU)ID_SUMO,
            GetModuleHandle(NULL), NULL);
        SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);

        buttonY += buttonSpacing + 20;
        hButton = CreateWindow(TEXT("BUTTON"), TEXT("Reset to Windows Default"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, buttonY, 300, buttonHeight, hwnd, (HMENU)ID_RESET_DEFAULT,
            GetModuleHandle(NULL), NULL);
        SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);

        hStatus = CreateWindow(TEXT("STATIC"), TEXT("Ready"),
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 10, 300, 25, hwnd, (HMENU)ID_STATUS,
            GetModuleHandle(NULL), NULL);
        SendMessage(hStatus, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
    break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BALANCED:
            ApplyPreset(0);
            break;
        case ID_BEST_HIT_REG:
            ApplyPreset(1);
            break;
        case ID_BETTER_HIT_REG:
            ApplyPreset(2);
            break;
        case ID_SUMO:
            ApplyPreset(3);
            break;
        case ID_RESET_DEFAULT:
            ApplyPreset(4);
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {

    if (!IsRunAsAdmin()) {
        if (TaskExists()) {
            RunAsAdminViaTask();
            Sleep(1000);
            return 0;
        }
        else {
            int result = MessageBox(NULL,
                TEXT("This program requires administrator privileges.\n\n")
                TEXT("Click OK to run as administrator (one-time setup).\n")
                TEXT("After this, it will run with admin rights automatically."),
                TEXT("Administrator Required"),
                MB_OKCANCEL | MB_ICONINFORMATION);

            if (result == IDOK) {
                SHELLEXECUTEINFOW sei = { sizeof(sei) };
                wchar_t exePath[MAX_PATH];
                GetModuleFileNameW(NULL, exePath, MAX_PATH);

                sei.lpVerb = L"runas";
                sei.lpFile = exePath;
                sei.lpParameters = L"";
                sei.nShow = SW_NORMAL;

                if (!ShellExecuteExW(&sei)) {
                    MessageBox(NULL, TEXT("Failed to run as administrator."),
                        TEXT("Error"), MB_OK | MB_ICONERROR);
                    return 1;
                }
            }
            return 0;
        }
    }

    if (!TaskExists()) {
        CreateAutoRunTask();
    }

    InitCommonControls();

    const wchar_t CLASS_NAME[] = L"NetworkOptimizerWindow";

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClass(&wc)) {
        return 0;
    }

    hMainWindow = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Network Optimization Tool",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 350,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hMainWindow == NULL) {
        return 0;
    }

    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}