#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include "MinHook.h"

#include <fstream>
#include <mutex>

using namespace std;

std::mutex logMutex;

char logPath[MAX_PATH] = {0};

typedef int(WINAPI *RecvFn)(SOCKET, char *, int, int);

RecvFn OriginalRecv = nullptr;
