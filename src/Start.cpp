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

const std::string Injector::hookDllName = "SocketHook.dll";

const ClientType Injector::clientType = ClientType::Unity;

const std::string Injector::gamePath_Flash = R"(C:\Users\58448\Desktop\SeerLauncher\bin\x64\Debug\SeerLauncher.exe)";

const std::string Injector::gamePath_Unity = R"(D:\Games\Seer\SeerLauncher\games\NewSeer\Seer.exe)";

Injector::Injector()
{
    char buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, buffer);

    Log::InitLogPath(buffer);
    Log::InitBattleLogPath(buffer);
    Cryptor::InitKey("!crAckmE4nOthIng:-)");
    DispatcherManager::InitDispatcher();
    PacketProcessor::SetClientType(clientType);
}

void Injector::StartInjector()
{
    std::thread pipeServer([this]
                           { this->PipeServerLoop(); });

    std::string gamePath = clientType == ClientType::Flash ? gamePath_Flash : gamePath_Unity;

    cout << gamePath << endl;

    STARTUPINFOA startInfo = {sizeof(startInfo)};
    PROCESS_INFORMATION processInfo = {};
    if (!CreateProcessA(gamePath.c_str(), nullptr, nullptr, nullptr,
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
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // 消息模式
        1, 0, 0, 0, nullptr);

    if (pipe == INVALID_HANDLE_VALUE)
    {
        Log::WriteLog("创建管道失败，错误码：" + std::to_string(GetLastError()), LogLevel::Error);
        return;
    }

    Log::WriteLog("等待 SocketHook.dll 连接到管道...");

    if (!ConnectNamedPipe(pipe, nullptr) && GetLastError() != ERROR_PIPE_CONNECTED)
    {
        Log::WriteLog("连接管道失败，错误码：" + std::to_string(GetLastError()), LogLevel::Error);
        CloseHandle(pipe);
        return;
    }

    // 最大包大小（header + payload），按需调整
    const size_t MAX_PACKET_SIZE = 10 * 1024 * 1024; // 10MB
    std::vector<char> msgBuf(MAX_PACKET_SIZE);

    while (true)
    {
        DWORD bytesRead = 0;
        if (!ReadFile(pipe, msgBuf.data(), (DWORD)msgBuf.size(), &bytesRead, nullptr) || bytesRead == 0)
        {
            break; // 管道断开
        }

        if (bytesRead < sizeof(PacketHeader))
        {
            Log::WriteLog("收到数据太短，无法解析 header", LogLevel::Error);
            break;
        }

        // 解析包头
        PacketHeader header;
        memcpy(&header, msgBuf.data(), sizeof(PacketHeader));

        // 校验长度
        if (header.payloadSize != bytesRead - sizeof(PacketHeader))
        {
            Log::WriteLog("数据长度不匹配: header.payloadSize=" +
                              std::to_string(header.payloadSize) + " 实际=" +
                              std::to_string(bytesRead - sizeof(PacketHeader)),
                          LogLevel::Error);
            break;
        }
        if (header.payloadSize > MAX_PACKET_SIZE - sizeof(PacketHeader))
        {
            Log::WriteLog("Payload 太大，拒绝处理", LogLevel::Error);
            break;
        }

        if (header.payloadSize == 1)
            continue;

        // 复制 payload
        std::vector<char> payload(header.payloadSize);
        if (header.payloadSize > 0)
        {
            memcpy(payload.data(), msgBuf.data() + sizeof(PacketHeader), header.payloadSize);
        }

        // 打印数据（十六进制）
        std::ostringstream oss;
        for (size_t i = 0; i < payload.size(); ++i)
        {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << (unsigned int)(unsigned char)payload[i] << " ";
        }
        Log::WriteLog(oss.str(), LogLevel::Temp, false);

        // 分发处理
        if (header.direction == 0)
            PacketProcessor::ProcessRecvPacket(header.socket, payload, header.payloadSize);
        else
            PacketProcessor::ProcessSendPacket(header.socket, payload, header.payloadSize);
    }

    Log::WriteLog("管道断开，退出。");
    CloseHandle(pipe);
}

bool Injector::ReadExact(HANDLE pipe, void *buffer, size_t size)
{
    char *ptr = static_cast<char *>(buffer);
    size_t totalRead = 0;
    DWORD bytesRead = 0;
    while (totalRead < size)
    {
        if (!ReadFile(pipe, ptr + totalRead, (DWORD)(size - totalRead), &bytesRead, nullptr) || bytesRead == 0)
            return false;
        totalRead += bytesRead;
    }
    return true;
}

int main()
{
    Injector *injector = new Injector();
    injector->StartInjector();
    delete injector;
    return 0;
}