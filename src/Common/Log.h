#pragma once

#include <fstream>
#include <mutex>
#include <windows.h>

enum class LogLevel
{
    Temp,
    Error
};

class Log
{
public:
    static void InitLogPath(char *InlogPath);

    static void WriteLog(const std::string &msg, LogLevel level = LogLevel::Temp, bool bShouldWrite = true);

private:
    static char logPath[MAX_PATH];

    static std::mutex logMutex;

    static std::string timeStr;

    static const char *LogLevelToString(LogLevel level);

};
