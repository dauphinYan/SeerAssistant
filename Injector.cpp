#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>

#pragma comment(lib, "kernel32.lib")

// 获取进程ID
DWORD GetProcessIdByName(const std::string &processName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe))
    {
        do
        {
            if (processName == pe.szExeFile)
            {
                CloseHandle(hSnapshot);
                return pe.th32ProcessID;
            }
        } while (Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return 0;
}

// 注入 DLL 到目标进程
bool InjectDLL(DWORD pid, const std::string &dllPath)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess)
    {
        std::cerr << "[!] 无法打开目标进程" << std::endl;
        return false;
    }

    // 分配内存存放 DLL 路径
    LPVOID pRemoteMem = VirtualAllocEx(hProcess, nullptr, dllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteMem)
    {
        std::cerr << "[!] 无法分配远程内存" << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    // 写入 DLL 路径
    if (!WriteProcessMemory(hProcess, pRemoteMem, dllPath.c_str(), dllPath.size() + 1, nullptr))
    {
        std::cerr << "[!] 写入远程内存失败" << std::endl;
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 获取 LoadLibraryA 地址
    LPTHREAD_START_ROUTINE loadLibraryRoutine = (LPTHREAD_START_ROUTINE)
        GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLibraryRoutine)
    {
        std::cerr << "[!] 获取 LoadLibraryA 地址失败" << std::endl;
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 创建远程线程加载 DLL
    HANDLE hRemoteThread = CreateRemoteThread(hProcess, nullptr, 0,
                                              loadLibraryRoutine, pRemoteMem, 0, nullptr);
    if (!hRemoteThread)
    {
        std::cerr << "[!] 创建远程线程失败" << std::endl;
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hRemoteThread, INFINITE);

    // 清理资源
    VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
    CloseHandle(hRemoteThread);
    CloseHandle(hProcess);

    std::cout << "[+] DLL 注入成功！" << std::endl;
    return true;
}

DWORD GetProcessIdByKeyword(const std::string &keyword)
{
    DWORD pid = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &pe))
        {
            do
            {
                std::string processName(pe.szExeFile);
                // 检查进程名是否包含关键字
                if (processName.find(keyword) != std::string::npos)
                {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe));
        }

        CloseHandle(hSnapshot);
    }
    return pid;
}

int main()
{
    std::string processKeyword = "SeerLauncher"; // 只要进程名包含 Waking 即可

    char fullPath[MAX_PATH] = {0};
    GetFullPathNameA("SocketHook.dll", MAX_PATH, fullPath, nullptr);
    std::string dllPath = fullPath;

    std::cout << "[*] 正在模糊查找包含 '" << processKeyword << "' 的进程..." << std::endl;
    DWORD pid = GetProcessIdByKeyword(processKeyword);
    if (pid == 0)
    {
        std::cerr << "[!] 未找到匹配的进程" << std::endl;
        return 1;
    }

    std::cout << "[*] 找到进程 PID: " << pid << std::endl;
    std::cout << "[*] 正在注入 DLL..." << std::endl;

    if (InjectDLL(pid, dllPath))
    {
        std::cout << "[+] 注入完成。" << std::endl;
    }
    else
    {
        std::cerr << "[!] 注入失败。" << std::endl;
    }

    system("pause");
    return 0;
}
