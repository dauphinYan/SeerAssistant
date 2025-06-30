#pragma once

#include <winsock2.h>
#include <windows.h>

#include "MinHook.h"

typedef int(WINAPI *RecvFn)(SOCKET, char *, int, int);

RecvFn OriginalRecv = nullptr;