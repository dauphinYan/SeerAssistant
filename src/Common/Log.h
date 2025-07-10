#pragma once

#include <fstream>
#include <mutex>
#include <windows.h>

struct PacketData;

enum class LogLevel
{
    Temp,
    Error
};

class Log
{
public:
    static void InitLogPath(HMODULE hModule);

    static void WriteLog(const std::string &msg, LogLevel level = LogLevel::Temp, bool bShouldWrite = true);

private:
    static char logPath[MAX_PATH];

    static std::mutex logMutex;

    static std::string timeStr;

    static const char *LogLevelToString(LogLevel level);

public:
    static void InitBattleLogPath(HMODULE hModule);

    static void WriteBattleLog(const std::string &msg);

private:
    static char BattleLogPath[MAX_PATH];

    static std::mutex BattleLogMutex;

    static void OnUseSkillCmdReceived(const PacketData &Data);
};