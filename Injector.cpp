#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <string>

#pragma comment(lib, "psapi.lib")

enum class EClientType
{
    Flash,
    Unity
};

// 注入 DLL 并传递客户端类型参数
bool InjectDLL(DWORD pid, const std::string &dllPath, EClientType ClientType)
{
    // 打开目标进程
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess)
    {
        std::cerr << "[!] 无法打开目标进程" << std::endl;
        return false;
    }

    // 分配远程内存用于 DLL 路径
    LPVOID pRemotePath = VirtualAllocEx(hProcess, nullptr, dllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!pRemotePath)
    {
        std::cerr << "[!] 无法分配远程内存" << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    // 写入 DLL 路径
    if (!WriteProcessMemory(hProcess, pRemotePath, dllPath.c_str(), dllPath.size() + 1, nullptr))
    {
        std::cerr << "[!] 写入远程内存失败" << std::endl;
        VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 远程加载 DLL
    LPTHREAD_START_ROUTINE pLoadLibrary = (LPTHREAD_START_ROUTINE)
        GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, pLoadLibrary, pRemotePath, 0, nullptr);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);

    // 获取远程 Module Base
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

    // 本地加载以获取 InitHook_Thread 偏移
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

    // 分配并写入 EClientType 参数
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

    // 远程调用 InitHook_Thread(LPVOID)
    HANDLE hInitThread = CreateRemoteThread(
        hProcess,
        nullptr,
        0,
        (LPTHREAD_START_ROUTINE)pInitRemote,
        pRemoteArg,
        0,
        nullptr
    );
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
    EClientType ClientType = EClientType::Unity;
    std::string exePath = (ClientType == EClientType::Flash)
        ? R"(C:\Users\henry\Desktop\C#\SeerLauncher\bin\x64\Debug\SeerLauncher.exe)"
        : R"(D:\Games\Seer\SeerLauncher\games\NewSeer\Seer.exe)";

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessA(exePath.c_str(), nullptr, nullptr, nullptr,
                        FALSE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi))
    {
        std::cerr << "[!] 无法启动目标进程，错误: " << GetLastError() << std::endl;
        return 1;
    }
    std::cout << "[*] 目标 PID: " << pi.dwProcessId << std::endl;

    // 获取 DLL 绝对路径
    CHAR full[MAX_PATH] = {0};
    GetFullPathNameA("SocketHook.dll", MAX_PATH, full, nullptr);
    std::string dllPath(full);

    // 恢复并注入
    ResumeThread(pi.hThread);
    WaitForInputIdle(pi.hProcess, 5000);
    if (!InjectDLL(pi.dwProcessId, dllPath, ClientType))
    {
        std::cerr << "[!] 注入失败" << std::endl;
        return 1;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    system("pause");
    return 0;
}
