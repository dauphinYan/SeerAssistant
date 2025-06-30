#include "Log.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <direct.h>

char Log::logPath[MAX_PATH];
std::mutex Log::logMutex;

static std::string GetTimeStampForFileName() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

static std::string GetTimeStamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto in_time_t = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

static std::thread::id GetThreadId() {
    return std::this_thread::get_id();
}

const char* Log::LogLevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Temp: return "Temp";
    case LogLevel::Error: return "Error";
    default: return "Unknown";
    }
}

void Log::InitLogPath(HMODULE hModule)
{
    GetModuleFileNameA(hModule, logPath, MAX_PATH);
    char* lastSlash = strrchr(logPath, '\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = '\0';
    }
    else
    {
        strcpy(logPath, ".\\");
    }

    strcat(logPath, "Log\\");
    _mkdir(logPath);

    std::string timeStr = GetTimeStampForFileName();
    std::string fullPath = std::string(logPath) + "LogTemp-" + timeStr + ".txt";

    strncpy(logPath, fullPath.c_str(), MAX_PATH - 1);
    logPath[MAX_PATH - 1] = '\0';
}

void Log::WriteLog(const std::string &msg, LogLevel level)
{
    std::lock_guard<std::mutex> lock(logMutex);
    std::ofstream ofs(logPath, std::ios::app);
    if (!ofs.is_open()) return;

    ofs << "[" << GetTimeStamp() << "] "
        << "[ThreadID " << GetThreadId() << "] "
        << "[" << LogLevelToString(level) << "] "
        << msg << "\n";
}
