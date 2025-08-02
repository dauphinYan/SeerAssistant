#include "Log.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <direct.h>

#include <iostream>

char Log::logPath[MAX_PATH];
std::mutex Log::logMutex;
char Log::BattleLogPath[MAX_PATH];
std::mutex Log::BattleLogMutex;

std::string Log::timeStr;

static std::string GetTimeStampForFileName()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

static std::string GetTimeStamp()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto in_time_t = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

static std::thread::id GetThreadId()
{
    return std::this_thread::get_id();
}

const char *Log::LogLevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Temp:
        return "Temp";
    case LogLevel::Error:
        return "Error";
    default:
        return "Unknown";
    }
}

void Log::InitLogPath(char *InLogPath)
{
    size_t len = strlen(InLogPath);
    if (InLogPath[len - 1] != '\\')
    {
        strcat(InLogPath, "\\");
    }

    strcat(InLogPath, "Log\\");
    _mkdir(InLogPath);

    strcat(InLogPath, "System\\");
    _mkdir(InLogPath);

    timeStr = GetTimeStampForFileName();
    std::string fullPath = std::string(InLogPath) + "System-" + timeStr + ".log";

    strncpy(logPath, fullPath.c_str(), MAX_PATH - 1);
    logPath[MAX_PATH - 1] = '\0';
}

void Log::WriteLog(const std::string &msg, LogLevel level, bool bShouldWrite)
{
    if (!bShouldWrite)
        return;
    std::lock_guard<std::mutex> lock(logMutex);
    std::ofstream ofs(logPath, std::ios::app);
    if (!ofs.is_open())
        return;

    ofs << "[" << GetTimeStamp() << "] "
        << "[" << LogLevelToString(level) << "] "
        << msg << "\n";
}

void Log::InitBattleLogPath(char *InBattleLogPath)
{
    char *lastSlash = strrchr(BattleLogPath, '\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = '\0';
    }
    else
    {
        strcpy(BattleLogPath, ".\\");
    }

    strcat(BattleLogPath, "Log\\");
    _mkdir(BattleLogPath);

    strcat(BattleLogPath, "User\\");
    _mkdir(BattleLogPath);

    std::string fullPath = std::string(BattleLogPath) + "BattleLog-" + timeStr + ".log";

    strncpy(BattleLogPath, fullPath.c_str(), MAX_PATH - 1);
    BattleLogPath[MAX_PATH - 1] = '\0';

    std::lock_guard<std::mutex> lock(BattleLogMutex);
    std::ofstream ofs(BattleLogPath, std::ios::app);
    if (!ofs.is_open())
        return;

    ofs << "战斗日志创建完毕！" << "\n\n";
}

void Log::WriteBattleLog(const std::string &msg, bool bShouldWrite)
{
    if (!bShouldWrite)
        return;
    std::lock_guard<std::mutex> lock(BattleLogMutex);
    std::ofstream ofs(BattleLogPath, std::ios::app);
    if (!ofs.is_open())
        return;

    ofs << msg << "\n\n";
}
