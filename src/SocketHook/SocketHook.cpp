#include "SocketHook.h"
#include "src/Common/Log.h"
#include "src/PacketParser/Packet.h"
#include "src/PacketParser/Cryptor.h"
#include <sstream>
#include <iomanip>

std::atomic<EClientType> g_ClientType = EClientType::Flash;
std::atomic<bool> g_hookEnabled = true;
std::atomic<bool> g_running = true;
std::mutex g_DataMutex;

decltype(&recv) OriginalRecv = nullptr;
decltype(&send) OriginalSend = nullptr;
int(PASCAL *OriginalWSARecv)(
    SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD,
    LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE) = nullptr;
int(PASCAL *OriginalWSASend)(
    SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD,
    LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE) = nullptr;

// 接收同步数据（Flash）
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
        Log::WriteLog("[Flash recv] " + oss.str(), LogLevel::Temp, false);
        PacketProcessor::ProcessRecvPacket(s, std::vector<char>(buf, buf + ret), ret);
    }
    return ret;
}

// 发送同步数据（Flash）
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

// 接收异步数据（Unity）
int PASCAL WSARecvEvent(
    SOCKET s, LPWSABUF bufs, DWORD bufCount, LPDWORD received, LPDWORD flags,
    LPWSAOVERLAPPED ov, LPWSAOVERLAPPED_COMPLETION_ROUTINE cb)
{
    int ret = OriginalWSARecv(s, bufs, bufCount, received, flags, ov, cb);
    if (g_hookEnabled && ret == 0 && received && *received > 0)
    {
        std::lock_guard<std::mutex> lk(g_DataMutex);
        char *data = bufs[0].buf;
        int len = static_cast<int>(*received);
        std::ostringstream oss;
        for (int i = 0; i < len; ++i)
            oss << std::hex << std::setw(2) << std::setfill('0')
                << (unsigned int)(unsigned char)data[i] << " ";
        Log::WriteLog("[Unity recv] " + oss.str(), LogLevel::Temp, true);
        PacketProcessor::ProcessRecvPacket(s, std::vector<char>(data, data + len), len);
    }
    return ret;
}

// 发送异步数据（Unity）
int PASCAL WSASendEvent(
    SOCKET s, LPWSABUF bufs, DWORD bufCount, LPDWORD sent, DWORD flags,
    LPWSAOVERLAPPED ov, LPWSAOVERLAPPED_COMPLETION_ROUTINE cb)
{
    int ret = OriginalWSASend(s, bufs, bufCount, sent, flags, ov, cb);
    if (g_hookEnabled && ret == 0 && sent && *sent > 0)
    {
        std::lock_guard<std::mutex> lk(g_DataMutex);
        char *data = bufs[0].buf;
        int len = static_cast<int>(*sent);
        std::ostringstream oss;
        for (int i = 0; i < len; ++i)
            oss << std::hex << std::setw(2) << std::setfill('0')
                << (unsigned int)(unsigned char)data[i] << " ";
        Log::WriteLog("[Unity send] " + oss.str(), LogLevel::Temp, false);
        PacketProcessor::ProcessSendPacket(s, std::vector<char>(data, data + len), len);
    }
    return ret;
}

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

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID)
{
    PacketProcessor::SetClientType(g_ClientType);

    if (reason == DLL_PROCESS_ATTACH)
    {
        MH_Initialize();
        Log::InitLogPath(hMod);
        Cryptor::InitKey("!crAckmE4nOthIng:-)");

        HMODULE ws2_32 = GetModuleHandleW(L"ws2_32");
        if (!ws2_32)
        {
            Log::WriteLog("GetModuleHandleW failed", LogLevel::Error);
            return FALSE;
        }

            LPVOID TargetRecv = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv"));
            LPVOID TargetSend = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "send"));
            MH_CreateHook(TargetRecv, reinterpret_cast<LPVOID>(RecvEvent), reinterpret_cast<LPVOID *>(&OriginalRecv));
            MH_CreateHook(TargetSend, reinterpret_cast<LPVOID>(SendEvent), reinterpret_cast<LPVOID *>(&OriginalSend));
            MH_EnableHook(TargetRecv);
            MH_EnableHook(TargetSend);
            Log::WriteLog("Flash hook is installed.");

        // if (g_ClientType == EClientType::Flash)
        // {
        //     LPVOID TargetRecv = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv"));
        //     LPVOID TargetSend = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "send"));
        //     MH_CreateHook(TargetRecv, reinterpret_cast<LPVOID>(RecvEvent), reinterpret_cast<LPVOID *>(&OriginalRecv));
        //     MH_CreateHook(TargetSend, reinterpret_cast<LPVOID>(SendEvent), reinterpret_cast<LPVOID *>(&OriginalSend));
        //     MH_EnableHook(TargetRecv);
        //     MH_EnableHook(TargetSend);
        //     Log::WriteLog("Flash hook is installed.");
        // }
        // else // Unity
        // {
        //     LPVOID TargetRecv = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "WSARecv"));
        //     LPVOID TargetSend = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "WSASend"));
        //     status = MH_CreateHook(TargetRecv, reinterpret_cast<LPVOID>(WSARecvEvent), reinterpret_cast<LPVOID *>(&OriginalWSARecv));
        //     Log::WriteLog("MH_CreateHook WSARecv: " + std::to_string(status));
        //     MH_CreateHook(TargetSend, reinterpret_cast<LPVOID>(WSASendEvent), reinterpret_cast<LPVOID *>(&OriginalWSASend));
        //     status = MH_EnableHook(TargetRecv);
        //     Log::WriteLog("MH_EnableHook WSARecv: " + std::to_string(status));
        //     MH_EnableHook(TargetSend);
        //     Log::WriteLog("Unity hook is installed.");
        // }

        // 启动监控线程
        CreateThread(nullptr, 0, MonitorThread, nullptr, 0, nullptr);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        HMODULE ws2_32 = GetModuleHandleW(L"ws2_32");

        // 取消钩子
        if (g_ClientType == EClientType::Flash)
        {
            MH_DisableHook(reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv")));
            MH_DisableHook(reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "send")));
        }
        else
        {
            MH_DisableHook(reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "WSARecv")));
            MH_DisableHook(reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "WSASend")));
        }
        MH_Uninitialize();
    }
    return TRUE;
}