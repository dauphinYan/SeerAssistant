#pragma once

#include <fstream>
#include <mutex>
#include <windows.h>

using namespace std;

class Log
{
public:
    static void InitLogPath(HMODULE hModule);

    static void WriteLog(const string &msg);

private:
    static char logPath[MAX_PATH];
    
    static mutex logMutex;
};