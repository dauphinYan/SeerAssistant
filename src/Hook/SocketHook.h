#pragma once

#include <winsock2.h>
#include <windows.h>
#include <atomic>
#include <mutex>

#include "MinHook.h"
#include "Src/Hook/HookProtocol.h"
#include <vector>

extern std::atomic<ClientType> g_clientType;
extern std::atomic<bool> g_hookEnabled;
extern std::atomic<bool> g_running;
extern std::mutex g_dataMutex;

// 原始函数指针（我们会保存它们以便在钩子中调用真实实现）
extern decltype(&recv) originalRecv;
extern decltype(&send) originalSend;
extern decltype(&recvfrom) originalRecvFrom;

// 导出的事件回调（钩子函数）
extern int WINAPI RecvEvent(SOCKET, char *, int, int);
extern int WINAPI RecvFromEvent(SOCKET, char *, int, int, struct sockaddr *, int *);
extern int WINAPI SendEvent(SOCKET, char *, int, int);

// 管道和发送数据到注入端的函数
extern void InitPipeClient();
extern void SendToInjector(SOCKET s, const char *data, size_t len, bool isSend);
extern void InitHook(ClientType type);

// 需要外部调用的函数。
extern "C" __declspec(dllexport)
DWORD WINAPI
InitHook_Thread(LPVOID lpParam);
