#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
#include <thread>
#include <sstream>
#include <iomanip>

#include "src/Common/Log.h"

#pragma comment(lib, "psapi.lib")

enum class EClientType
{
    Flash,
    Unity
};

struct PacketHeader
{
    uint32_t totalSize;
    uint32_t socket;
    uint32_t payloadSize;
    uint8_t direction; // 0 = recv, 1 = send
};

static const wchar_t *PIPE_NAME = L"\\\\.\\pipe\\SeerSocketHook";

void PipeServerLoop()
{
    HANDLE hPipe = CreateNamedPipeW(
        PIPE_NAME,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE |
            PIPE_READMODE_MESSAGE |
            PIPE_WAIT,
        1,
        0, 0,
        0, nullptr);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        Log::WriteLog("CreateNamedPipe failed:" + std::to_string(GetLastError()));
        return;
    }

    Log::WriteLog("等待 SocketHook.dll 连接到管道...");
    BOOL ok = ConnectNamedPipe(hPipe, nullptr);
    if (!ok && GetLastError() != ERROR_PIPE_CONNECTED)
    {
        Log::WriteLog("ConnectNamedPipe failed:" + std::to_string(GetLastError()));
        CloseHandle(hPipe);
        return;
    }
    Log::WriteLog("管道已连接，开始接收数据...");

    while (true)
    {
        PacketHeader header;
        DWORD bytesRead = 0;
        if (!ReadFile(hPipe, &header, sizeof(header), &bytesRead, nullptr) || bytesRead == 0)
            break;

        std::vector<char> payload(header.payloadSize);
        if (!ReadFile(hPipe, payload.data(), header.payloadSize, &bytesRead, nullptr))
            break;

        std::string dirStr = (header.direction == 0) ? "Recv" : "Send";

        std::ostringstream oss;
        for (int i = 0; i < header.payloadSize; ++i)
            oss << std::hex << std::setw(2) << std::setfill('0')
                << (unsigned int)(unsigned char)payload[i] << " ";

        Log::WriteLog("[" + dirStr + "] " + oss.str(), LogLevel::Temp, true);
    }

    Log::WriteLog("管道断开，退出。");
    CloseHandle(hPipe);
}

bool InjectDLL(DWORD pid, const std::string &dllPath, EClientType ClientType)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess)
    {
        std::cerr << "[!] 无法打开目标进程" << std::endl;
        return false;
    }

    LPVOID pRemotePath = VirtualAllocEx(hProcess, nullptr, dllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!pRemotePath)
    {
        std::cerr << "[!] 无法分配远程内存" << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, pRemotePath, dllPath.c_str(), dllPath.size() + 1, nullptr))
    {
        std::cerr << "[!] 写入远程内存失败" << std::endl;
        VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    LPTHREAD_START_ROUTINE pLoadLibrary = (LPTHREAD_START_ROUTINE)
        GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, pLoadLibrary, pRemotePath, 0, nullptr);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);

    HMODULE hMods[1024];
    DWORD cbNeeded;
    HMODULE hRemoteMod = nullptr;
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        for (DWORD i = 0; i < cbNeeded / sizeof(HMODULE); ++i)
        {
            CHAR modName[MAX_PATH] = {0};
            if (GetModuleBaseNameA(hProcess, hMods[i], modName, MAX_PATH))
            {
                if (_stricmp(modName, "SocketHook.dll") == 0)
                {
                    hRemoteMod = hMods[i];
                    break;
                }
            }
        }
    }
    if (!hRemoteMod)
    {
        std::cerr << "[!] 未找到远程 SocketHook.dll" << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    HMODULE hLocalMod = LoadLibraryA(dllPath.c_str());
    if (!hLocalMod)
    {
        std::cerr << "[!] 本地加载 DLL 失败" << std::endl;
        CloseHandle(hProcess);
        return false;
    }
    FARPROC pInitLocal = GetProcAddress(hLocalMod, "InitHook_Thread");
    if (!pInitLocal)
    {
        std::cerr << "[!] 找不到 InitHook_Thread 导出" << std::endl;
        FreeLibrary(hLocalMod);
        CloseHandle(hProcess);
        return false;
    }
    DWORD_PTR offset = (DWORD_PTR)pInitLocal - (DWORD_PTR)hLocalMod;
    FreeLibrary(hLocalMod);

    FARPROC pInitRemote = (FARPROC)((DWORD_PTR)hRemoteMod + offset);

    LPVOID pRemoteArg = VirtualAllocEx(hProcess, nullptr, sizeof(EClientType), MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteArg)
    {
        std::cerr << "[!] 无法分配参数内存" << std::endl;
        CloseHandle(hProcess);
        return false;
    }
    if (!WriteProcessMemory(hProcess, pRemoteArg, &ClientType, sizeof(EClientType), nullptr))
    {
        std::cerr << "[!] 写入参数失败" << std::endl;
        VirtualFreeEx(hProcess, pRemoteArg, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hInitThread = CreateRemoteThread(
        hProcess,
        nullptr,
        0,
        (LPTHREAD_START_ROUTINE)pInitRemote,
        pRemoteArg,
        0,
        nullptr);
    if (!hInitThread)
    {
        std::cerr << "[!] CreateRemoteThread 失败，错误码: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteArg, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    WaitForSingleObject(hInitThread, INFINITE);
    CloseHandle(hInitThread);
    VirtualFreeEx(hProcess, pRemoteArg, 0, MEM_RELEASE);

    std::cout << "[+] DLL 注入并初始化成功！" << std::endl;
    CloseHandle(hProcess);
    return true;
}

int main()
{
    char buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, buffer);

    Log::InitLogPath(buffer);
    Log::InitBattleLogPath(buffer);

    std::thread server(PipeServerLoop);
    EClientType ClientType = EClientType::Flash;
    std::string exePath = (ClientType == EClientType::Flash)
                              ? R"(C:\Users\henry\Desktop\C#\SeerLauncher\bin\x64\Debug\SeerLauncher.exe)"
                              : R"(D:\Games\Seer\SeerLauncher\games\NewSeer\Seer.exe)";

    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessA(exePath.c_str(), nullptr, nullptr, nullptr,
                        FALSE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi))
    {
        std::cerr << "[!] 无法启动目标进程，错误: " << GetLastError() << std::endl;
        return 1;
    }
    std::cout << "[*] 目标 PID: " << pi.dwProcessId << std::endl;

    CHAR full[MAX_PATH] = {0};
    GetFullPathNameA("SocketHook.dll", MAX_PATH, full, nullptr);
    std::string dllPath(full);

    ResumeThread(pi.hThread);
    
    std::cout << "[*] 等待进程启动..." << std::endl;
    WaitForInputIdle(pi.hProcess, 15000);
    
    Sleep(3000);
    
    DWORD exitCode;
    if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
        std::cerr << "[!] 目标进程已退出，退出码: " << exitCode << std::endl;
        return 1;
    }
    
    std::cout << "[*] 开始注入DLL..." << std::endl;
    if (!InjectDLL(pi.dwProcessId, dllPath, ClientType))
    {
        std::cerr << "[!] 注入失败" << std::endl;
        return 1;
    }

    server.join();

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    system("pause");
    return 0;
}
