#include "SocketHook.h"
#include "src/Common/Log.h"
#include "src/PacketParser/Packet.h"
#include "src/PacketParser/Cryptor.h"
#include <sstream>
#include <iomanip>

std::atomic<EClientType> g_ClientType = EClientType::Unity;
std::atomic<bool> g_hookEnabled = true;
std::atomic<bool> g_running = true;
std::mutex g_DataMutex;

decltype(&recv) OriginalRecv = nullptr;
decltype(&send) OriginalSend = nullptr;

DWORD WINAPI MonitorThread(LPVOID)
{
    const int toggleKey = VK_F8;
    const int exitKey = VK_F9;
    while (g_running)
    {
        if (GetAsyncKeyState(toggleKey) & 1)
            Log::WriteLog((g_hookEnabled = !g_hookEnabled) ? "Hook Enabled!" : "Hook Disabled!");
        if (GetAsyncKeyState(exitKey) & 1)
        {
            Log::WriteLog("Exit key pressed, shutting down...");
            g_running = false;
        }
        Sleep(100);
    }
    return 0;
}

void InitHook(EClientType type)
{
    g_ClientType = type;
    // 初始化 MinHook
    if (MH_Initialize() != MH_OK)
    {
        Log::WriteLog("MH_Initialize failed", LogLevel::Error);
        return;
    }

    // 告诉 PacketProcessor 目标客户端类型
    PacketProcessor::SetClientType(g_ClientType);

    // 获取 ws2_32 模块句柄
    HMODULE ws2_32 = GetModuleHandleW(L"ws2_32");
    if (!ws2_32)
    {
        Log::WriteLog("GetModuleHandleW failed", LogLevel::Error);
        return;
    }

    LPVOID targetRecv = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv"));
    LPVOID targetSend = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "send"));
    MH_CreateHook(targetRecv, reinterpret_cast<LPVOID>(RecvEvent), reinterpret_cast<LPVOID *>(&OriginalRecv));
    MH_CreateHook(targetSend, reinterpret_cast<LPVOID>(SendEvent), reinterpret_cast<LPVOID *>(&OriginalSend));
    MH_EnableHook(targetRecv);
    MH_EnableHook(targetSend);

    if (g_ClientType == EClientType::Flash)
    {
        Log::WriteLog("Flash hook is installed.");
    }
    else // Unity
    {
        Log::WriteLog("Unity hook is installed.");
    }

    CreateThread(nullptr, 0, MonitorThread, nullptr, 0, nullptr);
}

DWORD WINAPI InitHook_Thread(LPVOID lpParam)
{
    EClientType type = *reinterpret_cast<EClientType *>(lpParam);
    InitHook(type);
    return 0;
}

// 接收同步数据
int WINAPI RecvEvent(SOCKET s, char *buf, int len, int flags)
{
    int ret = OriginalRecv(s, buf, len, flags);
    if (g_hookEnabled && ret > 0)
    {
        std::lock_guard<std::mutex> lk(g_DataMutex);
        std::ostringstream oss;
        for (int i = 0; i < ret; ++i)
            oss << std::hex << std::setw(2) << std::setfill('0')
                << (unsigned int)(unsigned char)buf[i] << " ";

        if (g_ClientType == EClientType::Flash)
        {
            Log::WriteLog("[Flash recv] " + oss.str(), LogLevel::Temp, false);
        }
        else
        {
            Log::WriteLog("[Unity recv] " + oss.str(), LogLevel::Temp, true);
        }
        PacketProcessor::ProcessRecvPacket(s, std::vector<char>(buf, buf + ret), ret);
    }
    return ret;
}

// 发送同步数据
int WINAPI SendEvent(SOCKET s, char *buf, int len, int flags)
{
    int ret = OriginalSend(s, buf, len, flags);
    if (g_hookEnabled && ret > 0)
    {
        std::lock_guard<std::mutex> lk(g_DataMutex);
        std::ostringstream oss;
        for (int i = 0; i < ret; ++i)
            oss << std::hex << std::setw(2) << std::setfill('0')
                << (unsigned int)(unsigned char)buf[i] << " ";
        Log::WriteLog("[Flash send] " + oss.str(), LogLevel::Temp, false);
        PacketProcessor::ProcessSendPacket(s, std::vector<char>(buf, buf + ret), ret);
    }
    return ret;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID)
{
    PacketProcessor::SetClientType(g_ClientType);

    if (reason == DLL_PROCESS_ATTACH)
    {
        Log::InitLogPath(hMod);
        Log::InitBattleLogPath(hMod);
        Cryptor::InitKey("!crAckmE4nOthIng:-)");
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        HMODULE ws2_32 = GetModuleHandleW(L"ws2_32");

        MH_DisableHook(reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv")));
        MH_DisableHook(reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "send")));

        MH_Uninitialize();
    }
    return TRUE;
}