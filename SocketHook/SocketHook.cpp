#include "SocketHook.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>

void WriteDebugLog(const std::string& message) {
    std::ofstream logFile("sockethook_debug.log", std::ios::app);
    if (logFile.is_open()) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        logFile << "[" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << "] " 
                << message << std::endl;
        logFile.close();
    }
}

std::atomic<EClientType> g_ClientType = EClientType::Unity;
std::atomic<bool> g_hookEnabled = true;
std::atomic<bool> g_running = true;
std::mutex g_DataMutex;
HMODULE hModule;

decltype(&recv) OriginalRecv = nullptr;
decltype(&send) OriginalSend = nullptr;

static const wchar_t *PIPE_NAME = L"\\\\.\\pipe\\SeerSocketHook";
static HANDLE hPipe = INVALID_HANDLE_VALUE;
static std::mutex pipeMutex;

void InitPipeClient()
{
    WriteDebugLog("开始初始化管道客户端...");
    
    int retryCount = 0;
    const int maxRetries = 10;
    
    while (retryCount < maxRetries)
    {
        hPipe = CreateFileW(
            PIPE_NAME,
            GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            nullptr);
            
        if (hPipe != INVALID_HANDLE_VALUE) {
            WriteDebugLog("管道连接成功!");
            break;
        }

        DWORD error = GetLastError();
        WriteDebugLog("管道连接失败，错误码: " + std::to_string(error));
        
        if (error != ERROR_PIPE_BUSY) {
            WriteDebugLog("管道不可用，等待重试...");
            Sleep(1000);
            retryCount++;
            continue;
        }
        
        WriteDebugLog("管道忙碌，等待可用...");
        if (!WaitNamedPipeW(PIPE_NAME, 5000)) {
            WriteDebugLog("等待管道超时，重试...");
            retryCount++;
            continue;
        }
    }
    
    if (hPipe == INVALID_HANDLE_VALUE) {
        WriteDebugLog("管道连接最终失败，达到最大重试次数");
    }
}

void SendToInjector(SOCKET s, const char *data, size_t len, bool isSend)
{
    std::lock_guard<std::mutex> lk(pipeMutex);
    if (hPipe == INVALID_HANDLE_VALUE)
        return;

    struct PacketHeader
    {
        uint32_t totalSize;
        uint32_t socket;
        uint32_t payloadSize;
        uint8_t direction;    // 0 = recv, 1 = send
    };

    PacketHeader header;
    header.socket = (uint32_t)(uintptr_t)s;
    header.payloadSize = (uint32_t)len;
    header.direction = isSend ? 1 : 0;
    header.totalSize = sizeof(PacketHeader) + header.payloadSize;

    DWORD written = 0;
    WriteFile(hPipe, &header, sizeof(header), &written, nullptr);
    WriteFile(hPipe, data, header.payloadSize, &written, nullptr);
}

DWORD WINAPI MonitorThread(LPVOID)
{
    const int toggleKey = VK_F8;
    const int exitKey = VK_F9;
    while (g_running)
    {
        if (GetAsyncKeyState(toggleKey) & 1)
        {
            // Toggle hook
        }
        if (GetAsyncKeyState(exitKey) & 1)
        {
            g_running = false;
        }
        Sleep(100);
    }
    return 0;
}

void InitHook(EClientType type)
{
    WriteDebugLog("开始初始化Hook，客户端类型: " + std::to_string((int)type));
    
    g_ClientType = type;

    if (MH_Initialize() != MH_OK)
    {
        WriteDebugLog("MH_Initialize 失败");
        return;
    }
    WriteDebugLog("MH_Initialize 成功");

    // 等待ws2_32.dll加载，增加重试机制
    HMODULE ws2_32 = nullptr;
    int retryCount = 0;
    const int maxRetries = 50; // 最多等待5秒
    
    while (retryCount < maxRetries && !ws2_32) {
        ws2_32 = GetModuleHandleW(L"ws2_32");
        if (!ws2_32) {
            WriteDebugLog("ws2_32.dll 尚未加载，等待中... (尝试 " + std::to_string(retryCount + 1) + "/" + std::to_string(maxRetries) + ")");
            Sleep(100);
            retryCount++;
        }
    }
    
    if (!ws2_32)
    {
        WriteDebugLog("ws2_32.dll 加载失败，无法继续");
        return;
    }
    WriteDebugLog("ws2_32.dll 加载成功");

    LPVOID targetRecv = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv"));
    LPVOID targetSend = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "send"));
    
    if (!targetRecv || !targetSend) {
        WriteDebugLog("获取recv/send函数地址失败");
        return;
    }
    WriteDebugLog("获取recv/send函数地址成功");
    
    if (MH_CreateHook(targetRecv, reinterpret_cast<LPVOID>(RecvEvent), reinterpret_cast<LPVOID *>(&OriginalRecv)) != MH_OK) {
        WriteDebugLog("创建recv hook失败");
        return;
    }
    
    if (MH_CreateHook(targetSend, reinterpret_cast<LPVOID>(SendEvent), reinterpret_cast<LPVOID *>(&OriginalSend)) != MH_OK) {
        WriteDebugLog("创建send hook失败");
        return;
    }
    WriteDebugLog("Hook创建成功");
    
    if (MH_EnableHook(targetRecv) != MH_OK || MH_EnableHook(targetSend) != MH_OK) {
        WriteDebugLog("启用Hook失败");
        return;
    }
    WriteDebugLog("Hook启用成功");

    InitPipeClient();
    CreateThread(nullptr, 0, MonitorThread, nullptr, 0, nullptr);
    WriteDebugLog("Hook初始化完成");
}

DWORD WINAPI InitHook_Thread(LPVOID lpParam)
{
    WriteDebugLog("InitHook_Thread 开始执行");
    EClientType type = *reinterpret_cast<EClientType *>(lpParam);
    InitHook(type);
    WriteDebugLog("InitHook_Thread 执行完成");
    return 0;
}

int WINAPI RecvEvent(SOCKET s, char *buf, int len, int flags)
{
    int ret = OriginalRecv(s, buf, len, flags);
    if (g_hookEnabled && ret > 0)
    {
        std::lock_guard<std::mutex> lk(g_DataMutex);
        SendToInjector(s, buf, ret, false);
    }
    return ret;
}

int WINAPI SendEvent(SOCKET s, char *buf, int len, int flags)
{
    int ret = OriginalSend(s, buf, len, flags);
    if (g_hookEnabled && ret > 0)
    {
        std::lock_guard<std::mutex> lk(g_DataMutex);
        SendToInjector(s, buf, ret, true);
    }
    return ret;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        WriteDebugLog("DLL_PROCESS_ATTACH");
        DisableThreadLibraryCalls(hMod);
        hModule = hMod;
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        WriteDebugLog("DLL_PROCESS_DETACH");
        HMODULE ws2_32 = GetModuleHandleW(L"ws2_32");
        if (ws2_32) {
            MH_DisableHook(reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv")));
            MH_DisableHook(reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "send")));
        }
        MH_Uninitialize();
    }
    return TRUE;
}