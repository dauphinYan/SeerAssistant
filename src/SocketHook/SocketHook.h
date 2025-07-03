#pragma once

#include <winsock2.h>
#include <windows.h>
#include <atomic>
#include <mutex>

#include "MinHook.h"

std::atomic<bool> g_hookEnabled = false;
std::atomic<bool> g_running = true;

typedef int(WINAPI *RecvFn)(SOCKET, char *, int, int);
RecvFn OriginalRecv = nullptr;
std::mutex g_recvMutex;

typedef int(WINAPI *SendFn)(SOCKET, const char *, int, int);
SendFn OriginalSend = nullptr;
std::mutex g_sendMutex;