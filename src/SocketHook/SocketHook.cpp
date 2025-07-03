#include "SocketHook.h"
#include "src/Common/Log.h"
#include "src/PacketParser/Packet.h"
#include "src/PacketParser/Cryptor.h"
#include <sstream>
#include <iomanip>

int WINAPI RecvEvent(SOCKET S, char *BufferPtr, int Length, int Flag)
{
    int Result = OriginalRecv(S, BufferPtr, Length, Flag); // 缓冲区长度。

    if (g_hookEnabled && Result > 0)
    {
        std::lock_guard<std::mutex> lock(g_recvMutex);

        // std::ostringstream oss;
        // for (int i = 0; i < Result; ++i)
        // {
        //     oss << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)(unsigned char)BufferPtr[i] << " ";
        // }
        // std::string hexStr = oss.str();
        // Log::WriteLog("[Hooked recv] Data (hex): " + hexStr);

        std::vector<char> Temp(BufferPtr, BufferPtr + Result);

        PacketProcessor::ProcessRecvPacket(S, Temp, Result);
    }

    return Result;
}

int WINAPI SendEvent(SOCKET S, char *BufferPtr, int Length, int Flag)
{
    int Result = OriginalSend(S, BufferPtr, Length, Flag);

    if (g_hookEnabled && Result > 0)
    {
        std::lock_guard<std::mutex> lock(g_sendMutex);

        // std::ostringstream oss;
        // for (int i = 0; i < Result; ++i)
        // {
        //     oss << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)(unsigned char)BufferPtr[i] << " ";
        // }
        // std::string hexStr = oss.str();
        // Log::WriteLog("[Hooked send] Data (hex): " + hexStr);

        std::vector<char> Temp(BufferPtr, BufferPtr + Result);

        PacketProcessor::ProcessSendPacket(S, Temp, Result);
    }

    return Result;
}

DWORD WINAPI MonitorThread(LPVOID)
{
    const int toggleKey = VK_F8; // F8 键用于切换 Hook 开关
    const int exitKey = VK_F9;   // F9 键用于关闭程序

    while (g_running)
    {
        if (GetAsyncKeyState(toggleKey) & 1)
        {
            g_hookEnabled = !g_hookEnabled;
            Log::WriteLog(g_hookEnabled ? "Hook Enabled!" : "Hook Disabled!");
        }

        if (GetAsyncKeyState(exitKey) & 1)
        {
            Log::WriteLog("Exit key pressed, shutting down...");
            g_running = false;
        }

        Sleep(100);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    // ul_reason_for_call：当前DLL事件。
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) // DLL正在被加载到某个进程中。
    {
        MH_Initialize();

        Log::InitLogPath(hModule);

        // 获取目标函数地址。
        HMODULE ws2_32 = GetModuleHandleW(L"ws2_32");
        if (ws2_32 == nullptr)
        {
            Log::WriteLog("Fail to get target address.", LogLevel::Error);
            return FALSE;
        }
        // 获取 recv 函数地址。
        LPVOID TargetRecv = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv"));
        if (TargetRecv == nullptr)
        {
            Log::WriteLog("Fail to get recv address.", LogLevel::Error);
            return FALSE;
        }

        LPVOID TargetSend = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "send"));
        if (TargetSend == nullptr)
        {
            Log::WriteLog("Fail to get send address.", LogLevel::Error);
            return FALSE;
        }

        // 创建钩子，强制类型转换函数指针。
        if (MH_CreateHook(TargetRecv, reinterpret_cast<LPVOID>(RecvEvent), reinterpret_cast<LPVOID *>(&OriginalRecv)) != MH_OK)
        {
            Log::WriteLog("Fail to create recv hook.", LogLevel::Error);
            return FALSE;
        }

        if (MH_CreateHook(TargetSend, reinterpret_cast<LPVOID>(SendEvent), reinterpret_cast<LPVOID *>(&OriginalSend)) != MH_OK)
        {
            Log::WriteLog("Fail to create send hook.", LogLevel::Error);
            return FALSE;
        }

        // 启用钩子。
        if (MH_EnableHook(TargetRecv) != MH_OK)
        {
            Log::WriteLog("Fail to enable recv hook.", LogLevel::Error);
            return FALSE;
        }

        if (MH_EnableHook(TargetSend) != MH_OK)
        {
            Log::WriteLog("Fail to enable send hook.", LogLevel::Error);
            return FALSE;
        }

        Log::WriteLog("DLL is injected successful.");

        Cryptor::InitKey("!crAckmE4nOthIng:-)");

        CreateThread(nullptr, 0, MonitorThread, nullptr, 0, nullptr);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) // DLL与进程分离。
    {
        HMODULE ws2_32 = GetModuleHandleW(L"ws2_32");
        if (ws2_32)
        {
            LPVOID targetRecv = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv"));
            LPVOID targetSend = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "send"));
            if (targetRecv)
            {
                MH_DisableHook(targetRecv);
            }
            if (targetSend)
            {
                MH_DisableHook(targetSend);
            }
        }
        MH_Uninitialize();
    }

    return TRUE;
}