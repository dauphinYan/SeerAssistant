#include "SocketHook.h"
#include "src/Common/Log.h"
#include "src/PacketParser/Packet.h"
#include <sstream>
#include <iomanip>

int WINAPI RecvEvent(SOCKET S, char *BufferPtr, int Length, int Flag)
{
    int Result = OriginalRecv(S, BufferPtr, Length, Flag);
    if (Result > 0)
    {
        std::ostringstream oss;
        for (int i = 0; i < Result; ++i)
        {
            oss << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)(unsigned char)BufferPtr[i] << " ";
        }
        std::string hexStr = oss.str();
        Log::WriteLog("[Hooked recv] Data (hex): " + hexStr);

        vector<char> Temp(BufferPtr, BufferPtr + Result);
        // PacketProcessor::ProcessRecvPacket(S, Temp, Result);
    }
    return Result;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    // ul_reason_for_call：当前DLL事件。
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) // DLL正在被加载到某个进程中。
    {
        MH_Initialize();

        Log::InitLogPath(hModule);

        // 获取目标函数地址
        HMODULE ws2_32 = GetModuleHandleW(L"ws2_32");
        if (ws2_32 == nullptr)
        {
            Log::WriteLog("Fail to get target address.");
            return FALSE;
        }
        // 获取 recv 函数地址
        LPVOID TargetRecv = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv"));
        if (TargetRecv == nullptr)
        {
            Log::WriteLog("Fail to get recv address.");
            return FALSE;
        }

        // 创建钩子，强制类型转换函数指针
        if (MH_CreateHook(TargetRecv, reinterpret_cast<LPVOID>(RecvEvent), reinterpret_cast<LPVOID *>(&OriginalRecv)) != MH_OK)
        {
            Log::WriteLog("Fail to create hook.");
            return FALSE;
        }

        // 启用钩子
        if (MH_EnableHook(TargetRecv) != MH_OK)
        {
            Log::WriteLog("Fail to enable hook.");
            return FALSE;
        }

        Log::WriteLog("DLL is injected successful.");
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        HMODULE ws2_32 = GetModuleHandleW(L"ws2_32");
        if (ws2_32 != nullptr)
        {
            LPVOID targetRecv = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv"));
            if (targetRecv != nullptr)
            {
                MH_DisableHook(targetRecv);
            }
        }
        MH_Uninitialize();
    }

    return TRUE;
}