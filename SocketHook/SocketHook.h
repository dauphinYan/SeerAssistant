#pragma once

#include <winsock2.h>
#include <windows.h>
#include <atomic>
#include <mutex>

#include "MinHook.h"

enum class EClientType
{
    Flash,
    Unity
};

extern std::atomic<EClientType> g_ClientType;
extern std::atomic<bool> g_hookEnabled;
extern std::atomic<bool> g_running;
extern std::mutex g_DataMutex;

// 设置客户端类型
extern "C" __declspec(dllexport)
DWORD WINAPI InitHook_Thread(LPVOID lpParam);

// 原始函数指针
extern decltype(&recv) OriginalRecv;
extern decltype(&send) OriginalSend;

// Hook 函数声明
int WINAPI RecvEvent(SOCKET, char *, int, int);
int WINAPI SendEvent(SOCKET, char *, int, int);

