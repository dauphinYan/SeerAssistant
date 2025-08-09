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

const std::string Injector::hookDllName = "SocketHook_Flash.dll";

Injector::Injector()
{
    char Buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, Buffer);

    Log::InitLogPath(Buffer);
    Log::InitBattleLogPath(Buffer);
    Cryptor::InitKey("!crAckmE4nOthIng:-)");
    DispatcherManager::InitDispatcher();
    clientType = ClientType::Flash;
    PacketProcessor::SetClientType(clientType);
}

void Injector::StartInjector()
{
    std::thread pipeServer([this]
                           { this->PipeServerLoop(); });

    std::string GamePath = (clientType == ClientType::Flash)
                               ? R"(C:\Users\58448\Desktop\SeerLauncher\bin\x64\Debug\SeerLauncher.exe)"
                               : R"(D:\Seer\SeerLauncher\games\NewSeer\Seer.exe)";

    STARTUPINFOA startInfo = {sizeof(startInfo)};
    PROCESS_INFORMATION processInfo = {};
    if (!CreateProcessA(GamePath.c_str(), nullptr, nullptr, nullptr,
                        FALSE, CREATE_SUSPENDED, nullptr, nullptr, &startInfo, &processInfo))
    {
        Log::WriteLog("无法启动目标进程，错误:" + GetLastError(), LogLevel::Error);
        return;
    }
    Log::WriteLog("目标 PID：" + processInfo.dwProcessId);

    CHAR full[MAX_PATH] = {0};
    GetFullPathNameA(hookDllName.c_str(), MAX_PATH, full, nullptr);
    std::string dllPath(full);
    cout << dllPath << endl;
    ResumeThread(processInfo.hThread);

    Log::WriteLog("等待进程启动...");
    WaitForInputIdle(processInfo.hProcess, 15000);
    Sleep(3000);

    DWORD exitCode;
    if (GetExitCodeProcess(processInfo.hProcess, &exitCode) && exitCode != STILL_ACTIVE)
    {
        Log::WriteLog("目标进程已退出，退出码: " + exitCode, LogLevel::Temp);
        return;
    }

    Log::WriteLog("开始注入DLL...");
    if (!InjectDll(processInfo.dwProcessId, dllPath, clientType))
    {
        Log::WriteLog("注入失败。", LogLevel::Error);
        return;
    }

    pipeServer.join();

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
}

bool Injector::InjectDll(DWORD pid, const std::string &dllPath, ClientType clientType)
{
    HANDLE curProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!curProcess)
    {
        Log::WriteLog("无法打开目标进程。", LogLevel::Error);
        return false;
    }

    LPVOID remotePath = VirtualAllocEx(curProcess, nullptr, dllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!remotePath)
    {
        Log::WriteLog("无法分配远程内存。", LogLevel::Error);
        CloseHandle(curProcess);
        return false;
    }

    bool bIsWriteProcessMemory = WriteProcessMemory(curProcess, remotePath, dllPath.c_str(), dllPath.size() + 1, nullptr);
    if (!bIsWriteProcessMemory)
    {
        Log::WriteLog("写入内存失败。", LogLevel::Error);
        VirtualFreeEx(curProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(curProcess);
        return false;
    }

    LPTHREAD_START_ROUTINE loadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    HANDLE socketHookThread = CreateRemoteThread(curProcess, nullptr, 0, loadLibrary, remotePath, 0, nullptr);
    WaitForSingleObject(socketHookThread, INFINITE);
    CloseHandle(socketHookThread);
    VirtualFreeEx(curProcess, remotePath, 0, MEM_RELEASE);

    HMODULE modules[1024];
    DWORD bufferNeeded;
    HMODULE remoteModule = nullptr;
    if (EnumProcessModules(curProcess, modules, sizeof(modules), &bufferNeeded))
    {
        for (DWORD i = 0; i < bufferNeeded / sizeof(HMODULE); ++i)
        {
            CHAR modName[MAX_PATH] = {0};
            if (GetModuleBaseNameA(curProcess, modules[i], modName, MAX_PATH))
            {
                if (_stricmp(modName, hookDllName.c_str()) == 0)
                {
                    remoteModule = modules[i];
                    break;
                }
            }
        }
    }

    if (!remoteModule)
    {
        Log::WriteLog("未找到远程 SocketHook.dll。", LogLevel::Error);
        CloseHandle(curProcess);
        return false;
    }

    HMODULE localModule = ::LoadLibraryA(dllPath.c_str());

    if (!localModule)
    {
        Log::WriteLog("本地加载 DLL 失败。", LogLevel::Error);
        CloseHandle(curProcess);
        return false;
    }

    FARPROC initLocal = GetProcAddress(localModule, "InitHook_Thread");
    if (!initLocal)
    {
        Log::WriteLog("找不到 InitHook_Thread 方法。", LogLevel::Error);
        FreeLibrary(localModule);
        CloseHandle(curProcess);
        return false;
    }

    DWORD_PTR Offset = (DWORD_PTR)initLocal - (DWORD_PTR)localModule;
    FreeLibrary(localModule);
    FARPROC initRemote = (FARPROC)((DWORD_PTR)remoteModule + Offset);

    LPVOID remoteArg = VirtualAllocEx(curProcess, nullptr, sizeof(ClientType), MEM_COMMIT, PAGE_READWRITE);
    if (!remoteArg)
    {
        Log::WriteLog("无法分配参数内存。", LogLevel::Error);
        CloseHandle(curProcess);
        return false;
    }
    if (!WriteProcessMemory(curProcess, remoteArg, &clientType, sizeof(ClientType), nullptr))
    {
        Log::WriteLog("写入参数失败。", LogLevel::Error);
        VirtualFreeEx(curProcess, remoteArg, 0, MEM_RELEASE);
        CloseHandle(curProcess);
        return false;
    }

    HANDLE initThread = CreateRemoteThread(
        curProcess,
        nullptr,
        0,
        (LPTHREAD_START_ROUTINE)initRemote,
        remoteArg,
        0,
        nullptr);
    if (!initThread)
    {
        Log::WriteLog("CreateRemoteThread 失败，错误码:" + GetLastError(), LogLevel::Error);
        VirtualFreeEx(curProcess, remoteArg, 0, MEM_RELEASE);
        CloseHandle(curProcess);
        return false;
    }
    WaitForSingleObject(initThread, INFINITE);
    CloseHandle(initThread);
    VirtualFreeEx(curProcess, remoteArg, 0, MEM_RELEASE);

    Log::WriteLog("DLL 注入并初始化成功！");
    CloseHandle(curProcess);
    return true;
}

void Injector::PipeServerLoop()
{
    HANDLE pipe = CreateNamedPipeW(
        PIPE_NAME,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE |
            PIPE_READMODE_MESSAGE |
            PIPE_WAIT,
        1,
        0, 0,
        0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE)
    {
        Log::WriteLog("创建管道名失败，错误码：" + std::to_string(GetLastError()), LogLevel::Error);
        return;
    }

    Log::WriteLog("等待 SocketHook.dll 连接到管道...");

    if (!ConnectNamedPipe(pipe, nullptr) && GetLastError() != ERROR_PIPE_CONNECTED)
    {
        Log::WriteLog("连接管道名失败，错误码：" + std::to_string(GetLastError()), LogLevel::Error);
        CloseHandle(pipe);
        return;
    }

    while (true)
    {
        PacketHeader header;
        DWORD bytesRead = 0;
        if (!ReadFile(pipe, &header, sizeof(header), &bytesRead, nullptr) || bytesRead == 0)
            break;

        std::vector<char> payload(header.payloadSize);
        if (!ReadFile(pipe, payload.data(), header.payloadSize, &bytesRead, nullptr))
            break;

        std::string dirStr = (header.direction == 0) ? "Recv" : "Send";

        std::ostringstream oss;
        for (int i = 0; i < header.payloadSize; ++i)
            oss << std::hex << std::setw(2) << std::setfill('0')
                << (unsigned int)(unsigned char)payload[i] << " ";

        if (header.direction == 0)
        {
            PacketProcessor::ProcessRecvPacket(header.socket, payload, header.payloadSize);
        }
        else
        {
            PacketProcessor::ProcessSendPacket(header.socket, payload, header.payloadSize);
        }
    }

    Log::WriteLog("管道断开，退出。");
    CloseHandle(pipe);
}

int main()
{
    Injector *injector = new Injector();
    injector->StartInjector();
    delete injector;
    return 0;
}