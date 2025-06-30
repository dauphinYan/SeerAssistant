#include "SocketHook.h"

void InitLogPath(HMODULE hModule)
{
    GetModuleFileNameA(hModule, logPath, MAX_PATH);

    char *lastSlash = strrchr(logPath, '\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = '\0';
        strcat(logPath, "LogTemp.txt");
    }
    else
    {
        strcpy(logPath, "LogTemp.txt");
    }
}

void WriteLog(const string &msg)
{
    lock_guard<mutex> lock(logMutex);

    ofstream logFile(logPath, ios::app);
    if (logFile.is_open())
    {
        logFile << msg << endl;
        logFile.close();
    }
}

int WINAPI RecvEvent(SOCKET S, char *BufferPtr, int Length, int Flag)
{
    int Result = OriginalRecv(S, BufferPtr, Length, Flag);
    if (Result > 0)
    {
        string bufferContent(BufferPtr, Result);
        WriteLog("[Hooked recv] Data: " + bufferContent);
    }
    return Result;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    // ul_reason_for_call：当前DLL事件。
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) // DLL正在被加载到某个进程中。
    {
        MH_Initialize();

        InitLogPath(hModule);

        // 获取目标函数地址
        HMODULE ws2_32 = GetModuleHandleW(L"ws2_32");
        if (ws2_32 == nullptr)
        {
            WriteLog("Fail to get target address.");
            return FALSE;
        }
        // 获取 recv 函数地址
        LPVOID TargetRecv = reinterpret_cast<LPVOID>(GetProcAddress(ws2_32, "recv"));
        if (TargetRecv == nullptr)
        {
            WriteLog("Fail to get recv address.");
            return FALSE;
        }

        // 创建钩子，强制类型转换函数指针
        if (MH_CreateHook(TargetRecv, reinterpret_cast<LPVOID>(RecvEvent), reinterpret_cast<LPVOID *>(&OriginalRecv)) != MH_OK)
        {
            WriteLog("Fail to create hook.");
            return FALSE;
        }

        // 启用钩子
        if (MH_EnableHook(TargetRecv) != MH_OK)
        {
            WriteLog("Fail to enable hook.");
            return FALSE;
        }

        WriteLog("DLL is injected successful.");
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