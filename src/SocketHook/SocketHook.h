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

// 原始函数指针
extern decltype(&recv) OriginalRecv;
extern decltype(&send) OriginalSend;
extern int(PASCAL *OriginalWSARecv)(
    SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD,
    LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
extern int(PASCAL *OriginalWSASend)(
    SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD,
    LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);

// Hook 函数声明
int WINAPI RecvEvent(SOCKET, char *, int, int);
int WINAPI SendEvent(SOCKET, char *, int, int);
int PASCAL WSARecvEvent(
    SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD,
    LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
int PASCAL WSASendEvent(
    SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD,
    LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
DWORD WINAPI MonitorThread(LPVOID);