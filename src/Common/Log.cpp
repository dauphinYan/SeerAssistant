#include "Log.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

char Log::logPath[];
mutex Log::logMutex;

static string GetTimeStamp() {
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


void Log::InitLogPath(HMODULE hModule)
{
    GetModuleFileNameA(hModule, logPath, MAX_PATH);

    char *lastSlash = strrchr(logPath, '\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = '\0';
        strcat(logPath, "LogTemp.txt");
    }
    else
    {
        strcpy(logPath, "LogTemp.txt");
    }
}

void Log::WriteLog(const std::string &msg)
{
    std::lock_guard<std::mutex> lock(logMutex);
    std::ofstream ofs(logPath, std::ios::app);
    if (!ofs.is_open()) return;
    
    ofs << "[" << GetTimeStamp() << "] "
        << "[TID " << GetThreadId() << "] "
        << msg << "\n";
}
