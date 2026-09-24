#include "SocketHook.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <algorithm>

std::atomic<ClientType> g_clientType = ClientType::Unity;
std::atomic<bool> g_hookEnabled = true;
std::atomic<bool> g_running = true;
std::mutex g_dataMutex;
HMODULE hModule;

decltype(&recv) originalRecv = nullptr;
decltype(&send) originalSend = nullptr;
decltype(&recvfrom) originalRecvFrom = nullptr;

static HANDLE hPipe = INVALID_HANDLE_VALUE;
static std::mutex pipeMutex;

void WriteDebugLog(const std::string &message)
{
    std::ofstream logFile("Log/Sockethook_debug.log", std::ios::app);
    if (logFile.is_open())
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        logFile << "[" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << "] "
                << message << std::endl;
        logFile.close();
    }
}

int WINAPI RecvEvent(SOCKET s, char *buf, int len, int flags)
{
    int ret = originalRecv(s, buf, len, flags);
    if (g_hookEnabled && ret > 0)
    {
        std::lock_guard<std::mutex> lk(g_dataMutex);
        SendToInjector(s, buf, ret, false);
    }
    return ret;
}

int WINAPI RecvFromEvent(SOCKET s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
    int ret = originalRecvFrom(s, buf, len, flags, from, fromlen);
    if (g_hookEnabled && ret > 0)
    {
        std::lock_guard<std::mutex> lk(g_dataMutex);
        SendToInjector(s, buf, ret, false);
    }
    return ret;
}

int WINAPI SendEvent(SOCKET s, char *buf, int len, int flags)
{
    int ret = originalSend(s, buf, len, flags);
    if (g_hookEnabled && ret > 0)
    {
        std::lock_guard<std::mutex> lk(g_dataMutex);
        SendToInjector(s, buf, ret, true);
    }
    return ret;
}

void InitPipeClient()
{
    WriteDebugLog("开始初始化管道客户端...");

    int retryCount = 0;
    const int maxRetries = 10;

    while (retryCount < maxRetries)
    {
        hPipe = CreateFileW(
            HOOK_PIPE_NAME,
            GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr);

        if (hPipe != INVALID_HANDLE_VALUE)
        {
            WriteDebugLog("管道连接成功!");
            break;
        }

        DWORD error = GetLastError();
        WriteDebugLog("管道连接失败，错误码: " + std::to_string(error));

        if (error != ERROR_PIPE_BUSY)
        {
            WriteDebugLog("管道不可用，等待重试...");
            Sleep(1000);
            retryCount++;
            continue;
        }

        WriteDebugLog("管道忙碌，等待可用...");
        if (!WaitNamedPipeW(HOOK_PIPE_NAME, 5000))
        {
            WriteDebugLog("等待管道超时，重试...");
            retryCount++;
            continue;
        }
    }

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        WriteDebugLog("管道连接最终失败，达到最大重试次数");
    }
}

void SendToInjector(SOCKET s, const char *data, size_t len, bool isSend)
{
    std::lock_guard<std::mutex> lk(pipeMutex);
    if (hPipe == INVALID_HANDLE_VALUE)
        return;

    PacketHeader header;
    header.socket = (uint32_t)(uintptr_t)s;
    header.payloadSize = (uint32_t)len;
    header.direction = isSend ? 1 : 0;
    header.totalSize = sizeof(PacketHeader) + header.payloadSize;

    std::vector<char> buffer(sizeof(PacketHeader) + len);
    memcpy(buffer.data(), &header, sizeof(PacketHeader));
    memcpy(buffer.data() + sizeof(PacketHeader), data, len);

    DWORD written = 0;
    WriteFile(hPipe, buffer.data(), (DWORD)buffer.size(), &written, nullptr);
}

void InitHook(ClientType type)
{
    WriteDebugLog("开始初始化Hook，客户端类型: " + std::to_string((int)type));

    g_clientType = type;

    if (MH_Initialize() != MH_OK)
    {
        WriteDebugLog("MH_Initialize 失败");
        return;
    }
    WriteDebugLog("MH_Initialize 成功");

    // 等待 ws2_32.dll 加载
    HMODULE ws2_32 = nullptr;
    int retryCount = 0;
    const int maxRetries = 50; // 最多等待 5 秒

    while (retryCount < maxRetries && !ws2_32)
    {
        ws2_32 = GetModuleHandleW(L"ws2_32");
        if (!ws2_32)
        {
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

    // 获取函数地址
    LPVOID targetRecv = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv"));
    LPVOID targetSend = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "send"));
    LPVOID targetRecvFrom = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recvfrom"));

    if (!targetRecv || !targetSend)
    {
        WriteDebugLog("获取 recv/send 函数地址失败");
    }

    if (targetRecv)
    {
        if (MH_CreateHook(targetRecv, reinterpret_cast<LPVOID>(RecvEvent), reinterpret_cast<LPVOID *>(&originalRecv)) != MH_OK)
            WriteDebugLog("创建 recv hook 失败");
    }

    if (targetSend)
    {
        if (MH_CreateHook(targetSend, reinterpret_cast<LPVOID>(SendEvent), reinterpret_cast<LPVOID *>(&originalSend)) != MH_OK)
            WriteDebugLog("创建 send hook 失败");
    }

    if (targetRecvFrom && g_clientType == ClientType::Unity)
    {
        if (MH_CreateHook(targetRecvFrom, reinterpret_cast<LPVOID>(RecvFromEvent), reinterpret_cast<LPVOID *>(&originalRecvFrom)) != MH_OK)
            WriteDebugLog("创建 recvfrom hook 失败");
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        WriteDebugLog("启用 Hook 失败");
        return;
    }

    WriteDebugLog("Hook 创建并启用完成");

    InitPipeClient();
    WriteDebugLog("Hook 初始化完成");
}

DWORD WINAPI InitHook_Thread(LPVOID lpParam)
{
    WriteDebugLog("InitHook_Thread 开始执行");
    ClientType type = *reinterpret_cast<ClientType *>(lpParam);
    InitHook(type);
    WriteDebugLog("InitHook_Thread 执行完成");
    return 0;
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
        if (ws2_32)
        {
            LPVOID p;
            p = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv"));
            if (p)
                MH_DisableHook(p);
            p = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "send"));
            if (p)
                MH_DisableHook(p);
            p = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recvfrom"));
            if (p)
                MH_DisableHook(p);
        }
        MH_Uninitialize();
    }
    return TRUE;
}
