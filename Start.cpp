#include "Start.h"

#include <iostream>
#include <psapi.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <thread>

#include "Src/Common/Log.h"
#include "Src/Net/PacketParser/Packet.h"
#include "Src/Net/PacketParser/Cryptor.h"
#include "Src/Dispatcher/DispatcherManager.h"

#pragma comment(lib, "psapi.lib")

const wchar_t *Injector::PIPE_NAME = L"\\\\.\\pipe\\SeerSocketHook";

Injector::Injector()
{
    char Buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, Buffer);

    Log::InitLogPath(Buffer);
    Log::InitBattleLogPath(Buffer);
    Cryptor::InitKey("!crAckmE4nOthIng:-)");
    DispatcherManager::InitDispatcher();
    ClientType = EClientType::Flash;
    PacketProcessor::SetClientType(ClientType);
}

void Injector::StartInjector()
{
    std::thread PipeServer([this]
                           { this->PipeServerLoop(); });

    std::string GamePath = (ClientType == EClientType::Flash)
                               ? R"(C:\Users\58448\Desktop\SeerLauncher\bin\x64\Debug\SeerLauncher.exe)"
                               : R"(D:\Seer\SeerLauncher\games\NewSeer\Seer.exe)";

    STARTUPINFOA StartInfo = {sizeof(StartInfo)};
    PROCESS_INFORMATION ProcessInfo = {};
    if (!CreateProcessA(GamePath.c_str(), nullptr, nullptr, nullptr,
                        FALSE, CREATE_SUSPENDED, nullptr, nullptr, &StartInfo, &ProcessInfo))
    {
        Log::WriteLog("无法启动目标进程，错误:" + GetLastError(), LogLevel::Error);
        return;
    }
    Log::WriteLog("目标 PID：" + ProcessInfo.dwProcessId);

    CHAR full[MAX_PATH] = {0};
    GetFullPathNameA("SocketHook.dll", MAX_PATH, full, nullptr);
    std::string DllPath(full);
    cout << DllPath << endl;
    ResumeThread(ProcessInfo.hThread);

    Log::WriteLog("等待进程启动...");
    WaitForInputIdle(ProcessInfo.hProcess, 15000);
    Sleep(3000);

    DWORD ExitCode;
    if (GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode) && ExitCode != STILL_ACTIVE)
    {
        Log::WriteLog("目标进程已退出，退出码: " + ExitCode, LogLevel::Temp);
        return;
    }

    Log::WriteLog("开始注入DLL...");
    if (!InjectDll(ProcessInfo.dwProcessId, DllPath, ClientType))
    {
        Log::WriteLog("注入失败。", LogLevel::Error);
        return;
    }

    PipeServer.join();

    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);
}

bool Injector::InjectDll(DWORD Pid, const std::string &DllPath, EClientType ClientType)
{
    HANDLE CurProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Pid);
    if (!CurProcess)
    {
        Log::WriteLog("无法打开目标进程。", LogLevel::Error);
        return false;
    }

    LPVOID RemotePath = VirtualAllocEx(CurProcess, nullptr, DllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!RemotePath)
    {
        Log::WriteLog("无法分配远程内存。", LogLevel::Error);
        CloseHandle(CurProcess);
        return false;
    }

    bool bIsWriteProcessMemory = WriteProcessMemory(CurProcess, RemotePath, DllPath.c_str(), DllPath.size() + 1, nullptr);
    if (!bIsWriteProcessMemory)
    {
        Log::WriteLog("写入内存失败。", LogLevel::Error);
        VirtualFreeEx(CurProcess, RemotePath, 0, MEM_RELEASE);
        CloseHandle(CurProcess);
        return false;
    }

    LPTHREAD_START_ROUTINE LoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    HANDLE SocketHookThread = CreateRemoteThread(CurProcess, nullptr, 0, LoadLibrary, RemotePath, 0, nullptr);
    WaitForSingleObject(SocketHookThread, INFINITE);
    CloseHandle(SocketHookThread);
    VirtualFreeEx(CurProcess, RemotePath, 0, MEM_RELEASE);

    HMODULE Modules[1024];
    DWORD BufferNeeded;
    HMODULE RemoteModule = nullptr;
    if (EnumProcessModules(CurProcess, Modules, sizeof(Modules), &BufferNeeded))
    {
        for (DWORD i = 0; i < BufferNeeded / sizeof(HMODULE); ++i)
        {
            CHAR ModName[MAX_PATH] = {0};
            if (GetModuleBaseNameA(CurProcess, Modules[i], ModName, MAX_PATH))
            {
                if (_stricmp(ModName, "SocketHook.dll") == 0)
                {
                    RemoteModule = Modules[i];
                    break;
                }
            }
        }
    }

    if (!RemoteModule)
    {
        Log::WriteLog("未找到远程 SocketHook.dll。", LogLevel::Error);
        CloseHandle(CurProcess);
        return false;
    }

    HMODULE LocalModule = ::LoadLibraryA(DllPath.c_str());

    if (!LocalModule)
    {
        Log::WriteLog("本地加载 DLL 失败。", LogLevel::Error);
        CloseHandle(CurProcess);
        return false;
    }

    FARPROC InitLocal = GetProcAddress(LocalModule, "InitHook_Thread");
    if (!InitLocal)
    {
        Log::WriteLog("找不到 InitHook_Thread 方法。", LogLevel::Error);
        FreeLibrary(LocalModule);
        CloseHandle(CurProcess);
        return false;
    }

    DWORD_PTR Offset = (DWORD_PTR)InitLocal - (DWORD_PTR)LocalModule;
    FreeLibrary(LocalModule);
    FARPROC InitRemote = (FARPROC)((DWORD_PTR)RemoteModule + Offset);

    LPVOID RemoteArg = VirtualAllocEx(CurProcess, nullptr, sizeof(EClientType), MEM_COMMIT, PAGE_READWRITE);
    if (!RemoteArg)
    {
        Log::WriteLog("无法分配参数内存。", LogLevel::Error);
        CloseHandle(CurProcess);
        return false;
    }
    if (!WriteProcessMemory(CurProcess, RemoteArg, &ClientType, sizeof(EClientType), nullptr))
    {
        Log::WriteLog("写入参数失败。", LogLevel::Error);
        VirtualFreeEx(CurProcess, RemoteArg, 0, MEM_RELEASE);
        CloseHandle(CurProcess);
        return false;
    }

    HANDLE InitThread = CreateRemoteThread(
        CurProcess,
        nullptr,
        0,
        (LPTHREAD_START_ROUTINE)InitRemote,
        RemoteArg,
        0,
        nullptr);
    if (!InitThread)
    {
        Log::WriteLog("CreateRemoteThread 失败，错误码:" + GetLastError(), LogLevel::Error);
        VirtualFreeEx(CurProcess, RemoteArg, 0, MEM_RELEASE);
        CloseHandle(CurProcess);
        return false;
    }
    WaitForSingleObject(InitThread, INFINITE);
    CloseHandle(InitThread);
    VirtualFreeEx(CurProcess, RemoteArg, 0, MEM_RELEASE);

    Log::WriteLog("DLL 注入并初始化成功！");
    CloseHandle(CurProcess);
    return true;
}

void Injector::PipeServerLoop()
{
    HANDLE Pipe = CreateNamedPipeW(
        PIPE_NAME,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE |
            PIPE_READMODE_MESSAGE |
            PIPE_WAIT,
        1,
        0, 0,
        0, nullptr);
    if (Pipe == INVALID_HANDLE_VALUE)
    {
        Log::WriteLog("创建管道名失败，错误码：" + std::to_string(GetLastError()), LogLevel::Error);
        return;
    }

    Log::WriteLog("等待 SocketHook.dll 连接到管道...");

    if (!ConnectNamedPipe(Pipe, nullptr) && GetLastError() != ERROR_PIPE_CONNECTED)
    {
        Log::WriteLog("连接管道名失败，错误码：" + std::to_string(GetLastError()), LogLevel::Error);
        CloseHandle(Pipe);
        return;
    }

    while (true)
    {
        PacketHeader Header;
        DWORD bytesRead = 0;
        if (!ReadFile(Pipe, &Header, sizeof(Header), &bytesRead, nullptr) || bytesRead == 0)
            break;

        std::vector<char> payload(Header.payloadSize);
        if (!ReadFile(Pipe, payload.data(), Header.payloadSize, &bytesRead, nullptr))
            break;

        std::string dirStr = (Header.direction == 0) ? "Recv" : "Send";

        std::ostringstream oss;
        for (int i = 0; i < Header.payloadSize; ++i)
            oss << std::hex << std::setw(2) << std::setfill('0')
                << (unsigned int)(unsigned char)payload[i] << " ";

        if (Header.direction == 0)
        {
            PacketProcessor::ProcessRecvPacket(Header.socket, payload, Header.payloadSize);
        }
        else
        {
            PacketProcessor::ProcessSendPacket(Header.socket, payload, Header.payloadSize);
        }
    }

    Log::WriteLog("管道断开，退出。");
    CloseHandle(Pipe);
}

int main()
{
    Injector *injector = new Injector();
    injector->StartInjector();
    delete injector;
    return 0;
}