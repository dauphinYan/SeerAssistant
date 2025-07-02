#pragma once

#include <winsock2.h>
#include <windows.h>
#include <atomic>
#include <mutex>

#include "MinHook.h"

std::atomic<bool> g_hookEnabled = false;
std::atomic<bool> g_running = true;

std::mutex g_recvMutex;

typedef int(WINAPI *RecvFn)(SOCKET, char *, int, int);

RecvFn OriginalRecv = nullptr;