#include "Start.h"

#include <iostream>
#include <psapi.h>
#include <vector>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <thread>

#include "Src/Common/Log.h"
#include "Src/Net/PacketParser/Packet.h"
#include "Src/Net/PacketParser/Cryptor.h"

#pragma comment(lib, "psapi.lib")

const std::string Injector::hookDllName = "SocketHook.dll";

const ClientType Injector::clientType = ClientType::Unity;

const std::string Injector::gamePath_Flash = R"(C:\SeerLauncher.exe)";

const std::string Injector::gamePath_Unity = R"(D:\Program Files\Seer\NewSeer\Seer.exe)";

Injector::Injector()
{
    char buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, buffer);

    Log::InitLogPath(buffer);
    Cryptor::InitKey("!crAckmE4nOthIng:-)");
    PacketProcessor::SetClientType(clientType);
}

void Injector::StartInjector()
{
    std::string gamePath = clientType == ClientType::Flash ? gamePath_Flash : gamePath_Unity;

    std::cout << gamePath << std::endl;

    STARTUPINFOA startInfo = {sizeof(startInfo)};
    PROCESS_INFORMATION processInfo = {};
    if (!CreateProcessA(gamePath.c_str(), nullptr, nullptr, nullptr,
                        FALSE, CREATE_SUSPENDED, nullptr, nullptr, &startInfo, &processInfo))
    {
        Log::WriteLog("无法启动目标进程，错误:" + std::to_string(GetLastError()), LogLevel::Error);
        return;
    }
    Log::WriteLog("目标 PID：" + std::to_string(processInfo.dwProcessId));

    std::thread pipeServer([this]
                           { this->PipeServerLoop(); });
    const auto stopPipeServer = [&pipeServer]()
    {
        CancelSynchronousIo(reinterpret_cast<HANDLE>(pipeServer.native_handle()));
        pipeServer.join();
    };

    CHAR full[MAX_PATH] = {0};
    GetFullPathNameA(hookDllName.c_str(), MAX_PATH, full, nullptr);
    std::string dllPath(full);
    std::cout << dllPath << std::endl;
    ResumeThread(processInfo.hThread);

    Log::WriteLog("等待进程启动...");
    WaitForInputIdle(processInfo.hProcess, 15000);
    Sleep(3000);

    DWORD exitCode;
    if (GetExitCodeProcess(processInfo.hProcess, &exitCode) && exitCode != STILL_ACTIVE)
    {
        Log::WriteLog("目标进程已退出，退出码: " + std::to_string(exitCode), LogLevel::Temp);
        stopPipeServer();
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        return;
    }

    Log::WriteLog("开始注入DLL...");
    if (!InjectDll(processInfo.dwProcessId, dllPath, clientType))
    {
        Log::WriteLog("注入失败。", LogLevel::Error);
        stopPipeServer();
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
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
        Log::WriteLog("CreateRemoteThread 失败，错误码:" + std::to_string(GetLastError()), LogLevel::Error);
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
    std::error_code pathError;
    const auto captureDir = std::filesystem::current_path() / "Capture";
    std::filesystem::create_directories(captureDir, pathError);
    if (pathError)
    {
        Log::WriteLog("创建抓取目录失败: " + pathError.message(), LogLevel::Error);
        return;
    }

    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm localTime = {};
    localtime_s(&localTime, &now);
    std::ostringstream fileName;
    fileName << "Capture-" << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S") << ".bin";
    const auto capturePath = captureDir / fileName.str();
    std::ofstream capture(capturePath, std::ios::binary | std::ios::app);
    if (!capture)
    {
        Log::WriteLog("无法打开抓取文件: " + capturePath.string(), LogLevel::Error);
        return;
    }
    Log::WriteLog("原始数据写入: " + capturePath.string());

    HANDLE pipe = CreateNamedPipeW(
        HOOK_PIPE_NAME,
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

    // 管道消息由 SocketHook.dll 的包头和原始 payload 组成。
    const size_t MAX_PACKET_SIZE = 10 * 1024 * 1024;
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
            Log::WriteLog("收到数据太短，缺少管道包头", LogLevel::Error);
            continue;
        }

        // 仅检查管道帧长度，不读取游戏协议字段。
        PacketHeader header;
        memcpy(&header, msgBuf.data(), sizeof(PacketHeader));

        // 校验长度
        if (header.payloadSize != bytesRead - sizeof(PacketHeader))
        {
            Log::WriteLog("数据长度不匹配: header.payloadSize=" +
                              std::to_string(header.payloadSize) + " 实际=" +
                              std::to_string(bytesRead - sizeof(PacketHeader)),
                          LogLevel::Error);
            continue;
        }
        if (header.payloadSize > MAX_PACKET_SIZE - sizeof(PacketHeader))
        {
            Log::WriteLog("Payload 太大，拒绝处理", LogLevel::Error);
            continue;
        }

        capture.write(msgBuf.data(), bytesRead);
        capture.flush();
        if (!capture)
        {
            Log::WriteLog("写入原始数据失败", LogLevel::Error);
            break;
        }

        if (header.payloadSize > 1)
        {
            const char *payloadBegin = msgBuf.data() + sizeof(PacketHeader);
            std::vector<char> payload(payloadBegin, payloadBegin + header.payloadSize);
            if (header.direction == 0)
                PacketProcessor::ProcessRecvPacket(header.socket, payload, header.payloadSize);
            else
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
