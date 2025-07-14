#include <windows.h>
#include <iostream>
#include <shlwapi.h>
#include <cstdio>
#include <TlHelp32.h>

#pragma comment (lib, "Shlwapi.lib")


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

HINSTANCE hInst;
wchar_t gameFilePath[MAX_PATH] = { 0 };
wchar_t modFilePath[MAX_PATH] = { 0 };
HWND hGameNameText = nullptr;
HWND hModNameText = nullptr;
HWND hStatusText = nullptr;
HICON bgIcon = NULL;
#define LaunchButtonID 1001
#define SelectGameButtonID 1002
#define SelectModButtonID 1003
#define WINDOW_CLASS_NAME L"UE_ModLauncherClass"
#define WINDOW_TITLE      L"Unreal Engine - Mod Launcher"

DWORD GetProcessID(const char* targetProcessName)
{
    DWORD targetProcessID = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);

        if (Process32First(hSnapshot, &processEntry))
        {
            do
            {
                if (!_stricmp(processEntry.szExeFile, targetProcessName))
                {
                    targetProcessID = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &processEntry));
        }
    }
    CloseHandle(hSnapshot);
    return targetProcessID;
}

std::string GetShippingExeName()
{
    /*PathFindFileNameW(gameFilePath) -> L"Game.exe"
        WideCharToMultiByte -> "Game.exe"
        rfind(".exe") ->position 4
        resize(4) -> "Game"
        Return -> "Game-Win64-Shipping.exe"*/

    char base[MAX_PATH]; 
    WideCharToMultiByte(CP_ACP, 0, PathFindFileNameW(gameFilePath), -1, base, MAX_PATH, nullptr, nullptr);
    
    std::string str = base;
    size_t removal = str.rfind(".exe");
    
    if (removal != std::string::npos)
    {
        str.resize(removal);
    }
    
    return str + "-Win64-Shipping.exe";
}

void SaveSelectedItems(const wchar_t* GameFilePath, const wchar_t* ModFilePath)
{
    wchar_t iniPath[MAX_PATH];
    GetModuleFileNameW(NULL, iniPath, MAX_PATH);
    PathRemoveFileSpecW(iniPath);
    wcscat_s(iniPath, L"\\ModConfig.ini");

    WritePrivateProfileStringW(L"Settings", L"GamePath", GameFilePath, iniPath);
    WritePrivateProfileStringW(L"Settings", L"ModPath", ModFilePath, iniPath);
}

void LoadSelectedItems(HWND hWnd)
{
    wchar_t iniPath[MAX_PATH];
    GetModuleFileNameW(NULL, iniPath, MAX_PATH);
    PathRemoveFileSpecW(iniPath);
    wcscat_s(iniPath, L"\\ModConfig.ini");

    GetPrivateProfileStringW(L"Settings", L"GamePath", L"", gameFilePath, MAX_PATH, iniPath);
    GetPrivateProfileStringW(L"Settings", L"ModPath", L"", modFilePath, MAX_PATH, iniPath);


    if (wcslen(gameFilePath) > 0)
    {
        wchar_t* fileName = PathFindFileNameW(gameFilePath);
        wchar_t gameNameText[512];
        wsprintfW(gameNameText, L"Game.exe : %s", fileName);
        SetWindowTextW(hGameNameText, gameNameText);
    }

    if (wcslen(modFilePath) > 0)
    {
        wchar_t* fileName = PathFindFileNameW(modFilePath);
        wchar_t modNameText[512];
        wsprintfW(modNameText, L"Mod.dll : %s", fileName);
        SetWindowTextW(hModNameText, modNameText);
    }
}

void CreateBackgroundImage(HWND hWnd)
{
    bgIcon = (HICON)LoadImageW(
        hInst,
        MAKEINTRESOURCEW(104),
        IMAGE_ICON,
        0, 0,
        LR_DEFAULTCOLOR | LR_SHARED
    );
}

void DrawBackgroundImage(HWND hWnd)
{
    PAINTSTRUCT paintStruct;
    HDC hdc = BeginPaint(hWnd, &paintStruct);

    if (bgIcon)
    {
        RECT rect;
        GetClientRect(hWnd, &rect);
        DrawIconEx(hdc, 0, 0, bgIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);
    }

    EndPaint(hWnd, &paintStruct);
}

HWND CreateText(HWND hWnd, const wchar_t* text, int x, int y, int width, int height, DWORD style)
{
    return CreateWindowW(L"STATIC", text,
        WS_VISIBLE | WS_CHILD | style,
        x, y, width, height,
        hWnd, nullptr, hInst, nullptr);
}

HWND CreateButton(HWND hWnd, const wchar_t* text, int x, int y, int width, int height, int buttonID)
{
    return CreateWindowW(L"BUTTON", text,
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        x, y, width, height,
        hWnd, (HMENU)buttonID, hInst, nullptr);
}

void OnGameButtonClicked(HWND hWnd)
{
    wchar_t filePath[MAX_PATH] = { 0 };

    OPENFILENAMEW file = {};
    file.lStructSize = sizeof(file);
    file.hwndOwner = hWnd;
    file.lpstrFilter = L"Executable Files\0*.exe\0All Files\0*.*\0";
    file.lpstrFile = filePath;
    file.nMaxFile = MAX_PATH;
    file.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    file.lpstrTitle = L"Select Game.exe file";

    if (GetOpenFileNameW(&file))
    {
        wcscpy_s(gameFilePath, filePath);
        wchar_t* fileName = PathFindFileNameW(filePath);
        wchar_t gameNameText[512];
        wsprintfW(gameNameText, L"Game.exe : %s", fileName);

        SetWindowTextW(hGameNameText, gameNameText);
        SaveSelectedItems(gameFilePath, modFilePath);
    }
}

void OnModButtonClicked(HWND hWnd)
{
    wchar_t filePath[MAX_PATH] = { 0 };

    OPENFILENAMEW file = {};
    file.lStructSize = sizeof(file);
    file.hwndOwner = hWnd;
    file.lpstrFilter = L"DLL Files\0*.dll\0All Files\0*.*\0";
    file.lpstrFile = filePath;
    file.nMaxFile = MAX_PATH;
    file.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    file.lpstrTitle = L"Select Mod.dll file";

    if (GetOpenFileNameW(&file))
    {
        wcscpy_s(modFilePath, filePath);

        wchar_t* fileName = PathFindFileNameW(filePath);
        wchar_t modNameText[512];
        wsprintfW(modNameText, L"Mod.dll : %s", fileName);

        SetWindowTextW(hModNameText, modNameText);
        SaveSelectedItems(gameFilePath, modFilePath);
    }
}

void OnLaunchButtonClicked(HWND hWnd)
{
    if (!gameFilePath[0] || !modFilePath[0])
    {
        MessageBoxW(hWnd, L"Select both Game.exe and Mod.dll first.", L"Error", MB_OK | MB_ICONWARNING);
        return;
    }

    STARTUPINFOW startupInfo = { sizeof(startupInfo) };
    PROCESS_INFORMATION processInfo;
    DWORD processID = 0;

    // Launch the game process
    if (!CreateProcessW(gameFilePath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo))
    {
        MessageBoxW(hWnd, L"Failed to launch game.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    
    SetWindowTextW(hStatusText, L"Status : Waiting for game to start");

    // Wait for Shipping.exe
    std::string shipping = GetShippingExeName();
    while (!processID)
    {
        processID = GetProcessID(shipping.c_str());
        Sleep(500);
    }

    SetWindowTextW(hStatusText, L"Status : Injecting mod");

    // Prepare DLL path ANSI
    char dllPathA[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, modFilePath, -1, dllPathA, MAX_PATH, nullptr, nullptr);

    // Open target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!hProcess)
    {
        MessageBoxW(hWnd, L"Failed to open process.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Allocate & write DLL path
    void* remote = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    WriteProcessMemory(hProcess, remote, dllPathA, strlen(dllPathA) + 1, NULL);

    // CreateRemoteThread(LoadLibraryA)
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    FARPROC address = GetProcAddress(kernel32, "LoadLibraryA");
    HANDLE remoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)address, remote, 0, NULL);
    
    if (remoteThread)
    {
        CloseHandle(remoteThread);
        SetWindowTextW(hStatusText, L"Status : Injection complete");
    }
    CloseHandle(hProcess);
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
            CreateBackgroundImage(hWnd);

            CreateText(hWnd, L"1)", 20, 110, 20, 20, SS_CENTER);
            CreateText(hWnd, L"2)", 20, 210, 20, 20, SS_CENTER);
            CreateText(hWnd, L"3)", 20, 310, 20, 20, SS_CENTER);
            CreateText(hWnd, L"UNREAL ENGINE - MOD LAUNCHER \n\n Select your Game.exe file + Mod.dll file and then Launch the game :)", 40, 20, 500, 50, SS_CENTER);
            
            hGameNameText = CreateText(hWnd, L"Game.exe : ???", 250, 110, 300, 20, SS_CENTER);
            hModNameText = CreateText(hWnd, L"Mod.dll : ???" , 250, 210, 300, 20, SS_CENTER);
            hStatusText = CreateText(hWnd, L"Status : Game not launched"  , 250, 310, 300, 20, SS_CENTER);
            
            CreateButton(hWnd, L"Select Game.exe", 50, 100, 150, 40, SelectGameButtonID);
            CreateButton(hWnd, L"Select Mod.dll" , 50, 200, 150, 40, SelectModButtonID);
            CreateButton(hWnd, L"Launch Game"    , 50, 300, 150, 40, LaunchButtonID);
            
            LoadSelectedItems(hWnd);
            
            break;

        case WM_PAINT:
        {
            DrawBackgroundImage(hWnd);
            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == LaunchButtonID)
            {
                OnLaunchButtonClicked(hWnd);
            }
            if (LOWORD(wParam) == SelectGameButtonID)
            {
                OnGameButtonClicked(hWnd);
            }
            if (LOWORD(wParam) == SelectModButtonID)
            {
                OnModButtonClicked(hWnd);
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    hInst = hInstance;

    // Register window class
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassExW(&wcex))
        return 0;

    // Create window
    HWND hWnd = CreateWindowW(
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT, 
        600, 400,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}